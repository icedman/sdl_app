project('app', 'cpp',
  version : '0.1',
  default_options : ['warning_level=3', 'cpp_std=c++11']
)

add_global_arguments([
        '-w',
        '-D_XOPEN_SOURCE_EXTENDED',
    ], language: 'cpp')

add_global_arguments('-DFONT_PREBAKE_GLYPHS', language: 'cpp')
# add_global_arguments('-DWATCH_LEAKS', language: 'cpp')

if build_machine.system() == 'darwin'
#    add_global_arguments('-DFONT_FORCE_DRAW_PREBAKED_GLYPHS', language: 'cpp')    
     add_global_arguments('-DFONT_FIX_FIXED_WIDTH_EXTENTS', language: 'cpp')    
     add_global_arguments('-Wno-c++11-narrowing', language: 'cpp')    
endif

if host_machine.system() == 'windows'
add_global_arguments([
        '-DWIN64',
    ], language: 'cpp')
endif

if get_option('enable_scripting')
add_global_arguments([
        '-DENABLE_SCRIPTING'
    ], language: 'cpp')
endif
if get_option('enable_svg')
add_global_arguments([
        '-DENABLE_SVG'
    ], language: 'cpp')
endif

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
    'libs/editor/utf8.cpp',
    'libs/editor/util.cpp'
]

framework_inc = [
    'libs/framework',
    'libs/framework/common',
    'libs/framework/renderer',
    'libs/framework/system',
    'libs/framework/ui'
]

framework_files = [
    'libs/framework/common/color.cpp',
    'libs/framework/common/rect.cpp',
    'libs/framework/common/utf8.cpp',
    'libs/framework/common/hash.cpp',
    'libs/framework/system/system.cpp',
    'libs/framework/system/events.cpp',
    'libs/framework/system/tasks.cpp',
    'libs/framework/renderer/renderer.cpp',
    'libs/framework/renderer/pango_font.cpp',
    'libs/framework/renderer/fallback_font.cpp',
    'libs/framework/ui/styled_frame.cpp',
    'libs/framework/ui/layout.cpp',
    'libs/framework/ui/view.cpp',
    'libs/framework/ui/damage.cpp',
    'libs/framework/ui/button.cpp',
    'libs/framework/ui/image.cpp',
    'libs/framework/ui/icons.cpp',
    'libs/framework/ui/list.cpp',
    'libs/framework/ui/panel.cpp',
    'libs/framework/ui/popup.cpp',
    'libs/framework/ui/scrollarea.cpp',
    'libs/framework/ui/scrollbar.cpp',
    'libs/framework/ui/splitter.cpp',
    'libs/framework/ui/statusbar.cpp',
    'libs/framework/ui/tabbar.cpp',
    'libs/framework/ui/text.cpp',
    'libs/framework/ui/text_block.cpp'
]

if get_option('build_curses')
curses_dep = dependency('ncursesw', required: false, disabler: true)
if not curses_dep.found()
  curses_root = get_option('curses_root')
  curses_lib = cc.find_library('cursesw', dirs : curses_root, required : false, disabler: true)
  curses_dep = declare_dependency(include_directories: curses_root, dependencies: curses_lib)
endif
endif

pthread_dep = cc.find_library('pthread', required : true, disabler: true)
sdl2_dep = dependency('SDL2', required: false, disabler: true)
sdl2main_dep = cc.find_library('SDL2main', required: false, disabler: true)
cairo_dep = dependency('cairo', required: false, disabler: true)
pango_dep = dependency('pangocairo', required: false, disabler: true)
fontconfig_dep = dependency('fontconfig', required: false, disabler: true)
jsoncpp_dep = dependency('jsoncpp', required: true)
onigmo_dep = dependency('onigmo', required: true)

