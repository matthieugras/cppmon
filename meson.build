project('cppmon', 'cpp',
  version : '0.1',
  default_options : ['warning_level=3',
                     'cpp_std=c++17'])

fmt = dependency('fmt')
boost = dependency('boost')
flex_bin = find_program('flex', required: true)
bison_bin = find_program('bison', required: true)

subdir('parsing')

sources = ['cppmon.cpp', 'table.cpp']
exe = executable('cppmon', sources,
  install : true, dependencies : [fmt])

test('basic', exe)
