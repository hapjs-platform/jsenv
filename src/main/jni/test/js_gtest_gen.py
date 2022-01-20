#!/usr/bin/env python

# Copyright (c) 2021, the hapjs-platform Project Contributors
# SPDX-License-Identifier: EPL-1.0

import sys
import os
import re

def Usage():
  print("usage %s -out-file <out-file.cc> [file1.js ...]" % sys.argv[0])

out_file = ''
js_files = []

def ParseArgs():
  global out_file, js_files
  i = 1
  while i < len(sys.argv):
    if sys.argv[i] == '-out-file':
      out_file = sys.argv[i+1]
      i = i + 1
    else:
      js_files.append(sys.argv[i])
    i = i + 1

def ReadFile(filename):
  f = open(filename)
  content = f.read()
  f.close()
  return content

# $TEST(CaseName, TestName)$
def ParseJsFile(test_cases, js_file):
  pattern = '\$TEST\(([\w\d_]+),\s*([\w\d_]+)\)\$'
  try:
    fp = open(js_file, 'r')
    test_case = None
    lineno = 0

    for line in fp.readlines():
      r = re.match(pattern, line)
      if r:
        if test_case:
          test_cases.append(test_case)
        test_case = {
          'case_name': r.group(1),
          'test_name': r.group(2),
          'lineno': lineno,
          'js_file': os.path.basename(js_file),
          'lines' : []}
      else:
        if test_case:
          test_case['lines'].append(line)
      lineno = lineno + 1

    if test_case:
      test_cases.append(test_case)
    fp.close()
  except:
    print("ERROR cannot open file " + js_file)


TEMPLATE_TEST="""
namespace %(case_name)s_%(test_name)s {

static const char js_code_str[] = R"Jscode(
%(js_source)s
)Jscode";

static ::hybrid::TestInfo js_test_info = {
  "%(case_name)s",
  "%(test_name)s",
  "%(js_file)s",
  js_code_str,
  sizeof(js_code_str) - 1,
  %(lineno)d
};

static ::testing::TestInfo* test_case =
    ::hybrid::MakeAndRegisterJSTestInfo(&js_test_info);

}  // namespace %(case_name)s_%(test_name)s

"""

TEMPLATE_HEADER = """
// Copyright 2018 Xiaomi, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "test_help.h"

"""

def OutJsCase(test_case, fp):
  js_source = ''.join(test_case['lines'])
  test_case['js_source'] = js_source

  fp.write(TEMPLATE_TEST % test_case)


def OutFiles(test_cases):
  fp = open(out_file, 'w')
  fp.write(TEMPLATE_HEADER)
  for t in test_cases:
    OutJsCase(t, fp)
  fp.close()

def main():
  ParseArgs()
  test_cases = []
  for js_file in js_files:
    ParseJsFile(test_cases, js_file)
  OutFiles(test_cases)


if __name__ == '__main__':
  Usage()
  main()

