'''pgen = generator(bison_bin,
    output : ['@BASENAME@.cpp', '@BASENAME@.h'] ,
    arguments : ['@INPUT@', '--defines=@OUTPUT1@', '--output=@OUTPUT0@'])
parser_files = pgen.process('parser.yy')
lgen = generator(flex_bin,
    output : '@BASENAME@.cpp',
    arguments : ['-o', '@OUTPUT@', '@INPUT@'])
scanner_files = lgen.process('scanner.ll')'''

parser_includes = include_directories('.')

formula_src = files('formula.cpp')
'''lexer_src = [formula_src, parser_files, scanner_files, files('driver.cpp')]

lexer_exe = executable('formula_lexer', lexer_src,
    install : true, dependencies : [boost, fmt],
    include_directories: [parser_includes])'''


#rofl_exe = executable('formula_rep', [formula_files], install:true, dependencies: [boost, fmt])