from visitor import visit

class Node:
  def GetClass(self):
    return self.__class__.__name__

  def HasModifier(self, M):
    for m in getattr(self, 'modifiers', []):
      if isinstance(m, M):
        return True
    return False

  def GetExtendedAttribute(self, name):
    for attribute in getattr(self, 'extendedAttributes', []):
      if attribute.name == name:
        return attribute
    return None

  def GetRepeatableExtendedAttribute(self, name):
    attributes = []
    for attribute in getattr(self, 'extendedAttributes', []):
      if attribute.name == name:
        attributes.append(attribute)
    return attributes

  def GetExtendedAttributeValue(self, name, index):
    attribute = self.GetExtendedAttribute(name)
    return attribute.GetValue(index) if attribute else None

  def GetExtendedAttributeValues(self, name, *indice):
    attribute = self.GetExtendedAttribute(name)
    return attribute.GetValues(*indice) if attribute else None

  def GetRepeatableExtendedAttributeValues(self, name, *indice):
    attributes = self.GetRepeatableExtendedAttribute(name)
    return [ attribute.GetValues(*indice) for attribute in attributes ]

  def ToString(self):
    return "Node(class:%s, %s" % (self.GetClass(), self.__dict__)


class Literal(Node):
  pass

class BooleanLiteral(Literal):
  def __init__(self, value):
    self.value = value

class IntLiteral(Literal):
  def __init__(self, value):
    self.value = value

class FloatLiteral(Literal):
  def __init__(self, value):
    self.value = value

class CharLiteral(Literal):
  def __init__(self, value):
    self.value = value

class StringLiteral(Literal):
  def __init__(self, value):
    self.value = value

class NullLiteral(Literal):
  def __init__(self, value):
    self.value = value

class Name(Node):
  pass

class SimpleName(Node):
  def __init__(self, name):
    self.name = name

  @property
  def fullname(self):
    return self.name

@visit('qualifier', 'identifier')
class QualifiedName(Node):
  def __init__(self, qualifier, identifier):
    self.qualifier = qualifier
    self.identifier = identifier

  @property
  def fullname(self):
    return self.qualifier.fullname + '.' + self.identifier.fullname

class Type(Node):
  pass

class BuiltinType(Type):
  pass

class PrimitiveType(BuiltinType):
  pass

class VoidType(PrimitiveType):
  pass

class BooleanType(PrimitiveType):
  pass

class CharType(PrimitiveType):
  pass

class ByteType(PrimitiveType):
  pass

class UnsignedByteType(PrimitiveType):
  pass

class ShortType(PrimitiveType):
  pass

class UnsignedShortType(PrimitiveType):
  pass

class IntType(PrimitiveType):
  pass

class UnsignedIntType(PrimitiveType):
  pass

class LongType(PrimitiveType):
  pass

class UnsignedLongType(PrimitiveType):
  pass

class FloatType(PrimitiveType):
  pass

class DoubleType(PrimitiveType):
  pass

class StringType(PrimitiveType):
  pass

class CompositType(BuiltinType):
  pass

@visit('elementType')
class ArrayType(CompositType):
  def __init__(self, elementType):
    self.elementType = elementType

@visit('keyType', 'valueType')
class MapType(CompositType):
  def __init__(self, keyType, valueType):
    self.keyType = keyType
    self.valueType = valueType

@visit('keyType')
class SetType(CompositType):
  def __init__(self, keyType):
    self.keyType = keyType

@visit('valueType')
class OptionalType(CompositType):
  def __init__(self, valueType):
    self.valueType = valueType

@visit('valueType')
class ConstType(CompositType):
  def __init__(self, valueType):
    self.valueType = valueType

@visit('valueType')
class PointerType(CompositType):
  def __init__(self, valueType):
    self.valueType = valueType

@visit('valueType')
class ReferenceType(CompositType):
  def __init__(self, valueType):
    self.valueType = valueType

@visit('name')
class UserDefinedType(Type):
  def __init__(self, name):
    self.name = name

class Modifier(Node):
  pass

