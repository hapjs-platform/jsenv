#!/usr/bin/env python

# Copyright (c) 2021, the hapjs-platform Project Contributors
# SPDX-License-Identifier: EPL-1.0

import os
import sys
import subprocess

def  exec_eslint_lint_on_target():
  old_dir = os.getcwd()
  eslint_dir = os.path.dirname(os.path.realpath(__file__))
  print 'check current dir: %s, %s'%(old_dir, eslint_dir)

  # check if in eslint_dir, node_module has installed or not.
  os.chdir(eslint_dir);
  if (not os.path.isdir('./node_modules')):
    npmprocess = subprocess.Popen(['npm', 'install', '--registry=http://registry.npm.pt.mi.com'])
    npmprocess.wait()
  os.chdir(old_dir)

  # lint each js source code directory
  for index in range(len(sys.argv)):
    if (index >= 1):
      #print 'check sys.argv[%d]: %s'%(index, sys.argv[index])
      jsfile_dir = os.path.realpath(sys.argv[index])
      print 'Linting Javascript Code on Dir: %s'%jsfile_dir
      lint_process = subprocess.Popen([
        eslint_dir + '/node_modules/.bin/eslint',
        '--config ',
        eslint_dir + '/.eslintrc.json',
        '--ignore-path',
        jsfile_dir + '/.eslintignore',
        jsfile_dir])
      #print lint_process.returncode

  return True

if __name__ == '__main__':
  exec_eslint_lint_on_target()

