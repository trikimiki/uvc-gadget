> This is a (yet another) fork of [Freedesktop UVC gadget library](https://gitlab.freedesktop.org/camera/uvc-gadget).
>
> Original library cannot utilize `libcamera` advanced camera control parameters, such as AWB, Autofocus and etc modes - this heavily limits customization options when using RPi with Camera Module as an Plug-and-play OTG webcam.
>
> The goal is to add parameters to `uvc-gadget` binary that will pass those control options to `libcamera`.
>
> I am not an expert with development in this use case (C, UVC cameras, RPi), so expect hacky solutions - I did this for personal usage.
>
> Changes are tested with RPi Zero 2 W, Camera Module 3 (IMX708, basic module with no IR or wide lense), Pi OS Bookworm, and libcamera v0.5.2+99-bfd68f78.
>
> You can find [full guide here](https://www.raspberrypi.com/tutorials/plug-and-play-raspberry-pi-usb-webcam/), with specific [systemd fixes required for Bookworm here](https://telegra.ph/Pi-Camera-Webcam-Thing-Something-Something-Fix-01-02) (thank you [f4mi](https://www.youtube.com/@f4micom) for [inspiration](https://youtu.be/K1T1eMyPIC4) and fixes!).

# uvcgadget - UVC gadget C library

uvcgadget is a pure C library that implements handling of UVC gadget functions.

## Utilities

- uvc-gadget - Sample test application

## Build instructions:

To compile:

```
$ meson build
$ ninja -C build
```

## Cross compiling instructions:

Cross compilation can be managed by meson. Please read the directions at
https://mesonbuild.com/Cross-compilation.html for detailed guidance on using
meson.

In brief summary:
```
$ meson build --cross <meson cross file>
$ ninja -C build
```
