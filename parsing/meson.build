pgen = generator(bison_bin,
    output : ['@BASENAME@.cpp', '@BASENAME@.h'] ,
    arguments : ['@INPUT@', '--defines=@OUTPUT1@', '--output=@OUTPUT0@'])
parser_files = pgen.process('parser.yy')
lgen = generator(flex_bin,
    output : '@BASENAME@.cpp',
    arguments : ['-o', '@OUTPUT@', '@INPUT@'])
scanner_files = lgen.process('scanner.ll')

formula_files = ['formula.cpp','formula.h']

lexer_exe = executable('formula_lexer', [formula_files, parser_files, scanner_files, 'driver.cpp'],
    install : true, dependencies : [boost, fmt])


#rofl_exe = executable('formula_rep', [formula_files], install:true, dependencies: [boost, fmt])