if get_option('enable_svg')
    rsvg_lib = cc.find_library('rsvg-2', required: false)
    if build_machine.system() == 'darwin'
        rsvg_dep = declare_dependency(dependencies: [ rsvg_lib ], include_directories: ['/usr/local/include/librsvg-2.0', '/usr/local/include/gdk-pixbuf-2.0'])
    elif build_machine.system() == 'windows'
        rsvg_dep = declare_dependency(dependencies: [ rsvg_lib ], include_directories: ['c:/msys64/mingw64/include/librsvg-2.0', 'c:/msys64/mingw64/include/gdk-pixbuf-2.0'])
    else
        rsvg_dep = declare_dependency(dependencies: [ rsvg_lib ], include_directories: ['/usr/include/librsvg-2.0', '/usr/include/gdk-pixbuf-2.0'])
    endif
else
    rsvg_dep = []
endif

if get_option('enable_scripting')
    dl_dep = cc.find_library('dl', required : true) 
    if build_machine.system() == 'darwin'
        quickjs_lib = cc.find_library('quickjs', required: false, dirs:['/usr/lib/quickjs', '/usr/local/lib/quickjs'])
        quickjs_dep = declare_dependency(dependencies: [ dl_dep, quickjs_lib ], include_directories: ['/usr/local/include/quickjs'])
    elif build_machine.system() == 'windows'
        quickjs_lib = cc.find_library('quickjs', required: false, dirs:['c:/msys64/home/iceman/app/libs/quickjs'])
        quickjs_dep = declare_dependency(dependencies: [ dl_dep, quickjs_lib ], include_directories: ['c:/msys64/home/iceman/app/libs/quickjs'])
    else
        quickjs_lib = cc.find_library('quickjs', required: false, dirs:['/usr/lib/quickjs', '/usr/local/lib/quickjs'])
        quickjs_dep = declare_dependency(dependencies: [ dl_dep, quickjs_lib ], include_directories: ['/usr/include/quickjs'])
    endif
endif

if get_option('enable_scripting')
scripting_dep = [quickjs_dep]
else
scripting_dep = []
endif

tmparser_lib = static_library('tmparser',
    textmate_files, 
    include_directories: [ textmate_inc ],
    dependencies: [ jsoncpp_dep, onigmo_dep ]
)

tmparser_dep = declare_dependency(link_with: tmparser_lib, include_directories: [ textmate_inc ])

editor_lib = static_library('editor',
    editor_files, 
    include_directories: [ editor_inc ],
    dependencies: [ tmparser_dep, jsoncpp_dep, onigmo_dep, pthread_dep ]
)

editor_dep = declare_dependency(link_with: editor_lib, include_directories: [ editor_inc ])

framework_lib = static_library('framework',
    framework_files, 
    include_directories: [ framework_inc ],
    dependencies: [ cairo_dep, pango_dep, fontconfig_dep, rsvg_dep, jsoncpp_dep ]
)

framework_dep = declare_dependency(link_with: framework_lib, include_directories: [ framework_inc ])

if get_option('build_sdl')
executable('ashlar',
    'src/rich_text.cpp',
    'src/input_text.cpp',
    'src/completer_view.cpp',
    'src/commands_view.cpp',
    'src/search_view.cpp',
    'src/prompt_view.cpp',
    'src/app_view.cpp',
    'src/explorer_view.cpp',
    'src/editor_view.cpp',
    'src/gutter.cpp',
    'src/minimap.cpp',
    'src/main.cpp',
    'src/tests.cpp',
    framework_files,
    include_directories: [ './src', 'libs/PennerEasing' ],
    dependencies: [
        tmparser_dep,
        editor_dep,
        framework_dep,
        sdl2main_dep,
        sdl2_dep,
        cairo_dep,
        pango_dep,
        fontconfig_dep,
        rsvg_dep,
        scripting_dep
    ],
    win_subsystem: 'windows',
    install : true
)
endif

if get_option('build_curses')
executable('ashlr',
    'src/main.cpp',
    curses_files,
    framework_files,
    include_directories: [ './src' ],
    dependencies: [ tmparser_dep, editor_dep, curses_dep, scripting_dep ],
    install : true
)
endif
