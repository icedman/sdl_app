project('app', 'cpp',
  version : '0.1',
  default_options : ['warning_level=3', 'cpp_std=c++11']
)

add_global_arguments('-w', language: 'cpp')

pwd = meson.current_source_dir()
cc = meson.get_compiler('cpp')

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

curses_dep = dependency('ncursesw', required: false, disabler: true)
if not curses_dep.found()
  curses_root = get_option('curses_root')
  curses_lib = cc.find_library('cursesw', dirs : curses_root, required : false, disabler: true)
  curses_dep = declare_dependency(include_directories: curses_root, dependencies: curses_lib)
endif

pthread_dep = cc.find_library('pthread', required : true, disabler: true)
sdl2_dep = dependency('SDL2', required: false, disabler: true)
sdl2ttf_dep = dependency('SDL2_ttf', required: false, disabler: true) 
cairo_dep = dependency('cairo', required: false, disabler: true)
pango_dep = dependency('pangocairo', required: false, disabler: true)
jsoncpp_dep = dependency('jsoncpp', required: true)
onigmo_dep = dependency('onigmo', required: true)
dl_dep = cc.find_library('dl', required : true) 

rsvg_lib = cc.find_library('rsvg-2', required: false, dirs:['/usr/lib/', '/usr/local/lib/'])
if build_machine.system() == 'darwin'
    rsvg_dep = declare_dependency(dependencies: [ dl_dep, rsvg_lib ], include_directories: ['/usr/local/include/librsvg-2.0/librsvg', '/usr/local/include/gdk-pixbuf-2.0'])
else
    rsvg_dep = declare_dependency(dependencies: [ dl_dep, rsvg_lib ], include_directories: ['/usr/include/librsvg-2.0/librsvg', '/usr/include/gdk-pixbuf-2.0'])
endif

quickjs_lib = cc.find_library('quickjs', required: false, dirs:['/usr/lib/quickjs', '/usr/local/lib/quickjs'])
if build_machine.system() == 'darwin'
    quickjs_dep = declare_dependency(dependencies: [ dl_dep, quickjs_lib ], include_directories: ['/usr/local/include/quickjs'])
else
    quickjs_dep = declare_dependency(dependencies: [ dl_dep, quickjs_lib ], include_directories: ['/usr/include/quickjs'])
endif

ui_files = [
    'src/backend/renderer.cpp',
    'src/backend/render_font.cpp',
    'src/backend/render_cache.cpp',
    'src/layout.cpp',
    'src/events.cpp',
    'src/view.cpp',
    'src/ui/app_view.cpp',
    'src/ui/editor_view.cpp',
    'src/ui/explorer_view.cpp',
    'src/ui/statusbar_view.cpp',
    'src/ui/tabbar.cpp',
    'src/ui/text.cpp',
    'src/ui/button.cpp',
    'src/ui/image.cpp',
    'src/ui/scrollarea.cpp',
    'src/ui/scrollbar.cpp',
    'src/ui/panel.cpp',
    'src/tests.cpp'
]

tmparser_lib = library('tmparser',
    textmate_files, 
    include_directories: [ textmate_inc ],
    dependencies: [ jsoncpp_dep, onigmo_dep ]
)

tmparser_dep = declare_dependency(link_with: tmparser_lib, include_directories: [ textmate_inc ])

editor_lib = library('editor',
    editor_files, 
    include_directories: [ editor_inc ],
    dependencies: [ tmparser_dep, jsoncpp_dep, onigmo_dep, pthread_dep ]
)

editor_dep = declare_dependency(link_with: editor_lib, include_directories: [ editor_inc ])

executable('app',
    'src/main.cpp',
    ui_files,
    include_directories: [ './src', './src/ui', './src/backend' ],
    dependencies: [ tmparser_dep, editor_dep, sdl2_dep, cairo_dep, pango_dep, rsvg_dep ],
    install : true
)
