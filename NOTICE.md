Original QuestBridge code is GPL-3.0-or-later.
Research JSON files contain public GitHub repository metadata, not vendored implementations.
Locally downloaded upstream READMEs/sample files are ignored by Git and retain upstream licensing.
The PS5 SDK build recipe follows the public documented prospero.mk interface; no Sony code is bundled.
PSVR2Toolkit metadata and README licensing statements require clarification before any code reuse.
Development tools under .tools are untracked and governed by their own licenses.
Native transport uses OpenSSL 3 (Apache-2.0); host builds link the system library.
PS5 compilation uses separately provisioned static OpenSSL and LLVM C++ runtimes;
retain their notices and corresponding sources when distributing built artifacts.
See docs/dependencies/NATIVE_TOOLCHAIN.md for exact source revisions and evidence.
