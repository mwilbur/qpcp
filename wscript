top = '.'
out = 'build'

import os

def set_options(opt):
    opt.tool_options('compiler_cc')

def configure(conf):
    conf.check_tool('compiler_cc')
    conf.env.QP = os.path.abspath('../qp')

def build(bld):
    
    print(bld.env.QP)

    bld( features = ['cc','cprogram'],
         source = ['bsp.c','main.c','eeprom.c','isr.c'],
         cflags = ['-O0','-gdwarf-2'],
         target = 'cmd_proc.elf',
         includes = [ os.path.join(bld.env.QP,'qpc/include'),
                     os.path.join(bld.env.QP,'qpc/ports/80x86/linux/gnu')],
         libs = ['qf','qep','pthread'],
         libpath = [os.path.join(bld.env.QP,'qpc/ports/80x86/linux/gnu/dbg')]

         )