class StaticModifier(Modifier):
  pass

class ReadOnlyModifier(Modifier):
  pass

class ExtendedAttributeArgument(Node):
  def __init__(self, key, value):
    self.key = key
    self.value = value

@visit('arguments')
class ExtendedAttribute(Node):
  def __init__(self, name, arguments):
    self.name = name
    self.arguments = arguments

  def GetValue(self, index = 0):
    if isinstance(index, int):
      return self.arguments[index].key if index >= 0 and index < len(self.arguments) else None
    elif isinstance(index, str):
      for argument in self.arguments:
        if (argument.key == index):
          return argument.value
      return None
    return None

  def GetValues(self, *indice):
    return [ self.GetValue(index) for index in indice ]

class Member(Node):
  pass

class TypeDeclaration(Member):
  pass

@visit('extendedAttributes', 'modifiers', 'name', 'extends', '+', 'members', '-')
class Interface(TypeDeclaration):
  def __init__(self, extendedAttributes, modifiers, name, extends, members):
    self.extendedAttributes = extendedAttributes
    self.modifiers = modifiers
    self.name = name
    self.extends = extends
    self.members = members

@visit('extendedAttributes', 'modifiers', 'name', 'extends', '+', 'members', '-')
class Struct(TypeDeclaration):
  def __init__(self, extendedAttributes, modifiers, name, extends, members):
    self.extendedAttributes = extendedAttributes
    self.modifiers = modifiers
    self.name = name
    self.extends = extends
    self.members = members

@visit('extendedAttributes', 'modifiers', 'name', '+', 'members', '-')
class Enum(TypeDeclaration):
  def __init__(self, extendedAttributes, modifiers, name, members):
    self.extendedAttributes = extendedAttributes
    self.modifiers = modifiers
    self.name = name
    self.members = members

@visit('extendedAttributes', 'modifiers', 'type', 'name', 'arguments')
class Delegate(TypeDeclaration):
  def __init__(self, extendedAttributes, modifiers, type, name, arguments):
    self.extendedAttributes = extendedAttributes
    self.modifiers = modifiers
    self.type = type
    self.name = name
    self.arguments = arguments

@visit('extendedAttributes', 'name', 'arguments')
class Event(TypeDeclaration):
  def __init__(self, extendedAttributes, name, arguments):
    self.extendedAttributes = extendedAttributes
    self.name = name
    self.arguments = arguments

@visit('extendedAttributes', 'modifiers', 'name', 'arguments')
class Constructor(Member):
  def __init__(self, extendedAttributes, modifiers, name, arguments):
    self.extendedAttributes = extendedAttributes
    self.modifiers = modifiers
    self.name = name
    self.arguments = arguments

@visit('extendedAttributes', 'modifiers', 'type', 'name', 'arguments')
class Method(Member):
  def __init__(self, extendedAttributes, modifiers, type, name, arguments):
    self.extendedAttributes = extendedAttributes
    self.modifiers = modifiers
    self.type = type
    self.name = name
    self.arguments = arguments

@visit('extendedAttributes', 'modifiers', 'type', 'name')
class Property(Member):
  def __init__(self, extendedAttributes, modifiers, type, name):
    self.extendedAttributes = extendedAttributes
    self.modifiers = modifiers
    self.type = type
    self.name = name

@visit('extendedAttributes', 'name')
class EnumValue(Member):
  def __init__(self, extendedAttributes, name):
    self.extendedAttributes = extendedAttributes
    self.name = name

@visit('extendedAttributes', 'type', 'name', 'defaultValue')
class Argument(Node):
  def __init__(self, extendedAttributes, type, name, defaultValue):
    self.extendedAttributes = extendedAttributes
    self.type = type
    self.name = name
    self.defaultValue = defaultValue

@visit('name')
class Package(Member):
  def __init__(self, name):
    self.name = name

class Import(Node):
  def __init__(self, name):
    self.name = name

@visit('package', 'imports', 'interface')
class File(Node):
  def __init__(self, package, imports, interface):
    self.package = package
    self.imports = imports
    self.interface = interface

