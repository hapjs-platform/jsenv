#!/usr/bin/env python

# Copyright (c) 2021, the hapjs-platform Project Contributors
# SPDX-License-Identifier: EPL-1.0

import getopt
import sys

class OptionsObject:
  def __getitem__(self, name):
    return self.__dict__[name]

  def __setitem__(self, name, value):
    self.__dict__[name] = value

  def __str__(self):
    return str(self.__dict__)

class Option:
  def __init__(self, short_opt, long_opt, has_value, def_val = None):
    self.short = short_opt
    self.long = long_opt
    self.default = def_val
    self.has_value = has_value
    self.option = ('-' + short_opt, '--' + long_opt)

  def AddOption(self, short_opts, long_opts):
    if self.has_value:
      short_opts.append('-%s:' % self.short)
      long_opts.append('%s=' % self.long)
    else:
      short_opts.append('-' + self.short)
      long_opts.append(self.long)


  def CheckValue(self, out_obj, opt_name, opt_value):
    if opt_name in self.option:
      if self.has_value:
        out_obj[self.long] = opt_value
      else:
        out_obj[self.long] = True
      return True
    return False

  def InitValue(self, out_obj):
    if self.has_value:
      if self.default:
        out_obj[self.long] = self.default
    else:
      out_obj[self.long] = False


def parse_options(argv, options):
  shorts = []
  longs = []
  for opt in options:
    opt.AddOption(shorts, longs)
  shorts_str = ''.join(shorts)
  opts,args = getopt.getopt(argv, shorts_str, longs)
  out = OptionsObject()
  for opt in options:
    opt.InitValue(out)
  for opt_name, opt_value in opts:
    for opt in options:
      if (opt.CheckValue(out, opt_name, opt_value)):
        break
  out.args = args
  return out
