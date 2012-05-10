#! /usr/bin/env python
# encoding: utf-8

import os

# Necessary since we override different Contexts 
import waflib.extras.wurftools as wt

LIBNAME = 'libblockbuster'
VERSION = '0.1'

aaudir = os.path.abspath('..')

wt.add_dependency('kodo', 'git://github.com/steinwurf/kodo.git', '1.0.3')

wt.add_dependency('sak', 'git://github.com/steinwurf/sak.git', '1.0.0')

wt.add_dependency('fifi', 'git://github.com/steinwurf/fifi.git', '1.0.1')

wt.add_dependency('boost', 'git://github.com/steinwurf/external-boost.git',
                  '1.1.0-boost_1_48_0')

wt.add_dependency('gtest', 'git://github.com/steinwurf/external-gtest.git',
                  '1.0.1-gtest_1_6_0')

def load_helper(ctx, name):
    if ctx.is_system_dependency(name):
        ctx.fatal('Load a tool to find %s as system dependency' % name)
    else:
        ctx.load_dependency(name)

def options(opt):
    opt.load('wurftools')

    load_helper(opt, 'sak')
    load_helper(opt, 'fifi')
    load_helper(opt, 'boost')
    load_helper(opt, 'kodo')
    load_helper(opt, 'gtest')

def configure(conf):

    conf.load('wurftools')

    load_helper(conf, 'sak')
    load_helper(conf, 'fifi')
    load_helper(conf, 'boost')
    load_helper(conf, 'kodo')
    load_helper(conf, 'gtest')
   
 
def build(bld):

    load_helper(bld, 'sak')
    load_helper(bld, 'fifi')
    load_helper(bld, 'boost')
    load_helper(bld, 'kodo')
    load_helper(bld, 'gtest')

    cxxflags = []

    if bld.env.TOOLCHAIN == 'linux':
        cxxflags += ['-O2', '-g', '-ftree-vectorize',
                     '-Wextra', '-Wall']

    if bld.env.TOOLCHAIN == 'win32':
        cxxflags += ['/O2', '/Ob2', '/W3', '/EHsc']



    bld.stlib(features = 'cxx',
    		target	= 'blockbuster',
    		source	= [ os.path.join('..','server','kodo_encoder.cpp'), 
    				os.path.join('..','node','kodo_decoder.cpp'),
    				os.path.join('..','postoffice','Postoffice.cpp'),
    				os.path.join('..','serializer','serializer.cpp'),
    				'blockbuster.cpp'],
    		includes = ['..', '/home/jeppe/git/steinwurf/external-ffmpeg/ffmpeg'],
    		defines = '__STDC_CONSTANT_MACROS',    				
    		cxxflags = cxxflags,
    		use	= [ 'kodo_includes', 'boost_includes',
    				'fifi_includes', 'sak_includes'])
    				
    				
    				
    				
    				
    				
    				
    				
    				
    				
