#!/usr/bin/bash
cd build
ctest -j6 -C Debug -T test --output-on-failure
cd -