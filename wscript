# -*- coding: utf-8 -*-


def options(opt):
        opt.load('compiler_cxx')


def configure(conf):
        conf.load('compiler_cxx')
        conf.env.CXXFLAGS = ['-Wall', '-O2', '-g', '--std=c++0x']


def build(bld):
        bld.program(source='langdetect.cpp', target='langdetect', use='langdetect')
        bld.shlib(source=['detector.cpp', 'ngram_storage.cpp', 'code_sequence.cpp', 'const.cpp', 'exception.cpp',
                          'unicode_data.cpp', 'normalizer.cpp'],
                  target='langdetect')
