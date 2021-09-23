pgen = generator(bison_bin,
    output : ['@BASENAME@.cpp', '@BASENAME@.h'] ,
    arguments : ['@INPUT@', '--defines=@OUTPUT1@', '--output=@OUTPUT0@'])
lgen = generator(flex_bin,
    output : '@BASENAME@.cpp',
    arguments : ['-o', '@OUTPUT@', '@INPUT@'])
    parser_files = pgen.process('parser.yy')
scanner_files = lgen.process('scanner.ll')

parser_includes = include_directories('.')
parser_src = [parser_files, scanner_files, files('driver.cpp')]
trace_parser = declare_dependency(sources: parser_src,
                    include_directories: [parser_includes])
