project('app', 'cpp',
  version : '0.1',
  default_options : ['warning_level=3', 'cpp_std=c++17']
)

pwd = meson.current_source_dir()
cc = meson.get_compiler('cpp')

add_global_arguments('-w', language: 'cpp')

textmate_inc = include_directories(
    'libs/tm-parser/textmate/parser',
    'libs/tm-parser/textmate/theme',
    'libs/tm-parser/textmate/scopes'
)

textmate_files = [
    'libs/tm-parser/textmate/parser/grammar.cpp',
    'libs/tm-parser/textmate/parser/parser.cpp',
    'libs/tm-parser/textmate/parser/pattern.cpp',
    'libs/tm-parser/textmate/parser/reader.cpp',
    'libs/tm-parser/textmate/scopes/match.cpp',
    'libs/tm-parser/textmate/scopes/parse.cpp',
    'libs/tm-parser/textmate/scopes/scope.cpp',
    'libs/tm-parser/textmate/scopes/types.cpp',
    'libs/tm-parser/textmate/theme/theme.cpp',
    'libs/tm-parser/textmate/theme/util.cpp'
]

editor_inc = [
    'libs/editor'
]

editor_files = [
    'libs/editor/app.cpp',
    'libs/editor/block.cpp',
    'libs/editor/cursor.cpp',
    'libs/editor/document.cpp',
    'libs/editor/dots.cpp',
    'libs/editor/editor.cpp',
    'libs/editor/explorer.cpp',
    'libs/editor/extension.cpp',
    'libs/editor/highlighter.cpp',
    'libs/editor/icons.cpp',
    'libs/editor/indexer.cpp',
    'libs/editor/operation.cpp',
    'libs/editor/search.cpp',
    'libs/editor/snapshot.cpp',
    'libs/editor/statusbar.cpp',
    'libs/editor/util.cpp'
]

curses_dep = dependency('ncursesw', required: false, disabler: true)
if not curses_dep.found()
  curses_root = get_option('curses_root')
  curses_lib = cc.find_library('cursesw', dirs : curses_root, required : false, disabler: true)
  curses_dep = declare_dependency(include_directories: curses_root, dependencies: curses_lib)
endif

pthread_dep = cc.find_library('pthread', required : true, disabler: true)
sdl2_dep = dependency('SDL2', required: false, disabler: true)
cairo_dep = dependency('cairo', required: false, disabler: true)
pango_dep = dependency('pangocairo', required: false, disabler: true)
jsoncpp_dep = dependency('jsoncpp', required: true)
onigmo_dep = dependency('onigmo', required: true)
dl_dep = cc.find_library('dl', required : true) 

quickjs_lib = cc.find_library('quickjs', required: false, dirs:['/usr/lib/quickjs', '/usr/local/lib/quickjs'])
if build_machine.system() == 'darwin'
    quickjs_dep = declare_dependency(dependencies: [ dl_dep, quickjs_lib ], include_directories: ['/usr/local/include/quickjs'])
else
    quickjs_dep = declare_dependency(dependencies: [ dl_dep, quickjs_lib ], include_directories: ['/usr/include/quickjs'])
endif

ui_files = [
    'src/renderer.cpp',
    'src/render_cache.cpp',
    'src/layout.cpp',
    'src/events.cpp',
    'src/view.cpp',
    'src/ui/editor_view.cpp',
    'src/ui/explorer_view.cpp',
    'src/ui/text.cpp',
    'src/ui/button.cpp',
    'src/ui/scrollarea.cpp',
    'src/ui/scrollbar.cpp',
    'src/ui/panel.cpp',
    'src/tests.cpp'
]

library('tmparser',
    textmate_files, 
    include_directories: [ textmate_inc ],
    dependencies: [ jsoncpp_dep, onigmo_dep ]
)

tmparser_lib = cc.find_library('tmparser', required: false, dirs:[pwd + '/build'])
tmparser_dep = declare_dependency(dependencies: [ tmparser_lib ], include_directories: [ textmate_inc ])

editor_lib = library('editor',
    editor_files, 
    include_directories: [ editor_inc ],
    dependencies: [ tmparser_dep, jsoncpp_dep, onigmo_dep, pthread_dep ]
)

editor_lib = cc.find_library('editor', required: false, dirs:[pwd + '/build'])
editor_dep = declare_dependency(dependencies: [ editor_lib ], include_directories: [ editor_inc ])

executable('app',
    'src/main.cpp',
    ui_files,
    include_directories: [ './src', './src/ui' ],
    dependencies: [ tmparser_dep, editor_dep, sdl2_dep, cairo_dep, pango_dep ],
    install : true
)