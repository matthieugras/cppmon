project('cppmon', 'cpp',
  version : '0.1',
  default_options : ['warning_level=3',
                     'cpp_std=c++17'])

if get_option('withsan')
  add_global_arguments(['-fsanitize=address,undefined'], language: 'cpp')
  add_global_link_arguments(['-lasan', '-lubsan', '--rtlib=compiler-rt'], language: 'cpp')
endif

#jemalloc = dependency('jemalloc')
json = dependency('nlohmann_json')
ranges = dependency('range-v3')
fmt = dependency('fmt')
boost = dependency('boost')
benchmark = dependency('benchmark')
gtest = dependency('gtest')

absl_dep = dependency('abseil')
#flex_bin = find_program('flex', required: true)
#bison_bin = find_program('bison', required: true)

#subdir('parsing')

main_includes = include_directories('.')

# Table
table_src = files('table.cpp')
table_deps = [fmt,absl_dep]

sources = [files('cppmon.cpp'), table_src]
exe = executable('cppmon', sources,
    install : true, dependencies : [table_deps],
    include_directories: [main_includes])

# Table benchmark
table_bench_src = [table_src, files('tablebench.cpp')]
table_bench = executable('table_bench', table_bench_src,
    dependencies: [table_deps, benchmark])

# Formula definition
formula_src = files('formula.cpp')
formula_deps = [absl_dep, boost, json]

# MFormula definition
mformula_src = [formula_src, table_src]
mformula_deps = [absl_dep,table_deps,boost,formula_deps]

# Tests
test_src = [files('test_main.cpp', 'tabletest.cpp', 'formulatest.cpp', 'mformulatest.cpp'),
    mformula_src, formula_src]
test_deps = [gtest, fmt, mformula_deps, formula_deps, table_deps]
test_exe = executable('test_exe', test_src,
    dependencies: test_deps, include_directories: [main_includes])