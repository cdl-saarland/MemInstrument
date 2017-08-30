#!/usr/bin/env python

# This script checks whether there is a line in stdin that contains ALL command
# line arguments. Returns 0 if there is one, 1 otherwise

import sys

for line in sys.stdin:
    l = line.split("; ")
    missing = False
    for s in sys.argv[1:]:
        if not s in l:
            missing = True
            break
    if not missing:
        sys.exit(0)

sys.exit(1)

