#!/usr/bin/env python

# Copyright (c) 2021, the hapjs-platform Project Contributors
# SPDX-License-Identifier: EPL-1.0

import os
import re
import subprocess
import sys
from optparse import OptionParser

import gn_make

OUT_DIR = ''
JOBS_NUM = 4
LCOV_INFO = ''
XML_INFO = ''
LCOV_OUTPUT_DIR = ''
LOCAL_DIR = os.path.split(os.path.abspath(__file__))[0]
os.chdir(os.path.dirname(LOCAL_DIR))
PROJECT_DIR = os.getcwd()
LCOV_EXCLUDE_LIST = [
  '/usr/*',
  PROJECT_DIR + '/external/*'
]

def init_lcov_env():
  global OUT_DIR, LCOV_INFO, LCOV_OUTPUT_DIR, XML_INFO, JOBS_NUM
  parse = OptionParser(usage = 'usage:%prog [options] arg1', version =
                       '%prog version: 1.0')
  parse.add_option('-o', '--out_dir', metavar='out_dir', dest = 'out_dir',
                   action = 'store', type=str, default = 'out/x64.debug',
                   help = 'Enter out_dir name. such as out/x64.debug')
  parse.add_option('-l', '--lcov_info_name',metavar='lcov_info',
                   dest = 'lcov_info', type = str, default = 'lcov.info',
                   help = 'Enter lcov_info name')
  parse.add_option('-x', '--xml_info', metavar='xml_info',
                   dest = 'xml_info', type = str, default = 'coverage.xml',
                   help = 'Enter xml_info name')
  parse.add_option('-d', '--lcov_output_dir',metavar='lcov_output_dir',
                   dest = 'lcov_output_dir', type = str, default = 'lcov_out',
                   help = 'Enter lcov_output name')
  parse.add_option('-j', '--jobs', metavar='jobs_num', dest='jobs_num',
                   action='store', type=int, default=4,
                   help='Ninja run N jobs in parallel, such as -j 4')
  parse.add_option('-v', action = 'version', help = 'Gen_lcov Version')
  (options,args) = parse.parse_args()
  OUT_DIR = options.out_dir
  JOBS_NUM = options.jobs_num
  LCOV_INFO = options.lcov_info
  XML_INFO = options.xml_info
  LCOV_OUTPUT_DIR = options.lcov_output_dir

  # TODO(lijianbin3): set in envsetup.sh
  lcov_path = os.path.join(PROJECT_DIR, 'buildtools/linux64/lcov/bin')
  gcovr_path = os.path.join(PROJECT_DIR, 'buildtools/linux64/gcovr/bin')
  os.environ['PATH'] += ':' + lcov_path
  os.environ['PATH'] += ':' + gcovr_path
  os.environ['LD_LIBRARY_PATH'] = os.path.join(PROJECT_DIR, OUT_DIR)


def get_src_dir(gcno_dir):
  tmp_path = os.path.split(gcno_dir)[0]
  src_dir = os.path.split(tmp_path)[0]
  src_dir = src_dir.replace(os.path.join(OUT_DIR, 'obj/'), '')
  return src_dir


def find_file(file_name, folder, is_extension = False):
  file_list = []
  for (root, dirs, files)in os.walk('.'):
    for f in files:
      if is_extension :
        if  os.path.splitext(f)[1] == file_name:
          abs_path = os.path.abspath(os.path.join(root, f))
          file_list.append(abs_path)
      else :
        if f == file_name:
          abs_path = os.path.abspath(os.path.join(root, f))
          file_list.append(abs_path)
  return file_list;


def run_all_exe():
  result = True
  build_gn_list = find_file('BUILD.gn', PROJECT_DIR, is_extension = False)
  os.chdir(os.path.join(PROJECT_DIR, OUT_DIR))

  pattern = re.compile(r'jsenv_test\(\"(.+?)\"\)')

  while '' in build_gn_list:
    build_gn_list.remove('')

  exe_list = []
  for build_gn in build_gn_list:
    with open(build_gn, 'r') as f:
      file_content = f.read()
      exe = re.findall(pattern, file_content)
      if exe:
        exe_list.append(exe)
  while '' in exe_list:
    exe_list.remove('')
  for exe in exe_list:
    cmd = './' + ''.join(exe)
    print(cmd)
    status = gn_make.exec_cmd(cmd, True)
    if status != 0:
      result = False
  return result


