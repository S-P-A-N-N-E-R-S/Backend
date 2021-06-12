#!/bin/bash

# execute clang-format on all files
find src include apps \( -iname *.hpp -o -iname *.cpp \) | xargs clang-format -i -style=file --verbose
