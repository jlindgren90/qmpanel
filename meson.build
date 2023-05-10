project('qmpanel', 'cpp', version: '0.1')

srcs = [
  'panel/actionview.cpp',
  'panel/clocklabel.cpp',
  'panel/main.cpp',
  'panel/mainmenu.cpp',
  'panel/mainpanel.cpp',
  'panel/quicklaunch.cpp',
  'panel/resources.cpp',
  'panel/systray.cpp',
  'panel/taskbar.cpp',
  'panel/taskbutton.cpp',
  'panel/trayicon.cpp',
]

deps = [
  dependency('threads'),
  dependency('gio-2.0'),
  dependency('gio-unix-2.0'),
  dependency('qt5', modules: ['Core', 'Gui', 'Widgets', 'X11Extras']),
  dependency('KF5WindowSystem'),
  dependency('xcb'),
  dependency('xcomposite'),
  dependency('xdamage'),
  dependency('xrender'),
]

# these are harmless and will be addressed later
add_global_arguments('-Wno-deprecated-declarations', language : 'cpp')

executable('qmpanel', srcs, dependencies: deps, install: true)
