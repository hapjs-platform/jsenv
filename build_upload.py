#!/usr/bin/env python

# Copyright (c) 2021, the hapjs-platform Project Contributors
# SPDX-License-Identifier: EPL-1.0

import os
import sys
import optparse
import platform

script_dir = os.path.abspath(os.path.dirname(__file__))

def get_host_os():
  os = platform.system()
  if os == 'Linux': return 'linux'
  if os == 'windows': return 'win'
  if os == 'Mac': return 'mac'
  return None

host_os = get_host_os()
host_cpu = "x64"


def parse_args(argv):
  parser = optparse.OptionParser()
  parser.add_option("-v", "--version", dest="version", type="string", help="repo user password")
  parser.add_option("-t", "--test", dest="test", action="store_true", default=False, help="build for test aar")
  parser.add_option("-d", "--debug", dest="debug", action="store_true", default=False, help="build with debug mode")
  parser.add_option("-r", "--release", dest="release", action="store_true", default=False, help="build for release aar")
  parser.add_option("-l", "--upload", dest="upload", action="store_true", default=False, help="upload the achives")
  options, args = parser.parse_args(argv)

  return options

GN_ARGS_TEMPLATE="""
is_debug=%(is_debug)s
target_cpu="%(target_cpu)s"
target_os="%(target_os)s"
host_os="%(host_os)s"
host_cpu="%(host_cpu)s"
symbol_level=0
use_no_v8symbols=%(use_no_v8symbols)s
jsenv_version_code="%(version)s"
is_remote_repo=true
"""

gn = os.path.join(script_dir, "buildtools", "%s64"%host_os, "gn")
ninja = "ninja"
out_gn = "out.gn"

def build_aar(target_os, target_cpu, is_debug, use_no_v8symbols, version, upload):
  arg_options = {
    "is_debug" : is_debug and 'true' or 'false',
    "target_cpu": target_cpu,
    "target_os" : target_os,
    "host_os" : host_os,
    "host_cpu" : host_cpu,
    "version": version,
    "use_no_v8symbols": use_no_v8symbols and 'true' or 'false'
  }
  gn_args = GN_ARGS_TEMPLATE % arg_options
  cmds = []
  out_dir = "%s/%s.%s%s" % (out_gn, target_cpu, is_debug and 'debug' or 'release', use_no_v8symbols and '.nov8symbols' or '')
  cmds.append("%s gen %s --args='%s'" % (gn, out_dir, gn_args))
  cmds.append("%s -C %s jsenv_aar" % (ninja, out_dir))
  #if upload:
  #  cmds.append("%s -C %s jsenv_aar_upload" % (ninja, out_dir))

  print("\n".join(cmds))
  os.system(" && ".join(cmds))

def build_all_aars(is_debug, version, upload):
  build_aar("android", "arm", is_debug, False, version, False)
  build_aar("android", "arm64", is_debug, False, version, upload)
  build_aar("android", "arm", is_debug, True, version, False)
  build_aar("android", "arm64", is_debug, True, version, upload)

def main(argv):
  args = parse_args(argv)
  is_debug = args.debug
  for_test = args.test
  for_release = args.release
  need_upload = args.upload
  version = args.version

  if not for_test and not for_release:
    for_test = True

  if for_test:
    build_all_aars(is_debug, "%s-SNAPSHOT" % version, need_upload)
  else:
    build_all_aars(is_debug, version, need_upload)

if __name__ == '__main__':
  main(sys.argv[1:])
