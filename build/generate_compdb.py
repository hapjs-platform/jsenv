#!/usr/bin/python3
# Copyright 2013 The Flutter Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import argparse
import subprocess
import sys
import os
import pathlib


def main(argv):
    # Set the Ninja output directory
    output_directory = argv[1]
    print(output_directory)

    # Set the source directory
    source_directory = pathlib.Path(os.path.abspath(__file__)).parents[1]
    print(source_directory)

    # Compose database generation command
    generate_database_command = [
        'third_party/depot_tools/ninja',
        '-C',
        output_directory,
        '-t',
        'compdb',
        'cc',
        'cxx',
        'objc',
        'objcxx',
        'asm',
    ]

    print(generate_database_command)

    try:
        return_value = subprocess.run(generate_database_command, stdout=subprocess.PIPE, cwd=source_directory)
    except subprocess.CalledProcessError as exception:
        print("Failed to run Ninja: ", exception.returncode, exception.output)
        sys.exit(1)
    compile_commands = open('{}/compile_commands.json'.format(output_directory), 'w+')
    compile_commands.write(return_value.stdout.decode())
    compile_commands.close()


if __name__ == '__main__':
    print('Generate compilation database')
    sys.exit(main(sys.argv))
