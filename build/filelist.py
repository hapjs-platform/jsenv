#!/usr/bin/env python

# Copyright (c) 2021, the hapjs-platform Project Contributors
# SPDX-License-Identifier: EPL-1.0

import sys
import os

if len(sys.argv) <= 1:
  print("usage %s [output_file] [file1...]" % sys.argv[0])
  sys.exit(0)

output = sys.argv[1]

output_dir = os.path.dirname(output)
try:
  os.makedirs(output_dir)
except:
  pass

f = open(output, "wt")
for filename in sys.argv[2:]:
  f.write("%s\n" % filename)

f.close()
