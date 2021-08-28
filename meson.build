project('app', 'cpp',
  version : '0.1',
  default_options : ['warning_level=3', 'cpp_std=c++17']
)

pwd = meson.current_source_dir()
cc = meson.get_compiler('cpp')

add_global_arguments('-w', language: 'cpp')

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

app_files = [
    'src/renderer.cpp',
    'src/render_cache.cpp',
    'src/layout.cpp',
    'src/events.cpp',
    'src/view.cpp',
    'src/ui/text.cpp',
    'src/ui/button.cpp',
    'src/tests.cpp',
    'src/main.cpp'
]

executable('app',
    app_files,
    include_directories: [ './src', './src/ui' ],
    dependencies: [ sdl2_dep, cairo_dep, pango_dep ],
    install : true
)