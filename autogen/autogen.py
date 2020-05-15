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
                 | other
    field:       basictype fieldname semicolon
    ?basictype:  double
                 | ffloat
                 | int8
                 | uint8
                 | int16
                 | uint16
                 | int32
                 | uint32
                 | int64
                 | uint64
    double:      DOUBLE
    ffloat:      FLOAT
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

flatten = lambda l: [item for sublist in l for item in sublist]
flatone = lambda l: [i for i in l if (not isinstance(i,list)) or len(i) != 0]

# Tranform parsed tree to extract relevant IDL information
class IDLTransformer(Transformer):
  def _hlp(self, items):
    return ' '.join([x.value.spelling for x in items if isinstance(x, Token)])
  def datlst(self, items):      return flatone(items)
  def dat_item(self, items):    return flatone(items)
  def structname(self, items):  return items[0].value.spelling
  def field(self, items):       return flatten(items)
  def fieldname(self, items):   return [items[0].value.spelling]
  def basictype(self,items):    return [items]
  def double(self,items):       return [items[0].type]
  def ffloat(self,items):       return [items[0].type]
  def int8(self,items):         return [items[0].type]
  def uint8(self,items):        return [items[0].type]
  def int16(self,items):        return [items[0].type]
  def uint16(self,items):       return [items[0].type]
  def int32(self,items):        return [items[0].type]
  def uint32(self,items):       return [items[0].type]
  def int64(self,items):        return [items[0].type]
  def uint64(self,items):       return [items[0].type]
  def openbraces(self,items):   return []
  def closebraces(self,items):  return []
  def semicolon(self,items):    return []
  def struct(self,items):       return []
  def other(self, items):       return []

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

  print('Transformed Tree:')
  ttree  = IDLTransformer().transform(tree)
  for x in ttree: print(x)

if __name__ == '__main__':
  main()
