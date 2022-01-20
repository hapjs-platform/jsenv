import parser
import visitor
from loader import IDLLoader

loader = IDLLoader(['.'])

loader.Load('test.idl')

loader.Dump()
