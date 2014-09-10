#!/bin/sh
find . -maxdepth 1 -name '*.c' -or -name '*.cpp' -or -name '*.h'  | xargs wc -l

find ./SHA1/ -maxdepth 1 -name '*.c' -or -name '*.cpp' -or -name '*.h'  | xargs wc -l
