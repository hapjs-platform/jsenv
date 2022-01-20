import os
import sys
import optparse
import commands
import re
import json
import shutil
import platform

MAVEN="mvn"

BULID_GN ="""
config("%(name)s_config") {
  include_dirs = [
    "include"
  ]

  if (is_debug) {
    build_type = "debug"
  } else {
    build_type = "release"
  }

  defines = [%(defines)s]

  ldflags = [
    "-L" + rebase_path("$target_os/$target_cpu/$build_type", root_build_dir),
  ]
  libs = [%(libs)s]
}
"""


def get_host_os():
  os = platform.system()
  if os == 'Linux': return 'linux'
  if os == 'windows': return 'win'
  if os == 'Mac': return 'mac'
  return None

host_os = get_host_os()
host_cpu = "x64"

def make_dirs(dirs):
  try:
    os.makedirs(dirs)
  except:
    pass

def read_file(f):
  fd = open(f, "rt")
  c = fd.read()
  print(type(c), c)
  fd.close()
  return c

def run_cmd(cmd):
  cmd_str = " ".join(cmd)
  print(cmd_str)
  s,out = commands.getstatusoutput(cmd_str)
  if s != 0:
    print(out)
    sys.exit(s)

def maven_upload(repo, module, is_snapshot):
  configs = {}
  configs['remoteRepostories'] = repo['remoteRepostories']
  configs['file'] = os.path.join(".dist-native", "%s.zip" % module['name'])
  configs['repositoryId'] = repo['repositoryId']
  configs['name'] = module['name']
  configs['version'] = is_snapshot and "%s-SNAPSHOT" % module['version'] or module['version']
  configs['groupId'] = repo['groupId']
  cmd = []
  cmd.append(MAVEN)
  cmd.append("deploy:deploy-file")
  cmd.append("-Durl=%(remoteRepostories)s" % configs)
  cmd.append("-Dfile=%(file)s" % configs)
  cmd.append("-DgroupId=%(groupId)s" % configs)
  cmd.append("-DrepositoryId=%(repositoryId)s" % configs)
  cmd.append("-DartifactId=%(name)s" % configs)
  cmd.append("-Dpackaging=zip")
  cmd.append("-Dversion=%(version)s" % configs)
  run_cmd(cmd)

def resolve_var(s):
  if type(s) == str:
    return re.sub(r'\$([\w_][\w\d_]*)', lambda v:os.getenv(v.group(1)), s)
  elif type(s) == unicode:
    return re.sub(r'\$([\w_][\w\d_]*)', lambda v:os.getenv(v.group(1)), s.encode('utf-8'))
  elif type(s) == list:
    v = []
    for l in s:
      v.append(resolve_var(l))
    return v
  elif type(s) == dict:
    return resolve_vars(s)
  else:
    return s

def resolve_vars(d):
  for k in d.keys():
    d[k] = resolve_var(d[k])
  return d

def build_gn_args(configs):
  args = []
  if configs['build_type'] == 'debug':
    args.append("is_debug=true")
  args.append("target_os=\\\"%(target_os)s\\\"" % configs)
  args.append("target_cpu=\\\"%(target_cpu)s\\\"" % configs)
  args.append("host_os=\\\"%s\\\"" % host_os)
  args.append("host_cpu=\\\"%s\\\"" % host_cpu)
  if 'args' in configs:
    for k,v in configs['args'].items():
      if type(v) == str or type(v) == unicode:
        args.append("%s=\\\"%s\\\"" % (k, resolve_var(v)))
      else:
        args.append("%s=%s" % (k, str(v)))
  return " ".join(args)

def make_out_dir(name, cpu, build_type):
  return os.path.join("out.gn", "%s-%s.%s"%(name, cpu, build_type))

def build_gn(configs):
  # create gn
  cmd = []
  out_dir = make_out_dir(configs['name'], configs['target_cpu'], configs['build_type'])
  cmd = cmd + ["gn", "gen", out_dir]
  cmd.append("&&")
  cmd = cmd + ["gn", "gen", out_dir, "--args=\"%s\"" % build_gn_args(configs)]
  run_cmd(cmd)
  if 'targets' in configs:
    for target in configs['targets']:
      run_cmd(["ninja", "-C", out_dir, target])
  else:
    run_cmd(["ninja", "-C", out_dir])

def to_var_name(name):
  return re.sub("[-\s.]", "_", name)

