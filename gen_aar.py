#!/usr/bin/env python

# Copyright (c) 2021, the hapjs-platform Project Contributors
# SPDX-License-Identifier: EPL-1.0

import os
import sys
import shutil
import re

configs = {}

dir_keys = [
  'android-home',
  'android-ndk-home',
  'gradle-dir',
  'gen-dir',
  'native-lib-dir',
  #'external-java-source-dir',
]

jsenv_version_keys = [
  'is-remote-repo',
  'jsenv-version-code',
  'output-aar',
]

def parse_args(argv):
  count = len(argv)
  i = 0
  configs['debug'] = False
  configs['upload'] = False
  while i < count:
    arg = argv[i]
    if arg[:2] == '--':
      key = arg[2:]
      if key in dir_keys:
        configs[key] = os.path.abspath(argv[i + 1])
        i = i + 1
      elif key in jsenv_version_keys:
        configs[key] = argv[i + 1]
        i = i + 1
      elif key == 'native-libs':
        configs[key] = argv[i + 1:]
        break
      elif key == 'debug':
        configs[key] = True
      elif key == 'abi':
        configs['abi'] = argv[i + 1]
        i = i + 1
      elif key == 'upload':
        configs[key] = True
    i = i + 1


def cp_so(so_file_name, src_dir, dst_dir):
  so_file = os.path.join(src_dir, so_file_name)
  if os.path.exists(so_file):
    dst_so_file = os.path.join(dst_dir, 'jniLibs', configs['abi'], so_file_name)
    print('cp %s %s' % (so_file, dst_so_file))
    shutil.copy(so_file, dst_so_file)
    return True
  else:
    return False


def create_jniLibs():
  try:
    os.makedirs(os.path.join(configs['gen-dir'], 'jniLibs', configs['abi']))
  except:
    pass
  for lib in configs['native-libs']:
    m = re.match('lib.+\.so', lib)
    if m:
      cp_so(lib, configs['native-lib-dir'], configs['gen-dir'])


def build_aar():
  cmds = ['export', 'ANDROID_HOME="%s"' % configs['android-home']]
  cmds.append('&&')
  cmds.append('export ANDROID_NDK_HOME="%s" &&' % configs['android-ndk-home'])
  cmds.append('./gradlew clean')
  cmds.append('-PjsenvJniLibsDir="%s"' % configs['gen-dir'])
  #cmds.append('-Pexternal_java_source_dir="%s"'  % configs['external-java-source-dir'])
  aar_name = ''
  output_aar_name = os.path.basename(configs['gradle-dir'])
  if configs['debug']:
    cmds.append('assembleDebug')
    aar_name = '%s-debug.aar' % output_aar_name
  else:
    cmds.append('assembleRelease')
    aar_name = '%s-release.aar' % output_aar_name

  cmds.append('&& cp build/outputs/aar/%s %s' % (aar_name, os.path.abspath(configs['output-aar'])))

  os.chdir(configs['gradle-dir'])
  command_line = ' '.join(cmds)
  print(command_line)
  os.system(command_line)


def upload_aar():
  cmds = ['export', 'ANDROID_HOME="%s"' % configs['android-home']]
  cmds.append('&&')
  cmds.append('./gradlew uploadArchives')
  cmds.append('-PjsenvJniLibsDir="%s"' % configs['gen-dir'])
  cmds.append('-PjsenvOutputAarName="%s"' % os.path.splitext(os.path.basename(configs['output-aar']))[0])
  cmds.append('-PjsenvVersionCode="%s"' % configs['jsenv-version-code'])
  #cmds.append('-Pexternal_java_source_dir="%s"'  % configs['external-java-source-dir'])
  cmds.append('-PisRemoteRepo="%s"' % configs['is-remote-repo'])
  os.chdir(configs['gradle-dir'])
  command_line = ' '.join(cmds)
  print(command_line)
  os.system(command_line)


def dealwith_aar():
  if (configs['upload']):
    upload_aar()
    return
  create_jniLibs()
  build_aar()


def main():
  parse_args(sys.argv[1:])
  dealwith_aar()


if __name__ == '__main__':
  main()

