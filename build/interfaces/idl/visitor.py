def visit(*args):
  def decorate(c):
    c.members = args
    return c
  return decorate

class ASTVisitor:
  def __init__(self):
    self.__namespace = []

  @property
  def namespace(self):
    return '.'.join(self.__namespace)

  def Enter(self, node):
    pass

  def Exit(self, node):
    pass

  def EnterNamespace(self, name):
    self.__namespace.append(name)

  def ExitNamespace(self):
    self.__namespace.pop()

  def Visit(self, n):
    if n == None:
      return

    if isinstance(n, list):
      for e in n:
        self.Visit(e)
      return

    self.Enter(n)
    members = getattr(n.__class__, 'members', ())
    for m in members:
      if m == '+':
        self.EnterNamespace(n.name.name)
      elif m == '-':
        self.ExitNamespace()
      else:
        self.Visit(getattr(n, m, None))
    self.Exit(n)
