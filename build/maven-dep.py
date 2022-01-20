# coding:utf-8

import os
import sys
import optparse
import commands
import re
import json

MAVEN="mvn"

LOCAL_REPOSTORY = os.path.join(os.getenv("HOME"), ".m2", "repository")

# 使用mvn命令下载依赖包，并将其解压到特定目录下

# 参数说明
# --remote-repostories 定义远程仓库地址
# --group-id 定义仓库group id
# --local-prefix 定义本地解压存储的路径
# --config-name 定义生成的本地配置文件
# --config-type 定义本地生成的配置文件的类型：mk, gn
# --module 定义模块，可以定义多个，格式是  <module-name>:<version>
# --force force download and force set the values

def build_maven_get_dep_cmd(configs):
  cmd = []
  cmd.append(MAVEN)
  if ('force' in configs and configs['force']):
    cmd.append("-U")
  cmd.append("dependency:get")
  cmd.append("-DremoteRepositories=%(remoteRepostories)s" % configs)
  cmd.append("-DgroupId=%(groupId)s" % configs)
  cmd.append("-Dpackaging=zip")
  cmd.append("-DartifactId=%(module_name)s" % configs)
  cmd.append("-Dversion=%(module_version)s" % configs)
  return " ".join(cmd)

def get_local_module(groupId, module, version):
  groups = groupId.split('.')
  path = os.path.join(LOCAL_REPOSTORY, *groups)
  return os.path.join(path, module, version, "%s-%s.zip" % (module, version))


def make_dirs(dirs):
  try:
    os.makedirs(dirs)
  except:
    pass

def unzip_module_cmd(zip_file, dest):
  cmd = []
  cmd.append("unzip")
  cmd.append("-x")
  cmd.append(zip_file)
  cmd.append("-d")
  cmd.append(dest)
  return " ".join(cmd)

def gen_mk_config(dependencies):
  return "\n".join(["prebuilt_%s=%s" % (k,v) for k,v in dependencies.items()])

def gen_gn_config(dependencies):
  return "\n".join(["prebuilt_%s=\"%s\"" % (k,v) for k,v in dependencies.items()])

def gen_config_content(dependencies, config_type):
  if config_type == 'mk':
    return gen_mk_config(dependencies)
  elif config_type == 'gn':
    return gen_gn_config(dependencies)

def gen_config_file(dependencies, config_name, config_type):
  f = open(config_name, "wt")
  f.write(gen_config_content(dependencies, config_type))
  f.close()

def to_var_name(name):
  return re.sub("[-\s.]", "_", name)

def make_depedencies(args):
  dependencies = {}
  for module in args.modules:
    name,version = module.split(':')
    configs = {}
    configs['remoteRepostories'] = args.remoteRepostories
    configs['groupId'] = args.groupId
    configs['module_name'] = name
    configs['module_version'] = version
    configs['force'] = args.force

    local_path = os.path.join(args.localPrefix, "%s-%s" % (name, version))
    cmds = []
    cmds.append(build_maven_get_dep_cmd(configs))
    if args.force or (not os.path.exists(local_path)):
      make_dirs(local_path)
      cmds.append(unzip_module_cmd(get_local_module(args.groupId, name, version), local_path))
    dependencies[to_var_name(name)] = local_path
    total_cmds = " && ".join(cmds)
    print(total_cmds)
    os.system(total_cmds)
  gen_config_file(dependencies, args.configName, args.configType)

def read_file(f):
  fd = open(f, "rt")
  c = fd.read()
  print(type(c), c)
  fd.close()
  return c

class JsonArgs:
  defaults = {
    "force": False
  }
  def __init__(self, json, options):
    self.args = json
    if options.force:
      self.args['force'] = options.force

  def __getattr__(self, key):
    if key in self.args:
      return self.args[key]
    return JsonArgs.defaults[key]

def resolve_var(s):
  print(type(s), s)
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
    print(d[k])
  return d

def options_from_file(arg_file, options):
  return JsonArgs(resolve_vars(json.loads(read_file(arg_file))), options)


def parse_args(argv):
  usage = "usage: %prog [options]"
  parser = optparse.OptionParser(usage = usage)
  parser.add_option("-r", "--remote-repostories", dest = "remoteRepostories", type="string", help="remote repostories url")
  parser.add_option("-g", "--group-id", dest = "groupId", type="string", help="group id")
  parser.add_option("-m", "--module", dest="modules", action="append", type="string", default=[], help="module depedency")
  parser.add_option("-c", "--config-name", dest="configName", type="string", help="set config name")
  parser.add_option("-t", "--config-type", dest="configType", type="string", default = "gn", help="set config type")
  parser.add_option("-p", "--local-prefix", dest="localPrefix", type="string", help="set the local prefix dir")
  parser.add_option("-f", "--force", dest="force", action="store_true", default=False, help="force update")

  options,args = parser.parse_args(argv)
  if len(args) == 1 and not options.remoteRepostories:
    parser.print_usage()
    sys.exit(0)
  if len(args) == 2 and not options.remoteRepostories:
    return options_from_file(args[1], options)

  resolve_vars(options.__dict__)
  return options


def main():
  make_depedencies(parse_args(sys.argv))

if __name__ == '__main__':
  main()