def build_all_gn(name, builds):
  for build in builds:
    for cpu in build['target_cpu']:
      configs = {}
      configs['name'] = name
      configs['target_os'] = build['target_os']
      if 'args' in build:
        configs['args'] = build['args']
      configs['target_cpu'] = cpu
      if 'targets' in build:
        configs['targets'] = build['targets']

      configs['build_type'] = 'debug'
      build_gn(configs)
      configs['build_type'] = 'release'
      build_gn(configs)

def copy(src, dst):
  dirname = os.path.dirname(dst)
  make_dirs(dirname)
  print("copy %s => %s ...." % (src, dst))
  try:
    shutil.copyfile(src, dst)
  except IOError, e:
    print("copy %s => %s Error: %s" % (src, dst, str(e)))

def copy_from_source(dist_dir, sources):
  for entry in sources:
    src = entry['src']
    dest = entry['dest']
    files = entry['files']
    for f in files:
      copy(os.path.join(src, f), os.path.join(dist_dir, dest, f))

def copy_out_files(name, dist_dir, tos, cpu, build_type, sources):
  for entry in sources:
    src = entry['src']
    dest = entry['dest']
    files = entry['files']
    for f in files:
      copy(os.path.join(make_out_dir(name, cpu, build_type), src, f), os.path.join(dist_dir, tos, cpu, build_type, f))

def copy_from_out(name, dist_dir, sources, builds):
  for build in builds:
    for cpu in build['target_cpu']:
      copy_out_files(name, dist_dir, build['target_os'], cpu, 'debug', sources)
      copy_out_files(name, dist_dir, build['target_os'], cpu, 'release', sources)

def copy_build_files(project):
  module = project['module']
  dist_dir = os.path.join(".dist-native", module['name'])
  make_dirs(dist_dir)
  if 'from-source' in module:
    copy_from_source(dist_dir, module['from-source'])
  if 'from-out' in module:
    copy_from_out(module['name'], dist_dir, module['from-out'], project['builds'])

def gen_build_gn(module):
  configs = {}
  configs['name'] = to_var_name(module['name'])
  if 'defines' in module:
    configs['defines'] = ','.join(["\"%s\"" % d for d in module['defines']])
  if 'libs' in module:
    configs['libs'] = ','.join(["\"%s\"" % l for l in module['libs']])
  return BULID_GN % configs

def install_build_gn(project):
  module = project['module']
  dist_build_gn = os.path.join(".dist-native", module['name'], "BUILD.gn")
  f = open(dist_build_gn, "wt")
  f.write(gen_build_gn(module))
  f.close()

def zip_file(project):
  pwd = os.getcwd()
  os.chdir(".dist-native")
  module = project['module']
  run_cmd(["rm", "-f", "%s.zip"%module['name']])
  run_cmd(["zip", "-r", "%s.zip"%module['name'], module['name']])
  os.chdir(pwd)

def parse_args(argv):
  parser = optparse.OptionParser()
  parser.add_option("-s", "--upload-snapshot", dest="uploadSnapshot", action = "store_true", default=False, help="upload snapshot")
  parser.add_option("-r", "--upload-release", dest="uploadRelease", action = "store_true", default=False, help="upload release")
  parser.add_option("-l", "--upload-all", dest="uploadAll", action = "store_true", default=False, help="upload all")
  parser.add_option("-z", "--zip", dest="zip", action="store_true", default=False, help="zip file")
  parser.add_option("-b", "--build", dest="build", action="store_true", default=False, help="build")
  parser.add_option("-c", "--copy", dest="copy", action="store_true", default=False, help="copy")
  parser.add_option("-a", "--all", dest="all", action="store_true", default=False, help="do all")
  options, args = parser.parse_args(argv)
  return (options, args[0])

def main():
  options, json_file = parse_args(sys.argv[1:])
  project = resolve_vars(json.loads(read_file(json_file)))
  if options.all or options.build:
    build_all_gn(project['module']['name'], project['builds'])
  if options.all or options.copy:
    copy_build_files(project)
    install_build_gn(project)
  if options.all or options.zip:
    zip_file(project)
  if options.all or options.uploadAll or options.uploadSnapshot:
    maven_upload(project['snapshot'], project['module'], True)
  if options.all or options.uploadAll or options.uploadRelease:
    maven_upload(project['release'], project['module'], False)

if __name__ == '__main__':
  main()
