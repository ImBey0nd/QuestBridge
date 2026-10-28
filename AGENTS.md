# QuestBridge engineering instructions
Keep native PS5 execution as the objective. Target a reusable layer for any PS VR2 game; no specific game installation is a development prerequisite.
Owner-confirmed environment (2026-10-27): PS5 Fat, firmware 13.20, Kstuff Lite 1.11 Beta, latest Relapse, Quest 2 targeting latest system software, original Quest 2 Touch controllers, no physical PS VR2, Resident Evil Village not installed. Do not re-request this inventory. Pin public source revisions independently; ask only for a specific missing observation when a concrete experiment requires it.
Networking or synthetic video is never evidence of game compatibility.
Do not invent Sony exports, ABIs, offsets, title IDs, firmware support, or hardware results.
Record source URLs and immutable commits; label upstream claims, hypotheses, compilation, and hardware tests separately.
Unknown firmware disables all PS VR2 integration. No automatic patches, kernel writes, exploit execution, or console deployment.
Keep common models device neutral: meters, right handed, +Y up, -Z forward, quaternion xyzw.
Network services bind loopback by default. Require a random pre-shared key; never commit keys or console addresses.
Validate network lengths, values, authentication, sessions, sequence and allocation bounds before consuming data.
Use C++17/CMake for native components. Node built-ins are the desktop experiment harness, not a PS5 runtime dependency.
Run npm test and native CTest after relevant changes. Update docs/testing/SESSION.md with actual results.
No proprietary binaries, keys, SDK content, firmware dumps, or game assets. Review upstream licensing before importing code.
For a new or cloud session, read docs/HANDOFF.md and docs/PROJECT_BRIEF.md first, then update the handoff and session record when completing a milestone.
