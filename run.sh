#!/bin/sh

make clean
make

./main

xdg-open output.tga

