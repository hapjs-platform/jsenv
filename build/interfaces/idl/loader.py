import os
import sys
from parser import IDLParser
from visitor import ASTVisitor
from ast import *


def ErrorMessage(f, lineno, lexpos, msg):
  return "\n%s\n%s" % (FileLineMsg(f, lineno, msg), SourceLine(f, lineno, lexpos))

def FirstUpper(s):
  return s[0].upper() + s[1:]

def FileLineMsg(f, lineno, msg):
  filename = f.filename
  if filename:
    return "%s(%d) : %s" % (filename, lineno, msg)
  return "<BuiltIn> : %s" % msg

def SourceLine(f, lineno, lexpos):
  pos = lexpos - f.index[lineno - 1]
  caret = ' ' * pos + '^'
  return "%s\n%s\n" % (f.lines[lineno - 1], caret)

def AppendMethodTo(name, method, methods):
  for n, m in methods:
    if n == name:
      m.append(method)
      return
  methods.append((name, [method]))

def AppendDynamicEntryTo(key, name, member, entries):
  key = key or ''
  name = name or member.simpleName
  item = None
  for k, v in entries:
    if k == key:
      item = v
      break
  if not item:
    item = []
    entries.append([key, item])

  item.append([name, member])

def AppendNamespaceTo(namespace, namespaces):
  if namespace not in namespaces:
    namespaces.append(namespace)


def LastName(name):
  return name[name.rindex('.')+1:]

primtype_signature = {
  BooleanType : 'z',
  CharType : 'c',
  StringType : 'S',
  VoidType : 'v',
  ByteType : 'b',
  UnsignedByteType : 'B',
  ShortType : 'h',
  UnsignedShortType: 'H',
  IntType : 'i',
  UnsignedIntType : 'I',
  LongType : 'l',
  UnsignedLongType: 'L',
  FloatType : 'f',
  DoubleType : 'd'
}

primtype_types = {
  BooleanType : 'kInt',
  CharType : 'kInt',
  StringType : 'kString',
  VoidType : 'kVoid',
  ByteType : 'kInt',
  UnsignedByteType : 'kInt',
  ShortType : 'kInt',
  UnsignedShortType: 'kInt',
  IntType : 'kInt',
  UnsignedIntType : 'kInt',
  LongType : 'kInt64',
  UnsignedLongType: 'kInt64',
  FloatType : 'kFloat32',
  DoubleType : 'kFloat64'
}


class InterfaceLoader:
  def __init__(self, idl_file):
    self.idl_file = idl_file
    self.Load()

  def Load(self):
    parser = IDLParser()
    f = parser.ParseFile(self.idl_file)
    self.Resolve(f)

  def Resolve(self, f):
    self.package = f.package.name.fullname
    self.import_list = [ imp.name.fullname for imp in f.imports]
    self.name = f.interface.name.fullname
    self.extendedAttributes = f.interface.extendedAttributes
    self.modifiers = f.interface.modifiers
    self.extends = f.interface.extends
    self.methods = []
    self.properties = []
    self.events = []
    self.constructors = []
    self.ResolveImports()
    self.ParseMembers(f.interface)

  @property
  def fullname(self):
    return self.package + '.' + self.name

  def ResolveImports(self):
    self.resolved = {}
    self.resolved[self.name] = self.fullname
    for imp in self.import_list:
      self.resolved[LastName(imp)] = imp

  def ParseMembers(self, interface):
    for member in interface.members:
      func = InterfaceLoader.ParseMethods.get(member.__class__)
      if func:
        func(self, member)

  def ParseMethod(self, method):
    method.signature = self.CreateSignature(method.type, method.arguments)
    method.index_name = "kMethod%s" % (FirstUpper(method.name.fullname))
    method.call_types = self.CreateCallTypes(method.type, method.arguments)
    self.methods.append(method)

  def ParseProperty(self, prop):
    prop.signature = self.TypeSignature(prop.type)
    prop.value_type = self.GetValueType(prop.type)
    prop.readonly = prop.HasModifier(ReadOnlyModifier)
    prop.index_name = "kProperty%s" % (FirstUpper(prop.name.fullname))
    self.properties.append(prop)

  def ParseConstructor(self, ctr):
    ctr.argument_types = self.CreateArgumentTypes(ctr.arguments)
    self.constructors.append(ctr)

  def ParseEvent(self, event):
    if event.GetExtendedAttribute('final'):
      event.final = True
    else:
      event.final = False
    self.events.append(event)

  def TypeSignature(self, type):
    s = primtype_signature.get(type.__class__ , None)
    if s: return s
    if isinstance(type, Interface):
      return '{' + self.resolved[type.name] + '}'
    else:
      raise Exception("unkown type: %s (%s)" % (type.GetClass(), type.ToString()))

  def CreateSignature(self, return_type, args):
    ret = self.TypeSignature(return_type)
    for arg in args:
      ret = ret + self.TypeSignature(arg.type)
    return ret

  def GetValueType(self, type):
    s = primtype_types.get(type.__class__, None)
    if s: return s
    return 'kUnsupportType' #TODO support anther type

  def CreateCallTypes(self, ret_type, arguments):
    l = [self.GetValueType(ret_type)]
    return l + self.CreateArgumentTypes(arguments)

  def CreateArgumentTypes(self, arguments):
    l = []
    for arg in arguments:
      l.append(self.GetValueType(arg.type))
    return l

  ParseMethods = {
    Method : ParseMethod,
    Property : ParseProperty,
    Event : ParseEvent,
    Constructor: ParseConstructor
  }
