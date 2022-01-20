#!/usr/bin/env python

# Copyright (c) 2021, the hapjs-platform Project Contributors
# SPDX-License-Identifier: EPL-1.0

import sys
import os
import subprocess
import argparse

def which(bin):
    child = subprocess.Popen([ 'which', bin ],
                             stdout = subprocess.PIPE,
                             stderr = subprocess.PIPE)
    stdout, stderr = child.communicate()
    code = child.poll()
    if code != 0:
        return ''
    return stdout.strip()



def main(argv):
    parser = argparse.ArgumentParser('bin_path')
    parser.add_argument('bin')
    options = parser.parse_args(argv)

    child = subprocess.Popen([ 'which', options.bin ],
                             stdout = subprocess.PIPE)
    stdout, stderr = child.communicate()
    code = child.poll()
    if code != 0:
        return

    path = stdout.strip()
    print os.path.realpath(path)

if __name__ == '__main__':
   main(sys.argv[1:])
