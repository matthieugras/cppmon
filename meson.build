project('cppmon', 'cpp',
  version : '0.1',
  default_options : ['warning_level=3',
                     'cpp_std=c++17'])

if get_option('withsan')
  add_global_arguments(['-fsanitize=address,undefined'], language: 'cpp')
  add_global_link_arguments(['-lasan', '-lubsan', '--rtlib=compiler-rt'], language: 'cpp')
endif

abseil = subproject('abseil-cpp')

ranges = dependency('range-v3')
fmt = dependency('fmt')
boost = dependency('boost')
benchmark = dependency('benchmark')
gtest = dependency('gtest')

absl_hash = abseil.get_variable('absl_hash_dep')
absl_container = abseil.get_variable('absl_container_dep')
absl_dep = [absl_hash, absl_container]
flex_bin = find_program('flex', required: true)
bison_bin = find_program('bison', required: true)

subdir('parsing')

main_includes = include_directories('.')

# Table
table_src = files('table.cpp')
table_deps = [fmt,absl_dep]

sources = [files('cppmon.cpp'), table_src]
exe = executable('cppmon', sources,
    install : true, dependencies : [table_deps],
    include_directories: [main_includes])

# Table test
table_test_src = [table_src, files('tabletest.cpp')]
table_test = executable('table_test', table_test_src,
    dependencies: [table_deps], include_directories: [main_includes])
test('Table correctness', table_test)

# Table benchmark
table_bench_src = [table_src, files('tablebench.cpp')]
table_bench = executable('table_bench', table_bench_src,
    dependencies: [table_deps, benchmark])

# MFormula definition
mformula_src = [formula_src]
mformula_deps = [absl_dep,table_deps]
mformula_test_src = [mformula_src, files('mformulatest.cpp')]
mformula_test = executable('mformula_test', mformula_test_src,
    dependencies: mformula_deps,
    include_directories: [main_includes, parser_includes])
test('MFormula correctness', mformula_test)