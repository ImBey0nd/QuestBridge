# Offline PS5 environment intake

This tool checks whether the environment information needed for source review has
been supplied. It does not contact the console, inspect a module binary, verify
the truth of a declaration, choose an ABI, or authorize execution.

Copy [PS5_ENVIRONMENT_TEMPLATE.json](PS5_ENVIRONMENT_TEMPLATE.json) to an ignored
local folder such as `artifacts/environment.json`. Keep unknown fields `null`.
Each supplied field has exactly three properties:

```json
{
  "value": "the identity or observation actually recorded",
  "origin": "existing_log",
  "reference": "local evidence label and relevant line or capture time"
}
```

Allowed origins are `owner_report`, `system_screen`, `existing_log`,
`local_metadata` and `public_reference`. The origin is self-declared, not verified
by this tool. Public references do not fill gaps about the installed environment.
The template intentionally contains no prefilled owner hardware claims.

| Field | Required information |
| --- | --- |
| model_revision | Console revision, excluding serial number |
| firmware_full_build | Full version/build displayed by the console |
| relapse_identity | Installed version or commit, if already known |
| kstuff_identity | Installed version/build, if already known |
| loader_identity | Actual loader name and installed build identity |
| loader_source | Source URL and immutable commit associated with that loader, if known |
| loader_stdout | Whether existing loader logs are available; record unavailable explicitly if so |
| game_identity | Verified region/title ID, installed version and VR Mode state |
| module_metadata | Existing authorized metadata, including representation and provenance; report unavailable if absent |

```sh
node tools/diagnostics/ps5-environment.mjs artifacts/environment.json
```

The summary lists missing fields and declared origins without copying their values
or references. `awaiting_source_review` means every field has a declaration, which
may itself say unavailable; a reviewer must still establish whether the evidence
is sufficient. No status enables deployment, runtime bindings or compatibility.
Incomplete valid records exit 0 and produce `missing_environment_evidence`;
malformed input exits 2. Input is UTF-8 JSON bounded to 64 KiB, with 1024-character
single-line values/references and strict field names.

Keep the filled record local and review it before sharing. Output omission is not
a general log sanitizer. No credentials, console addresses, serial numbers or
proprietary binaries belong in the record or repository. Do not install, update,
extract, decrypt or execute anything to fill an unknown field. The text form in
[the owner's guide](../LO_QUE_TENGO_QUE_HACER_YO.md) remains sufficient; the owner
does not need to edit JSON. Existing PS5 artifacts remain build-only under
[HARDWARE_NEXT.md](HARDWARE_NEXT.md).
