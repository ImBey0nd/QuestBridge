# Offline native import inventory

Build outside the checkout to preserve the tracked Windows build directory:

```sh
cmake -S . -B /absolute/external/build
cmake --build /absolute/external/build --parallel 2
ctest --test-dir /absolute/external/build --output-on-failure
```

Inspect an existing, authorized, unencrypted local ELF:

```sh
node tools/diagnostics/ps5-imports.mjs \
  --scanner /absolute/external/build/qb_import_scan \
  --firmware 13.20 /absolute/private/path/module.elf \
  > /absolute/private/path/imports.json
```

On Windows use the corresponding `.exe` scanner path. The scanner runs on the host;
it is not a PS5 runtime dependency. The wrapper passes a single bounded snapshot
to its stdin, so the reported SHA256 identifies exactly the bytes scanned.
The tool never executes the input, contacts a console or modifies source files.

Do not commit firmware modules, game assets or private metadata. SELF containers
and encrypted input are rejected. No decryption or extraction operation is included.
`--firmware` is an optional declaration (default `unknown`), not firmware detection.

## Report meaning

- `scope` is `relocation_referenced_imports`, not a complete import/call-graph audit.
- Standard ELF pointer relocations of types 1, 6 and 7 referencing undefined symbols
  are preserved. Other relocation types and unreferenced symbols are not inventoried.
- Every reference is retained; one symbol can have multiple relocation destinations.
- `target_vaddr` is an ELF virtual address relative to that image's layout. It is not
  a live process address, reusable firmware offset or an instruction to patch memory.
- `addend_bits` preserves the raw 64-bit addend; no relocation is applied.
- `segment_declares_write` is an ELF flag, not actual runtime writability. RELRO or
  loader policy can still protect a slot.
- Encoded identities preserve separate library and module IDs. Unknown qualifiers
  or absent declarations remain unresolved.
- `symbol_candidates` contains exact-name or candidate-name-hash matches from the
  public-source catalog. Provider mismatch is retained explicitly. A hash match does
  not supply a function signature, prove an import executes, or verify callability.
- `callability_verified`, `interposition_verified`, `hardware_verified` and
  `runtime_bindings_enabled` remain false; game compatibility is not demonstrated.
- `input_sha256` identifies file bytes; `interface_catalog_sha256` identifies the
  candidate catalog. Neither identifies an unseen live module or its authenticity.

Supported profile: ELF64 little-endian x86-64, executable/shared/SCE object types
recognized in the public SDK, standard dynamic-table VA tags in PT_LOAD or
PT_SCE_RELRO mappings, SysV or GNU hash, 24-byte symbols and RELA entries.
Canonical ELF64 widths are used if entry-size tags are absent, as in the SDK probe.
Split SCE metadata layouts with other tag/address conventions are unsupported.
Missing tags, ambiguous mapping, malformed indexes/strings and partial table overlap
fail closed. No firmware-specific process structure offsets are used.

Limits: 64 MiB input, 256 program headers, 65536 dynamic entries, 262144 symbols,
131072 relocation records and 256-byte names. The Node wrapper also bounds native
output to 32 MiB and scan time to 10 seconds. Exit 0 means metadata parsing succeeded;
exit 2 means invalid/unsupported input or execution error. No compatible-game status
is produced on either path.

## Reproduce synthetic end-to-end classification

The native test helper can emit an invented metadata-only ELF. It contains no
executable instructions, Sony structures, game data or console observations:

```sh
/absolute/external/build/qb_import_tests --write-synthetic-elf /absolute/external/synthetic.elf
node tools/diagnostics/ps5-imports.mjs \
  --scanner /absolute/external/build/qb_import_scan \
  --firmware 13.20 /absolute/external/synthetic.elf
```

Expected: one candidate `sceHmd2Initialize` identity, two relocation references,
false hardware/runtime flags. This does not test Sony's function or prove a PS5 hook.
