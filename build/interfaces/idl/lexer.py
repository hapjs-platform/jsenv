import ply.lex as lex
import sys

class IDLLexer:
  literals = r'*.(){}[],;:=+-/~|&^?<>'

  t_ignore = ' \t'

  tokens = [
    'integer',
    'float',
    'string',
    'char',
    'identifier',
  ]

  keywords = {
    'true' : 'TRUE',
    'false' : 'FALSE',
    'null' : 'NULL',
    'byte' : 'BYTE',
    'char' : 'CHAR',
    'boolean' : 'BOOLEAN',
    'short' : 'SHORT',
    'int' : 'INT',
    'long' : 'LONG',
    'float' : 'FLOAT',
    'double' : 'DOUBLE',
    'string' : 'STRING',
    'void' : 'VOID',
    'unsigned' : 'UNSIGNED',
    'package' : 'PACKAGE',
    'interface' : 'INTERFACE',
    'struct' : 'STRUCT',
    'enum' : 'ENUM',
    'delegate' : 'DELEGATE',
    'property' : 'PROPERTY',
    'readonly' : 'READONLY',
    'static' : 'STATIC',
    'import' : 'IMPORT',
    'map' : 'MAP',
    'set' : 'SET',
    'const' : 'CONST',
    'event': 'EVENT',
  }

  t_float = r'-?(([0-9]+\.[0-9]*|[0-9]*\.[0-9]+)([Ee][+-]?[0-9]+)?|[0-9]+[Ee][+-]?[0-9]+)'

  t_integer = r'-?([1-9][0-9]*|0[Xx][0-9A-Fa-f]+|0[0-7]*)'

  def t_string(self, t):
    r'"[^"]*"'
    self.AddLines(t.value.count('\n'))
    return t

  def t_char(self, t):
    r"'[^']+'"
    self.AddLines(t.value.count('\n'))
    return t

  def t_LINE_END(self, t):
    r'\n+'
    self.AddLines(len(t.value))

  def t_COMMENT(self, t):
    r'(/\*(.|\n)*?\*/)|(//.*(\n[ \t]*//.*)*)'
    self.AddLines(t.value.count('\n'))

  def t_KEYWORD_OR_SYMBOL(self, t):
    r'\$?[A-Za-z_][A-Za-z_0-9]*'
    t.type = self.keywords.get(t.value, 'identifier')

    if t.value[0] == '$':
      t.value = t.value[1:]
    return t

  def t_error(self, t):
    msg = 'Unrecognized input'
    lineno = self.GetLexer().lineno
    lexpos = self.GetLexer().lexpos
    out = self.ErrorMessage(lineno, lexpos, msg)
    sys.stderr.write(out + '\n')
    self.errors += 1

  def ErrorMessage(self, lineno, lexpos, msg):
    return "\n%s\n%s" % (self.FileLineMsg(lineno, msg), self.SourceLine(lineno, lexpos))

  def FileLineMsg(self, lineno, msg):
    filename = self.GetLexer().filename
    if filename:
      return "%s(%d) : %s" % (filename, lineno, msg)
    return "<BuiltIn> : %s" % msg

  def SourceLine(self, lineno, lexpos):
    pos = lexpos - self.index[lineno - 1]
    caret = ' ' * pos + '^'
    return "%s\n%s\n" % (self.lines[lineno - 1], caret)

  def AddLines(self, count):
    self.GetLexer().lineno += count
    for _ in range(count):
      self.index.append(self.GetLexer().lexpos)

  def GetLexer(self):
    return self.__lexobj

  def Input(self, data, filename):
    lexer = self.GetLexer()
    lexer.lineno = 1
    lexer.filename = filename
    lexer.input(data)
    self.lines = data.split('\n')
    self.index = [0]
    self.filename = filename

  def __init__(self):
    self.index = [0]
    self.errors = 0
    self.filename = None
    self.keywords = IDLLexer.keywords
    self.tokens = IDLLexer.tokens + self.keywords.values()
    self.__lexobj = lex.lex(object=self, lextab=None, optimize=0)
    self.lines = None
