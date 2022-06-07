#Copyright (c) 2022, the hapjs-platform Project Contributors
#SPDX-License-Identifier: EPL-1.0

import sys
import os
import commands
import optparse
import shutil
#import requests
import zipfile

GN_ARGS="""
is_debug=%(is_debug)s
target_cpu = "%(target_cpu)s"
target_os = "%(target_os)s"
is_component_build = false
symbol_level = 0
use_custom_libcxx = false
v8_android_log_stdout = true
v8_enable_i18n_support = false
v8_static_library = true
v8_use_external_startup_data = false
v8_imminent_deprecation_warnings = false
clang_use_chrome_plugins = false
"""

gn_args = [
  {
    'target_os': 'android',
    'cpus': [
      {
        'target_cpu': 'arm64'
      },
      {
        'target_cpu': 'arm'
      }
    ]
  },
  {
    'target_os': 'linux',
    'cpus': [
      {
        'target_cpu': 'x64'
      },
      {
        'target_cpu': 'x86'
      }
    ]
  }
]

def make_dirs(dirs):
  try:
    os.makedirs(dirs)
  except:
    pass


def parse_args(argv):
  parser = optparse.OptionParser()
  parser.add_option("-s", "--v8-source", dest="v8_source", type="string", help="url of v8 source")
  parser.add_option("-t", "--target", dest="target", action="append", choices=["android", "linux", "windows", "mac"])
  parser.add_option("-b", "--remote-branch", dest="remote_branch", action="store", default="remotes/origin/8.3", help="set remote-branch")

  options, args = parser.parse_args(argv)
  return options

def unzip(local_file, dest_dir):
  os.system("unzip -x %s -d %s" % (local_file, dest_dir))

def get_target_os(target):
  for g in gn_args:
    if g['target_os'] == target:
      return g
  return None

def install_result(out_dir, target, cpu, is_release, cwd_dir):
  target_dir = os.path.join(cwd_dir, "..", target, cpu, is_release and 'release' or 'debug')
  make_dirs(target_dir)
  # copy v8_static
  v8_static = os.path.join(out_dir, "obj", "libv8_static.a")
  shutil.copy(v8_static, os.path.join(target_dir, "libv8_static.a"))
  # copy mksnapshot
  if target == 'android':
    v8_snapshot = os.path.join(out_dir, cpu is 'arm' and "clang_x86_v8_arm" or cpu is 'arm64' and "clang_x64_v8_arm64" or cpu is 'x64' and 'clang_x64' or cpu is 'x86' and 'clang_x86', "mksnapshot")
  elif target == 'linux':
    v8_snapshot = os.path.join(out_dir, "mksnapshot")
  else:
    print("Cannot find target %s!" % (target))
    return
  shutil.copy(v8_snapshot, os.path.join(target_dir, "mksnapshot"))

  #Don't compile libv8_static.a into libjsenv.so directly, as the no v8 symbol exported. We will use objects compile to solve this issue.
  v8_static_dir_len = len(os.path.dirname(v8_static)) + 1
  _,objs = commands.getstatusoutput("ar t %s" % v8_static)
  # Objs contain all the objects unpacked from libv8_static.a, which is a thin static library.
  for obj in objs.split('\n'):
    dst = os.path.join(target_dir, obj[v8_static_dir_len:])
    dst_dir = os.path.dirname(dst)
    make_dirs(dst_dir)
    shutil.copy(obj, dst)
  # Create v8_objects.gni from libv8_static.a, which include all the objects needed for jsenv building.
  obj_path = os.path.join(target_dir, "v8_objects.gni")
  fw = open(obj_path, "w")
  fw.write("v8_objects = [\n")
  for obj in objs.split('\n'):
    ln = '"' + obj[obj.find("obj/") + 4:] + '",\n'
    fw.write(ln)
  fw.write("]")

def build_cpu_aarch_release(target, cpu, is_release, cwd):
  is_debug = is_release and 'false' or 'true'
  arg_list = {
    'is_debug': is_debug,
    'target_cpu' : cpu,
    'target_os' : target
  }
  gn_gen_args = GN_ARGS % arg_list
  out_dir = "out/%s_%s_%s" % (target, cpu, is_release and 'release' or 'debug')
  gn_gen_command = "gn gen %s --args=\'%s\'" % (out_dir, gn_gen_args)
  print(gn_gen_command)
  os.system(gn_gen_command)
  print("build .............")
  build_cmd = "ninja -C %s v8_static" % out_dir
  print(build_cmd)
  os.system(build_cmd)
  install_result(out_dir, target, cpu, is_release, cwd)

def build_cpu_aarch(target, cpu, cwd):
  build_cpu_aarch_release(target, cpu, False, cwd)
  build_cpu_aarch_release(target, cpu, True, cwd)

def build_one_target(target, cwd):
  gn = get_target_os(target)
  if not gn:
    print("Unkown target:%s" % target)
    return
  for cpu in gn['cpus']:
    build_cpu_aarch(target, cpu['target_cpu'], cwd)

def checkout(remote_branch):
  branch_name = remote_branch.split('/')[-1]
  c,r = commands.getstatusoutput("git branch")
  if c == 0 and r.find(branch_name) >= 0:
    print("branch name : " + branch_name )
    os.system("git checkout %s" % branch_name)
  else:
    print("branch name : " + branch_name + ", remote_branch: " + remote_branch )
    os.system("git checkout -b %s -t %s" % (branch_name, remote_branch))
  #print("checkout remote_branch: " + remote_branch)
  #os.system("git checkout %s" % remote_branch)

def patch_source(patch_dir, source_dir):
  # 1.patches for V8_EXPORT and V8_PLATFORM_EXPORT
  os.system("git checkout BUILD.gn")
  patch_path = os.path.join(patch_dir, "patch/BUILD.gn.patch")
  source_path = os.path.join(source_dir, "BUILD.gn")
  print("patch patch_path %s > source_path %s" % (patch_path, source_path))
  os.system("patch -p0 %s < %s" % (source_path, patch_path))

  # 2.patch v8-debugger-agent-impl.cc for breakpoint bug.
  os.system("git checkout src/inspector/v8-debugger-agent-impl.cc")
  patch_path = os.path.join(patch_dir, "patch/v8-debugger-agent-impl.cc.patch")
  source_path = os.path.join(source_dir, "src/inspector/v8-debugger-agent-impl.cc")
  print("patch patch_path %s > source_path %s" % (patch_path, source_path))
  os.system("patch -p0 %s < %s" % (source_path, patch_path))
  # 3.copy wrapper to v8
  print("copy wrapper %s->%s" %(patch_dir, source_dir))
  dest = os.path.join(source_dir, "wrapper")
  if os.path.exists(dest):
    shutil.rmtree(dest)
  shutil.copytree(os.path.join(patch_dir, "wrapper"), dest)

def build_source(targets, source_dir, remote_branch):
  print(" source dir : " + source_dir )
  cwd = os.path.abspath(os.getcwd())
  os.chdir(source_dir)
  checkout(remote_branch)
  patch_source(cwd, source_dir)
  for t in targets:
    build_one_target(t, cwd)

def main(argv):
  args = parse_args(argv)
  build_source(args.target, args.v8_source, args.remote_branch)

if __name__ == '__main__':
  main(sys.argv[1:])

