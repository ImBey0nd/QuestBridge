# Is QuestBridge doable?

Assessment after the 2026-10-26 cloud build: **the headset app and communication parts are achievable; replacing PS VR2 for native PS5 VR games is still unproven.** Continuing as a research project makes sense. Promising a working replacement, a completion date or broad game support does not yet have an evidence basis.

## What we have proved

The desktop programs exchange authenticated tracking, controller and tiny synthetic stereo packets. Their automated checks pass in Linux. Shared C++ code also builds and passes its tests. We have now produced a real, debug-signed Android APK containing the OpenXR probe and loader. A clean Android build and lint run pass; the APK's signature and packaged ARM64 libraries can be checked without a headset.

An APK is the headset's installable app file. Building one proves that the code can be turned into an Android application. It does not prove the headset will accept its OpenXR calls. This probe only asks whether an OpenXR runtime and headset system are available. It does not yet read movement, draw a VR scene or connect to a PS5.

## The difficult part

Think of the Quest as a screen plus movement sensors, and the PS5 as the machine running the game. Sending information between those devices is only one part of the job. A native PS5 VR game expects Sony's headset, controllers and VR system services to behave in specific ways. QuestBridge must make the game receive usable movement/input and get the game's separate left/right eye images out to the Quest at the right time.

We have not verified a usable PS5 interface for doing those things on the requested firmware 13.20. The existing research records public leads, but no tested detection replacement, controller/tracking interface or native stereo capture path. A jailbreak or homebrew loader by itself does not demonstrate that those VR services can be replaced. This uncertainty could be solvable with further research, or it could block the original goal.

Even if access is possible, comfortable VR will require low-delay video encoding, network transport and display timing. Eye tracking and headset haptics unavailable on Quest 2 also need explicit limits; we cannot assume every game will tolerate missing features. The current tiny synthetic images and loopback timing do not measure any of this.

## What would change the assessment

| Milestone | What it would prove | What remains afterward |
| --- | --- | --- |
| Run this probe on Quest 2 and capture its result | The app can discover that device's OpenXR runtime/system | Sessions, movement, controllers and rendering |
| Implement and test a diagnostic scene with tracked hands/head | A real Quest client is working | PS5 VR integration and video transport |
| Compile and separately test a minimal user-space PS5 probe | The selected SDK/loader works on the actual setup | All VR-specific interfaces |
| Obtain justified interface evidence for VR initialization, tracking/input and stereo output on firmware 13.20 | Whether a practical PS5 integration path exists | Implementation, timing and individual game tests |
| Demonstrate one native game's VR initialization, stereo image and pose/controller response | A first supported game/configuration works | Reliability and wider compatibility |

The next device experiment is the small Quest runtime probe. The most consequential research priority is the PS5 VR interface evidence: it should guide investment in a full streaming client. Cloud work can continue building and testing independent components, but only hardware observations can settle the central compatibility question.

No Quest/headset, console or game was contacted in this cloud session. Unknown firmware remains disabled, and no exploit, memory patch or console deployment was performed. See [research](research/RESEARCH.md), [hardware evidence procedure](testing/HARDWARE.md) and [cloud build results](testing/CLOUD_2026-10-26.md).
