#!/usr/bin/env python

# Copyright (c) 2021, the hapjs-platform Project Contributors
# SPDX-License-Identifier: EPL-1.0

# interface idl generate cpp file

import sys
import os
from idl import InterfaceLoader
from idl import ast
from mako.template import Template
from options import *

pwd_dir = os.path.dirname(os.path.realpath(sys.argv[0]))

def ReadFile(filename):
  f = open(filename)
  content = f.read()
  f.close()
  return content

def GetFile(filename):
  return os.path.join(pwd_dir, 'templates', filename)

def GetTemplate(filename):
  return Template(ReadFile(GetFile(filename)))

class CppValueType:
  def __init__(self, type, name):
    self.type = self.GetCppType(type)
    self.name = name

  def GetCppType(self, type):
    s = CppValueType.cpp_types.get(type.__class__, '<unkown>')
    return s

  cpp_types = {
    ast.VoidType : 'void',
    ast.BooleanType : 'bool',
    ast.CharType : 'char',
    ast.ByteType : 'int8_t',
    ast.UnsignedByteType : 'uint8_t',
    ast.ShortType : 'int16_t',
    ast.UnsignedShortType : 'uint16_t',
    ast.IntType : 'int32_t',
    ast.UnsignedIntType : 'uint32_t',
    ast.LongType : 'int64_t',
    ast.UnsignedLongType : 'uint64_t',
    ast.FloatType : 'float',
    ast.DoubleType : 'double',
    ast.StringType : 'base::string'
  }

class CppInterface:
  def __init__(self, idl_file):
    self.interface = InterfaceLoader(idl_file)
    self.CreateNames()
    self.CreateImports()
    self.methods = self.ProcessMethods()
    self.constructors = self.ProcessConstructors()
    self.properties = self.ProcessProperties()
    self.events = self.ProcessEvents()

  def CreateNames(self):
    self.name = self.interface.name
    fullname = self.interface.package + '.' + self.interface.name
    self.header = fullname.replace('.', '/') + '.h'
    self.interface_source = fullname.replace('.', '/') + '_interface.cc'
    self.header_guard = fullname.replace('.','_').upper() + '_H_'
    self.implement_guard = 'IMPLEMENT_' + fullname.replace('.','_').upper()
    self.namespace_using = self.interface.package.replace('.', '::')
    self.namespaces = self.interface.package.split('.')
    self.extends = self.interface.extends
    self.fullname = fullname;

  def HasEvents(self):
    return len(self.interface.events) > 0

  def ProcessMethods(self):
    methods = []
    for method in self.interface.methods:
      method.cpp_define = self.CreateMethodDefine(method)
      methods.append(method)
    return methods

  def ProcessEvents(self):
    events = []
    for event in self.interface.events:
      event.cpp_define = self.CreateArgumentsDefine(event.arguments)
      events.append(event)
    return events

  def ProcessConstructors(self):
    constructors = []
    for ctr in self.interface.constructors:
      ctr.cpp_define = self.CreateArgumentsDefine(ctr.arguments)
      constructors.append(ctr)
    return constructors

  def ProcessProperties(self):
    properties = []
    for prop in self.interface.properties:
      prop.cpp_type = CppValueType(prop.type, '')
      properties.append(prop)
    return properties

  def CreateMethodDefine(self, method):
    defines = [CppValueType(method.type, '')]
    return defines + self.CreateArgumentsDefine(method.arguments)

  def CreateArgumentsDefine(self, arguments):
    defines = []
    for arg in arguments:
      defines.append(CppValueType(arg.type, arg.name.fullname))
    return defines

  def CreateImports(self):
    self.usings = []
    self.import_headers = []
    for imp in self.interface.import_list:
      self.usings.append(imp.replace('.', '::'))
      self.import_headers.append(imp.replace('.', '/') + '.h')


class CppRender:
  def __init__(self, idl_file):
    self.interface = CppInterface(idl_file)
    self.meta_template = GetTemplate("meta.mt")
    self.interface_template = GetTemplate("interface.mt")

  def GenerateMeta(self):
    return self.meta_template.render(interface = self.interface)

  def GeneratorInterface(self):
    return self.interface_template.render(interface = self.interface)


options = [
  Option('h', 'header', True),
  Option('c', 'source', True),
  Option('i', 'idl', True)
]

def LastWith(s, w):
  idx = s.rindex(w)
  if idx >= 0:
    return s[idx] == w
  return False

def CheckOutFile(cr, args):
  header = cr.interface.header
  source = cr.interface.interface_source
  if LastWith(args.header, header):
    sys.stderr.write("header file must end with: \'%s\'" % header)
    return False
  if LastWith(args.source, source):
    sys.stderr.write("source file must end with: \'%s\'" % source)
    return False
  return True

def WriteFile(out, file_name):
  f = open(file_name, 'wt')
  f.write(out)
  f.close()


def main():
  args = parse_options(sys.argv[1:], options)
  cr = CppRender(args.idl)
  if not cr:
    sys.stderr.write("parse idl: %s" % args.idl)
    return
  if not CheckOutFile(cr, args):
    return
  WriteFile(cr.GenerateMeta(), args.source)
  WriteFile(cr.GeneratorInterface(), args.header)


def test():
  cr = CppRender(sys.argv[1])
  print("====================== meta ===================")
  print(cr.GenerateMeta())
  print("====================== interface ===============")
  print(cr.GeneratorInterface())


if __name__ == '__main__':
  main()
