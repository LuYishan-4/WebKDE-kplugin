This project is the direct successor to[Web_Cursor](https://github.com/LuYishan-4/Animated_UltralightWeb_Cursor) . During the development of the OpenGL renderer, it became clear that the original scope was disproportionate to the actual development workload. Moving forward, this application will inherit the core capabilities of its predecessor while sharpening its focus exclusively on KWin

# Why use OpenGL ES

## Overview
This project is migrating its rendering backend to **OpenGL ES (GLES)**. This transition is motivated by the upcoming deprecation of Desktop OpenGL support in the KWin window manager (specifically targeting Plasma 6.8 and future releases).

## Why the Change?
As KWin shifts its focus away from Desktop OpenGL, ensuring compatibility with the future compositor architecture requires our project to standardize on OpenGL ES. This change ensures:
* **Future-Proofing:** Compatibility with the evolving KWin environment.
* **Consistency:** Aligned rendering paths across supported Linux desktop platforms.
* **Efficiency:** Leveraging the optimized GLES pipeline for improved performance and stability within modern Wayland-centric desktop sessions.

