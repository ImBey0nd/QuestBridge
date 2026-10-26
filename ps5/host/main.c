/* SPDX-License-Identifier: GPL-3.0-or-later
 * User-space SDK smoke test, NOT a PS VR2 host or tracking receiver.
 * Entry point follows pinned ps5-payload-dev/sdk samples/hello_world.
 */
#include <stdio.h>
int main(void) {
 puts("QuestBridge SDK probe: no VR integration, no memory patches.");
 return 0;
}
