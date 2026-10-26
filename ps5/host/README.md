# PS5 SDK probe
Source scaffold only. No PS5 ELF compiled or executed here.
Selected interface: ps5-payload-dev/sdk commit a7685ce8e97ce1f8a4f38253a41d54241c9124f1; its samples/hello_world/Makefile includes toolchain/prospero.mk and its example uses C main.
This payload only prints a message. It is not the tracking receiver and exposes no network service.
On a POSIX environment with SDK prerequisites installed:
```sh
export PS5_PAYLOAD_SDK=/absolute/path/to/installed/ps5-payload-sdk
make -C ps5/host
```
The build output should be questbridge-probe.elf. No deployment target is included. Confirm the specific loader/firmware path and preserve stdout logs before claiming execution.
WSL and the required SDK/Clang/lld are absent on this workstation. Native Windows GCC cannot substitute for the PS5 SDK.
Next: provision the supported toolchain, build this harmless probe, then design a native authenticated tracking receiver using reviewed SDK sockets and crypto interfaces. No Sony VR module ABI is assumed.
