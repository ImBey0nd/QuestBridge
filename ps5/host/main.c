/* SPDX-License-Identifier: GPL-3.0-or-later
 * SDK compilation probe, NOT a PS VR2 host or tracking receiver.
 * The pinned SDK CRT performs kernel writes before main. Build-only until
 * startup is replaced/audited against the project's no-kernel-write rule.
 * Entry point follows pinned ps5-payload-dev/sdk samples/hello_world.
 */
#include <stdio.h>
int main(void) {
 puts("QuestBridge SDK probe: main reached; no VR integration.");
 return 0;
}
