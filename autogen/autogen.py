#!/usr/bin/python3
# Autogeneration Utilities for CLOSURE
#
from   clang.cindex  import Index, TokenKind
from   lark.lexer    import Lexer, Token
from   argparse      import ArgumentParser
from   lark          import Lark, Tree
from   lark.visitors import Transformer
import json
import sys
import os

# Invoke libclang tokenizer
def cindex_tokenizer(f,a):
  return Index.create().parse(f,args=a).cursor.get_tokens()

# Transform tokens for CLE parsing
class TypeLexer(Lexer):
  def __init__(self, lexer_conf): pass
  def lex(self, data):
    for x in data:
      print (x.spelling)
      if x.kind == TokenKind.PUNCTUATION:
        if   x.spelling == '{': yield Token('OPENBRACES', x)
        elif x.spelling == '}': yield Token('CLOSEBRACES', x)
        elif x.spelling == ';': yield Token('SEMICOLON', x)
        #elif x.spelling == '[': yield Token('OPENBRACKET', x)
        #elif x.spelling == ']': yield Token('CLOSEBRACKET', x)
        #elif x.spelling == ';': yield Token('COLON', x)
        else:                   yield Token('PUNCTUATION', x)
      elif x.kind == TokenKind.IDENTIFIER:
        yield Token('IDENTIFIER', x)
      elif x.kind == TokenKind.LITERAL:
        yield Token('LITERAL', x)
      elif x.kind == TokenKind.COMMENT:
        yield Token('COMMENT', x)
      elif x.kind == TokenKind.KEYWORD:
        if   x.spelling == 'struct':   yield Token('STRUCT', x)
        elif x.spelling == 'unsigned': yield Token('UNSIGNED', x)
        elif x.spelling == 'char':     yield Token('CHAR', x)
        elif x.spelling == 'short':    yield Token('SHORT', x)
        elif x.spelling == 'int':      yield Token('INT', x)
        elif x.spelling == 'long':     yield Token('LONG', x)
        elif x.spelling == 'float':    yield Token('FLOAT', x)
        elif x.spelling == 'double':   yield Token('DOUBLE', x)
        else:                          yield Token('KEYWORD', x)
      else:
        raise TypeError(x)

# Grammar and parser for IDL
def idl_parser():
  return Lark(r"""
    datlst:      dat_item+
    ?dat_item:   struct structname openbraces field+ closebraces semicolon
    field:      basictype fieldname semicolon
    ?basictype:  double
                 | float
                 | int8
                 | uint8
                 | int16
                 | uint16
                 | int32
                 | uint32
                 | int64
                 | uint64
    double:      DOUBLE
    float:       FLOAT
    int8:        CHAR
    uint8:       UNSIGNED CHAR
    int16:       SHORT
    uint16:      UNSIGNED SHORT
    int32:       INT
    uint32:      UNSIGNED INT
    int64:       LONG
    uint64:      UNSIGNED LONG
    openbraces:  OPENBRACES
    closebraces: CLOSEBRACES
    semicolon:   SEMICOLON
    structname:  IDENTIFIER
    struct:      STRUCT
    fieldname:   IDENTIFIER
    other:       PUNCTUATION
                 | IDENTIFIER
                 | LITERAL
                 | KEYWORD
                 | COMMENT
    %declare PUNCTUATION IDENTIFIER LITERAL KEYWORD COMMENT OPENBRACES CLOSEBRACES SEMICOLON STRUCT UNSIGNED CHAR SHORT INT LONG FLOAT DOUBLE
  """, start='datlst', parser='lalr', lexer=TypeLexer)

'''
def deraw(s):
  return s.replace('R"JSON(','').replace(')JSON"','').replace('\n','')

# Tranform parsed tree to extract relevant IDL information
class IDLTransformer(Transformer):
  def _hlp(self, items):
    return ' '.join([x.value.spelling for x in items if isinstance(x, Token)])
  def acode(self, items):    return [i for s in items for i in s]
  def other(self, items):    return []
  def begin(self, items):    return []
  def end(self, items):      return []
  def deff(self, items):     return []
  def pfx(self, items):      return items[0].value.extent.start.line
  def label(self, items):    return self._hlp(items)
  def clejson(self, items):  return json.loads(deraw(self._hlp(items)))
  def cledef(self, items):   return [['cledef'] + items]
  def clebegin(self, items): return [['clebegin'] + items]
  def cleend(self, items):   return [['cleend'] + items]
  def cleappnl(self, items): return [['cleappnl'] + items]

'''
    
# Parse command line argumets
def get_args():
  p = ArgumentParser(description='CLOSURE Autogeneration Utility')
  p.add_argument('-i', '--idl_file', required=True, type=str, help='Input IDL file')
  #p.add_argument('-j', '--json_cle_file', required=True, type=str, help='Input CLE-JSON file')
  p.add_argument('-c', '--clang_args', required=False, type=str, 
                 default='-x,c++,-stdlib=libc++', help='Arguments for clang')
  return p.parse_args()

# Create and invoke tokenizer, parser, tree transformer, and source transformer
def main():
  args   = get_args()
  print('Options selected:')
  for x in vars(args).items(): print('  %s: %s' % x)
  
  toks   = cindex_tokenizer(args.idl_file, args.clang_args.split(','))
  tree   = idl_parser().parser.parse(toks)
  print(tree.pretty())

  '''
  print('Transformed Tree:')
  ttree  = IDETransformer().transform(tree)
  for x in ttree: print(x)
  '''

if __name__ == '__main__':
  main()
