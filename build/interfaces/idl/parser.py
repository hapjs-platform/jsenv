import sys
import ply.yacc as yacc
import ply.lex as lex
from ast import *
import lexer

def CreateASTNode(p, t, *args):
  n = t(*args)
  p[0] = n
  for i in range(1, len(p)):
    if isinstance(p[i], Node):
      n.lineno = p[i].lineno
      n.lexpos = p[i].lexpos
      return
    elif isinstance(p[i], list):
      for m in p[i]:
        if isinstance(m, Node):
          n.lineno = m.lineno
          n.lexpos = m.lexpos
          return
    else:
      n.lineno = p.lineno(i)
      n.lexpos = p.lexpos(i)
      return


class IDLParser:

  def p_BooleanLiteral(self, p):
    """BooleanLiteral : TRUE
                      | FALSE"""
    CreateASTNode(p, BooleanLiteral, p[1])

  def p_IntLiteral(self, p):
    """IntLiteral : integer"""
    CreateASTNode(p, IntLiteral, p[1])

  def p_FloatLiteral(self, p):
    """FloatLiteral : float"""
    CreateASTNode(p, FloatLiteral, p[1])

  def p_CharLiteral(self, p):
    """CharLiteral : char"""
    CreateASTNode(p, CharLiteral, p[1])

  def p_StringLiteral(self, p):
    """StringLiteral : string"""
    CreateASTNode(p, StringLiteral, p[1])

  def p_NullLiteral(self, p):
    """NullLiteral : NULL"""
    CreateASTNode(p, NullLiteral, p[1])

  def p_Literal(self, p):
    """Literal : BooleanLiteral
               | IntLiteral
               | FloatLiteral
               | CharLiteral
               | StringLiteral
               | NullLiteral"""
    p[0] = p[1]

  def p_SimpleName(self, p):
    """SimpleName : identifier"""
    CreateASTNode(p, SimpleName, p[1])

  def p_QualifiedName(self, p):
    """QualifiedName : Name '.' SimpleName"""
    CreateASTNode(p, QualifiedName, p[1], p[3])

  def p_Name(self, p):
    """Name : SimpleName
            | QualifiedName"""
    p[0] = p[1]

  def p_BooleanType(self, p):
    """BooleanType : BOOLEAN"""
    CreateASTNode(p, BooleanType)

  def p_CharType(self, p):
    """CharType : CHAR"""
    CreateASTNode(p, CharType)

  def p_ByteType(self, p):
    """ByteType : BYTE"""
    CreateASTNode(p, ByteType)

  def p_UnsignedByteType(self, p):
    """UnsignedByteType : UNSIGNED BYTE"""
    CreateASTNode(p, UnsignedByteType)

  def p_ShortType(self, p):
    """ShortType : SHORT"""
    CreateASTNode(p, ShortType)

  def p_UnsignedShortType(self, p):
    """UnsignedShortType : UNSIGNED SHORT"""
    CreateASTNode(p, UnsignedShortType)

  def p_IntType(self, p):
    """IntType : INT"""
    CreateASTNode(p, IntType)

  def p_UnsignedIntType(self, p):
    """UnsignedIntType : UNSIGNED INT"""
    CreateASTNode(p, UnsignedIntType)

  def p_LongType(self, p):
    """LongType : LONG"""
    CreateASTNode(p, LongType)

  def p_UnsignedLongType(self, p):
    """UnsignedLongType : UNSIGNED LONG"""
    CreateASTNode(p, UnsignedLongType)

  def p_FloatType(self, p):
    """FloatType : FLOAT"""
    CreateASTNode(p, FloatType)

  def p_DoubleType(self, p):
    """DoubleType : DOUBLE"""
    CreateASTNode(p, DoubleType)

  def p_VoidType(self, p):
    """VoidType : VOID"""
    CreateASTNode(p, VoidType)

  def p_StringType(self, p):
    """StringType : STRING"""
    CreateASTNode(p, StringType)

  def p_ArrayType(self, p):
    """ArrayType : Type '[' ']'"""
    CreateASTNode(p, ArrayType, p[1])

  def p_MapType(self, p):
    """MapType : MAP '<' Type ',' Type '>'"""
    CreateASTNode(p, MapType, p[3], p[5])

  def p_SetType(self, p):
    """SetType : SET '<' Type '>'"""
    CreateASTNode(p, SetType, p[3])

  def p_OptionalType(self, p):
    """OptionalType : Type '?'"""
    CreateASTNode(p, OptionalType, p[1])

  def p_ConstType(self, p):
    """ConstType : CONST Type"""
    CreateASTNode(p, ConstType, p[2])

  def p_PointerType(self, p):
    """PointerType : Type '*'"""
    CreateASTNode(p, PointerType, p[1])

  def p_ReferenceType(self, p):
    """ReferenceType : Type '&'"""
    CreateASTNode(p, ReferenceType, p[1])

  def p_UserDefinedType(self, p):
    """UserDefinedType : Name"""
    CreateASTNode(p, UserDefinedType, p[1])

  def p_Type(self, p):
    """Type : BooleanType
            | CharType
            | ByteType
            | UnsignedByteType
            | ShortType
            | UnsignedShortType
            | IntType
            | UnsignedIntType
            | LongType
            | UnsignedLongType
            | FloatType
            | DoubleType
            | VoidType
            | StringType
            | ArrayType
            | MapType
            | SetType
            | OptionalType
            | ConstType
            | PointerType
            | ReferenceType
            | UserDefinedType"""
    p[0] = p[1]

  def p_StaticModifier(self, p):
    """StaticModifier : STATIC"""
    CreateASTNode(p, StaticModifier)

  def p_ReadOnlyModifier(self, p):
    """ReadOnlyModifier : READONLY"""
    CreateASTNode(p, ReadOnlyModifier)

  def p_Modifier(self, p):
    """Modifier : StaticModifier
                | ReadOnlyModifier"""
    p[0] = p[1]

  def p_ModifierList(self, p):
    """ModifierList : ModifierList Modifier
                    |"""
    if len(p) == 1:
      p[0] = []
    else:
      p[0] = p[1] + [p[2]]

  def p_ExtendedAttributeValue(self, p):
    """ExtendedAttributeValue : identifier
                              | StringLiteral"""
    if isinstance(p[1], StringLiteral):
      p[0] = p[1].value[1:-1]
    else:
      p[0] = p[1]

  def p_ExtendedAttributeArgument(self, p):
    """ExtendedAttributeArgument : ExtendedAttributeValue '=' ExtendedAttributeValue
                                 | ExtendedAttributeValue"""
    if len(p) == 2:
      CreateASTNode(p, ExtendedAttributeArgument, p[1], None)
    else:
      CreateASTNode(p, ExtendedAttributeArgument, p[1], p[3])

  def p_ExtendedAttributeArgumentList(self, p):
    """ExtendedAttributeArgumentList : ExtendedAttributeArgument ',' ExtendedAttributeArgumentList
                                     | ExtendedAttributeArgument
                                     |"""
    if len(p) == 4:
      p[0] = [p[1]] + p[3]
    elif len(p) == 2:
      p[0] = [p[1]]
    else:
      p[0] = []

  def p_ExtendedAttribute(self, p):
    """ExtendedAttribute : identifier
                         | identifier '(' ExtendedAttributeArgumentList ')'"""
    if len(p) == 2:
      CreateASTNode(p, ExtendedAttribute, p[1], [])
    else:
      CreateASTNode(p, ExtendedAttribute, p[1], p[3])

  def p_ExtendedAttributeList(self, p):
    """ExtendedAttributeList : ExtendedAttribute ',' ExtendedAttributeList
                             | ExtendedAttribute
                             |"""
    if len(p) == 4:
      p[0] = [p[1]] + p[3]
    elif len(p) == 2:
      p[0] = [p[1]]
    else:
      p[0] = []

  def p_ExtendedAttributeBlock(self, p):
    """ExtendedAttributeBlock : '[' ExtendedAttributeList ']'
                              |"""
    if len(p) == 4:
      p[0] = p[2]
    else:
      p[0] = []

  def p_Extends(self, p):
    """Extends : ':' UserDefinedType
               |"""
    if len(p) == 3:
      p[0] = p[2]
    else:
      p[0] = None

  def p_Interface(self, p):
    """Interface : ExtendedAttributeBlock ModifierList INTERFACE SimpleName Extends '{' MemberList '}' ';'"""
    CreateASTNode(p, Interface, p[1], p[2], p[4], p[5], p[7])

  def p_Struct(self, p):
    """Struct : ExtendedAttributeBlock ModifierList STRUCT SimpleName Extends '{' MemberList '}' ';'"""
    CreateASTNode(p, Struct, p[1], p[2], p[4], p[5], p[7])

  def p_Enum(self, p):
    """Enum : ExtendedAttributeBlock ModifierList ENUM SimpleName '{' EnumValueList '}' ';'"""
    CreateASTNode(p, Enum, p[1], p[2], p[4], p[6])

  def p_Property(self, p):
    """Property : ExtendedAttributeBlock ModifierList PROPERTY Type SimpleName ';'"""
    CreateASTNode(p, Property, p[1], p[2], p[4], p[5])

  def p_EnumValue(self, p):
    """EnumValue : ExtendedAttributeBlock SimpleName"""
    CreateASTNode(p, EnumValue, p[1], p[2])

  def p_EnumValueList(self, p):
    """EnumValueList : EnumValue ',' EnumValueList
                 | EnumValue
                 |"""
    if len(p) == 4:
      p[0] = [p[1]] + p[3]
    elif len(p) == 2:
      p[0] = [p[1]]
    else:
      p[0] = []

  def p_Argument(self, p):
    """Argument : ExtendedAttributeBlock Type SimpleName
                | ExtendedAttributeBlock Type SimpleName '=' Literal"""
    if len(p) == 4:
      CreateASTNode(p, Argument, p[1], p[2], p[3], None)
    else:
      CreateASTNode(p, Argument, p[1], p[2], p[3], p[5])

  def p_ArgumentList(self, p):
    """ArgumentList : Argument ',' ArgumentList
                | Argument
                |"""
    if len(p) == 4:
      p[0] = [p[1]] + p[3]
    elif len(p) == 2:
      p[0] = [p[1]]
    else:
      p[0] = []

  def p_Delegate(self, p):
    """Delegate : ExtendedAttributeBlock ModifierList DELEGATE Type SimpleName '(' ArgumentList ')' ';'"""
    CreateASTNode(p, Delegate, p[1], p[2], p[4], p[5], p[7])

  def p_Event(self, p):
    """Event : ExtendedAttributeBlock EVENT SimpleName '(' ArgumentList ')' ';'"""
    CreateASTNode(p, Event, p[1], p[3], p[5])

  def p_Method(self, p):
    """Method : ExtendedAttributeBlock ModifierList Type SimpleName '(' ArgumentList ')' ';'"""
    CreateASTNode(p, Method, p[1], p[2], p[3], p[4], p[6])

  def p_Constructor(self, p):
    """Constructor : ExtendedAttributeBlock ModifierList SimpleName '(' ArgumentList ')' ';'"""
    CreateASTNode(p, Constructor, p[1], p[2], p[3], p[5])

  def p_Package(self, p):
    """Package : PACKAGE Name ';'"""
    CreateASTNode(p, Package, p[2])

  def p_Member(self, p):
    """Member : Struct
              | Enum
              | Delegate
              | Constructor
              | Method
              | Event
              | Property"""
    p[0] = p[1]

  def p_MemberList(self, p):
    """MemberList : MemberList Member
                  |"""
    if len(p) == 3:
      p[0] = p[1] + [p[2]]
    else:
      p[0] = []

  def p_Import(self, p):
    """Import : IMPORT Name ';'"""
    CreateASTNode(p, Import, p[2])

  def p_ImportList(self, p):
    """ImportList : ImportList Import
                  |"""
    if len(p) == 3:
      p[0] = p[1] + [p[2]]
    else:
      p[0] = []

  def p_File(self, p):
    """File : Package ImportList Interface"""
    CreateASTNode(p, File, p[1], p[2], p[3])

  def p_error(self, p):
    if p:
      msg = "Unexpected token"
      lineno = p.lineno
      lexpos = p.lexpos
      out = self.lexer.ErrorMessage(lineno, lexpos, msg)
      sys.stderr.write(out + '\n')
    else:
      sys.stderr.write('Unexpected eof\n')
    raise Exception("Can't parse idl file! (%s)" % self.filename)


  precedence = (
    ('nonassoc', '*', '&'),
    ('nonassoc', 'CONST'),
    ('nonassoc', '?'),
  )

  def __init__(self, debug=False):
    self.lexer = lexer.IDLLexer()
    self.tokens = self.lexer.tokens
    self.precedence = IDLParser.precedence

    self.__yaccobj = yacc.yacc(module=self,
                               tabmodule=None,
                               start='File',
                               debug=debug,
                               optimize=0,
                               write_tables=0)

  def ParseText(self, filename, data):
    self.filename = filename

    try:
      self.lexer.Input(data, filename)
      f = self.__yaccobj.parse(lexer=self.lexer.GetLexer())
      f.filename = filename
      f.index = self.lexer.index
      f.lines = self.lexer.lines
      return f
    except lex.LexError as lexError:
      sys.stderr.write('Error in token: %s\n' % str(lexError))
    return None

  def ParseFile(self, filename, path = None):
    if path == None:
      path = filename

    with open(path) as f:
      return self.ParseText(filename, f.read())
