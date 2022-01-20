#!/usr/bin/env python

# Copyright (c) 2021, the hapjs-platform Project Contributors
# SPDX-License-Identifier: EPL-1.0

import os
import subprocess
import sys
from optparse import OptionParser

OUT_DIR = ''
JOBS_NUM = 4

def init_gn_make():
  py_dir = os.path.split(os.path.abspath(__file__))[0]
  project_dir = os.path.dirname(py_dir)
  os.chdir(project_dir)
  gn_path = os.path.join(project_dir, 'buildtools/linux64/')
  # TODO(lijianbin3): set in envsetup.sh
  os.environ['PATH'] += ':' + gn_path


def gen_args(out_dir):
  gn_args = ['host_os = "linux"',
             'host_cpu = "x64"',
             'target_os = "linux"',
             'target_cpu = "x64"',
             'is_gcov = true',
             'is_debug = true']
  gn_args_str = ' '.join(gn_args)
  cmd = ' '.join(['gn gen', out_dir, '--args=\'', gn_args_str,'\''])
  status = exec_cmd(cmd, True)
  if status != 0 :
    sys.exit()


def build_project(out_dir, jobs_num):
  cmd = ' '.join(['ninja', '-C', out_dir, "-j", str(jobs_num)])
  status = exec_cmd(cmd, True)
  if status != 0 :
    sys.exit()


def exec_cmd(cmd, shell_flag):
  status = subprocess.call(cmd, shell = shell_flag)
  if status != 0 :
    print('error:' + cmd)
  return status


def gn_make_main(out_dir='out/x64.debug', jobs_num=4):
  init_gn_make()
  gen_args(out_dir)
  build_project(out_dir, jobs_num)
  return True


def init_option():
  global OUT_DIR, JOBS_NUM
  parse = OptionParser(usage='usage:%prog [options] arg1',
                       version='%prog version: 1.0')
  parse.add_option('-o', '--out_dir', metavar='out_dir', dest='out_dir',
                   action='store', type=str, default='out/x64.debug',
                   help='Enter out_dir name, such as out/x64.debug')
  parse.add_option('-v', action='version',  help='Gn_make Version')
  parse.add_option('-j', '--jobs', metavar='jobs_num', dest='jobs_num',
                   action='store', type=int, default=4,
                   help='Ninja run N jobs in parallel, such as -j 4')
  (options,args) = parse.parse_args()
  OUT_DIR = options.out_dir
  JOBS_NUM = options.jobs_num


# main_entry
if __name__ == '__main__':
  init_option()
  gn_make_main(OUT_DIR, JOBS_NUM)
