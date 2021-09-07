project('cppmon', 'cpp',
  version : '0.1',
  default_options : ['warning_level=3',
                     'cpp_std=c++17'])

#add_global_arguments('-fsanitize=address,undefined', language : 'cpp')

fmt = dependency('fmt')
boost = dependency('boost')
benchmark = dependency('benchmark')
gtest = dependency('gtest')
flex_bin = find_program('flex', required: true)
bison_bin = find_program('bison', required: true)

subdir('parsing')

table_src = ['table.h', 'table.cpp']

sources = ['cppmon.cpp', table_src]
exe = executable('cppmon', sources,
  install : true, dependencies : [fmt])

table_test_src = [table_src, 'tabletest.cpp']
table_test = executable('table_test', table_test_src, dependencies: [fmt])
test('Table correctness', table_test)

table_bench_src = [table_src, 'tablebench.cpp']
table_bench = executable('table_bench', table_bench_src, dependencies: [fmt,gtest,benchmark])