def gen_gcov():
  result = True
  gcno_list = find_file('.gcno', os.path.join(PROJECT_DIR, OUT_DIR),
                        is_extension = True)
  print(gcno_list)
  while '' in gcno_list:
    gcno_list.remove('')

  for gcno_file in gcno_list:
    src_dir = get_src_dir(gcno_file)
    cmd = ' '.join(['gcov', src_dir, '-o', gcno_file])
    print(cmd)
    status = gn_make.exec_cmd(cmd, True)
    if status != 0 :
      result = False
      print('error: generate gcov error')
      sys.exit()
  return result


def generateExcludeDirs():
  exclude_dirs = ''
  for e_dir in LCOV_EXCLUDE_LIST:
    exclude_dirs +=  '--exclude "' + e_dir  + '" '
  return exclude_dirs


def gen_lcov(lcov_info='lcov.info'):
  exclude_args = generateExcludeDirs()
  cmd = ' '.join(['lcov', '-d .', '-o', lcov_info, '-b .', '-c', exclude_args])
  print (cmd)
  gn_make.exec_cmd(cmd, True)
  return lcov_info


def gen_html(lcov_info = 'lcov.info', output_dir='lcov_out'):
  result = True
  cmd = ' '.join(['genhtml', '-o', output_dir, lcov_info])
  print (cmd)
  status = gn_make.exec_cmd(cmd, True)
  if status != 0 :
    print('error: generate html')
    result = False
    sys.exit()
  return result


def compile_project():
  return gn_make.gn_make_main(OUT_DIR, JOBS_NUM)


def gen_xml(xml_info='coverage.xml'):
  result = True
  print os.path.join(PROJECT_DIR, OUT_DIR, xml_info)
  exclude_args = generateExcludeDirs()
  cmd = ' '.join(['gcovr', '-r', PROJECT_DIR, '-o', xml_info, '-x',
                  '--xml-pretty', exclude_args])
  print(cmd)
  status = gn_make.exec_cmd(cmd, True)
  if status != 0 :
    print('error: generate xml')
    result = False
    sys.exit()
  return result


def coverage_result_analysis(html_path):
  result = True
  print(html_path)
  with open(html_path, 'r') as f:
    file_content = f.read()
    pattern = re.compile(r'headerCovTableEntryHi\">(.+?)<')
    coverageScoreStr = re.findall(pattern, file_content)
  if len(coverageScoreStr) != 2:
    result = False
    print('error: parse index.html error in coverage_result_analysis: '
           ' '.join(coverageScoreStr))
    return result
  linesScore = float(''.join(coverageScoreStr[0]).strip('%'))
  functionScore = float(''.join(coverageScoreStr[1].strip('%')))
  if functionScore < 100 or linesScore < 100 :
    print('Coverage-Test error: Functions = {}, Lines = {}'.
            format(functionScore, linesScore))
  return result


def chown_out_dir():
  out_dir = os.path.join(PROJECT_DIR, OUT_DIR)
  os.chown(os.path.dirname(out_dir), 1000, 1000)


def gen_lcov_main():
  # init && gn_make
  init_lcov_env()
  status = compile_project()
  if status != True:
    print('error: compile error')
    sys.exit()

  # run all executable
  status = run_all_exe()
  if status != True:
    print('error: error in gtest exe')

  # gcov src_path -o target_path
  status = gen_gcov()
  if status != True:
    print('error: generate gcov error')
    sys.exit()

  # lcov -d . -o locv_name.info -b . -c --exclude "/usr/*"
  status = gen_lcov(LCOV_INFO)
  if status == '':
    print('error: generate lcov error')
    sys.exit()

  # genhtml -o lcov_out lcov.info
  status = gen_html(LCOV_INFO, LCOV_OUTPUT_DIR)
  if status != True:
    print('error: generate html error')
    sys.exit()

  # gcovr -r . -o coverage.xml -x --xml-pretty --exlcude "/usr/*"
  status = gen_xml(XML_INFO)
  if status != True:
    print('error: generate xml error')
    sys.exit()

  # analyze the coverage result
  html_path = os.path.join(PROJECT_DIR, OUT_DIR, LCOV_OUTPUT_DIR, 'index.html')
  status = coverage_result_analysis(html_path)
  if status != True:
    print('error: coverage result error')
    sys.exit()

  # jenkins need change the owner of the out_dir
  chown_out_dir()

# main_entry
if __name__ == '__main__':
  gen_lcov_main()
