# Overview

The goal of this assignment is to create an electron application that will interact with a native
module that uses the OBS project (https://github.com/obsproject) for its backend in order to
stream to Twitch.

Supported platforms: Windows 10 64-bit only

# Content
- Streamer - Twitch Streaming application base on Electron (electron-quick-start - https://github.com/electron/electron-quick-start)
- osb - OSB-Studio project (https://github.com/obsproject/obs-studio)
- obs-addon - node.js native plugin. It implements OBS pipeline initialization and streaming start/stop functionality
- TestApp - Windows test application for streaming to Twitch
- Readme.md - this file
- obs_win.patch - Patch to OBS with a couple fixes (details in the code)

# Prerequisites
- VS 2019
- QT 5.12+ - https://download.qt.io/archive/qt/
- CEF Wrapper 3770 (x64 - https://cdn-fastly.obsproject.com/downloads/cef_binary_75.1.16+g16a67c4+chromium-75.0.3770.100_windows64_minimal.zip)
- CMake 3.16+ - https://cmake.org/download/
- node.js - https://nodejs.org/en/download/current/
- cmake-js - 'npm install -g cmake-js'
- Vulkan SDK - https://vulkan.lunarg.com/sdk/home
- iTunes - https://support.apple.com/kb/DL2065?viewlocale=en_US&locale=en_US , required for CoreAudio AAC plugin
- vcpkg - https://github.com/microsoft/vcpkg
  - FFmpeg - 'vcpkg install ffmpeg[avcodec,avdevice,avfilter,avformat,avresample,core,gpl,postproc,swresample,swscale,x264]:x64-windows --recurse'
  - libcurl - 'vcpkg install curl:x64-windows'
  - vulkan - 'vcpkg install vulkan:x64-windows'
  - mbedTLS - 'vcpkg install mbedTLS:x64-windows'


# Build
