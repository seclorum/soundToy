cmake_minimum_required(VERSION 3.15)
project(soundtoy NONE)
include("${CMAKE_CURRENT_LIST_DIR}/../osdk_cmake/OSDKSample.cmake")
osdk_add_sample(NAME STOY ADDRESS "$550" SOURCES toy.c soundtoy.s picture.s qrandom.s attrsyn.c)
