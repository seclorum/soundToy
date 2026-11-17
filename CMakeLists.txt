cmake_minimum_required(VERSION 3.15)
project(soundtoy NONE)
include("${CMAKE_CURRENT_LIST_DIR}/OSDKStoy.cmake")
# picture.s (8 KB OverlayLabel) and qrandom.s are unreferenced since the
# 1.1 rewrite - link65 pulls in every listed input, so they stay OUT.
osdk_add_stoy(NAME STOY ADDRESS "$600" SOURCES toy.c soundtoy.s shimmer.s keyboard.s attrsyn.c)
