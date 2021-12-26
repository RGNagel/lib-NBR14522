#!/usr/bin/bash

find ./src ./include ./app ./tests \
-type f \
-regextype egrep -regex ".*.(c|cpp|h|hpp)" \
-exec clang-format -i {} \;