# Raylib Backend

This directory contains the default raylib/OpenGL implementation of the backend-neutral native services:

- `RaylibAppWindow` handles window, monitor, mode, timing, VSync, and icon behavior.
- `RaylibInputProvider` maps raylib keyboard/mouse/window state to shared input.
- `RaylibRenderer` implements textures, fonts, atlas/quad drawing, shapes, scissors, render targets, and RGBA readback.
- `RaylibFramePipeline` renders the scene, FPS display, OpenGL MSAA resolve, and GLSL post-processing at the exact selected resolution before final window scaling.
- `RaylibAudioBackend` and `RaylibCaptureService` provide audio and capture services.
- `RaylibGame.cpp` assembles these services for the shared `Game` constructor.

Raylib remains the default Windows/Linux backend and the Windows x86 backend. Shared scenes/gameplay do not include raylib headers or own raw raylib resources; those types stay inside this directory.
