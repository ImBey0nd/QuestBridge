# Offline PS5VR log interpretation

This tool reads an existing UTF-8 text file. It never connects to a console,
loads Sony modules, patches a process or executes a payload. Supported log format:
[PS5VR at 209eb7f0160000507a0ca017842941b8e8e88f46](https://github.com/mtp-24/PS5VR/blob/209eb7f0160000507a0ca017842941b8e8e88f46/app/src/app_vr.c).
It is a diagnostic for that format, not a universal PS5 log parser.

```sh
node tools/diagnostics/ps5-vr-log.mjs tests/research/fixtures/ps5-vr-synthetic.log \
  --origin synthetic --firmware 13.20
```

The example is invented test data. Its first failure is tracker initialization;
a later submitted-frame marker does not change that. It is not hardware evidence.

For a previously captured, authorized local log, keep it outside the checkout:

```sh
node tools/diagnostics/ps5-vr-log.mjs /absolute/private/path/session.log \
  --origin console_capture --firmware 13.20 > /absolute/private/path/report.json
```

`--origin` defaults to `unverified`; `--firmware` defaults to `unknown`. Both
are user declarations, never proof. Input SHA256 identifies the text supplied,
not the console, binary, firmware, recording authenticity or capture completeness.
No console experiment is authorized or triggered by these commands.

## Interpret the report

- `events` contains recognized stages and raw status values. Handle checks only
  recognize nonnegative signed values; they do not prove a live device.
- `first_failure` preserves the first logged failure in an attempt.
- `missing_setup_stages` lists absent expected status messages.
- `setup_order_consistent` checks the order of recognized setup stages.
- `logged_setup_checks_passed` means all expected logged statuses/handles passed
  and no recognized failure, malformed relevant message or order conflict appeared.
  It does not mean initialization, device detection or frame acceptance was verified.
- `submission_markers` counts text markers only. The upstream marker is unconditional
  with respect to SetParam/flip errors; never report it as accepted frames.
- `device_status_raw` is an uninterpreted number. The logged GetDeviceInformation
  call's return code is absent, so the analyzer cannot validate this status.
- `hardware_verified` is always false. Tracking injection, stereo capture and game
  compatibility always remain `not_demonstrated`.

A new Hmd2 initialization starts another attempt after a prior initialization.
The tool cannot identify a missing restart marker, edited/interleaved captures,
unlogged failures or errors in other log formats. Keep each process/session log
separate and preserve its provenance. Unknown messages are not interpreted; only
malformed messages from the recognized stage families block the logged check flag.

Limits: 1 MiB, 10000 lines, 4096 characters per line, 128 attempts. CLI rejects
invalid UTF-8. Reports omit raw lines and console addresses; retain source logs
privately. Runtime failures in the log are reported in JSON; process exit 0 means
the file was parsed, not that setup succeeded. Invalid input/options exit 2.
