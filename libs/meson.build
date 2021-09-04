textmate_inc = include_directories(
    'tm-parser/textmate/parser',
    'tm-parser/textmate/theme',
    'tm-parser/textmate/scopes'
)

textmate_files = [
    'tm-parser/textmate/parser/grammar.cpp',
    'tm-parser/textmate/parser/parser.cpp',
    'tm-parser/textmate/parser/pattern.cpp',
    'tm-parser/textmate/parser/reader.cpp',
    'tm-parser/textmate/scopes/match.cpp',
    'tm-parser/textmate/scopes/parse.cpp',
    'tm-parser/textmate/scopes/scope.cpp',
    'tm-parser/textmate/scopes/types.cpp',
    'tm-parser/textmate/theme/theme.cpp',
    'tm-parser/textmate/theme/util.cpp'
]

editor_inc = [
    'libs/editor'
]

editor_files = [
    'libs/editor/app.cpp',
    'libs/editor/backend.cpp',
    'libs/editor/block.cpp',
    'libs/editor/cursor.cpp',
    'libs/editor/document.cpp',
    'libs/editor/dots.cpp',
    'libs/editor/editor.cpp',
    'libs/editor/explorer.cpp',
    'libs/editor/extension.cpp',
    'libs/editor/highlighter.cpp',
    'libs/editor/indexer.cpp',
    'libs/editor/operation.cpp',
    'libs/editor/search.cpp',
    'libs/editor/snapshot.cpp',
    'libs/editor/statusbar.cpp',
    'libs/editor/util.cpp'
]

pthread_dep = cc.find_library('pthread', required : true, disabler: true)
jsoncpp_dep = dependency('jsoncpp', required: true)
onigmo_dep = dependency('onigmo', required: true)
dl_dep = cc.find_library('dl', required : true) 

tmparser_lib = cc.find_library('tmparser', required: false, dirs:[pwd + '/build'])
tmparser_dep = declare_dependency(dependencies: [ tmparser_lib ], include_directories: [ textmate_inc ])

editor_lib = cc.find_library('editor', required: false, dirs:[pwd + '/build'])
editor_dep = declare_dependency(dependencies: [ editor_lib ], include_directories: [ editor_inc ])

library('tmparser',
    textmate_files, 
    include_directories: [ textmate_inc ],
    dependencies: [ jsoncpp_dep, onigmo_dep ]
)

library('editor',
    editor_files, 
    include_directories: [ editor_inc ],
    dependencies: [ tmparser_dep, jsoncpp_dep, onigmo_dep, pthread_dep ]
)

