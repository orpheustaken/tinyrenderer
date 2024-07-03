#!/bin/sh

make && ./src/build/main ./obj/head.obj

xdg-open img_output.tga

