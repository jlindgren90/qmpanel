project('qmpanel', 'cpp', version: '0.1')

qt5 = import('qt5')
mocs = qt5.compile_moc(headers: [
  'panel/statusnotifier/sniasync.h',
  'panel/statusnotifier/statusnotifieriteminterface.h',
  'panel/statusnotifier/statusnotifierproxy.h',
  'panel/statusnotifier/statusnotifierwatcher.h',
])

srcs = [
  mocs,
  'panel/actionview.cpp',
  'panel/clocklabel.cpp',
  'panel/main.cpp',
  'panel/mainmenu.cpp',
  'panel/mainpanel.cpp',
  'panel/quicklaunch.cpp',
  'panel/resources.cpp',
  'panel/statusnotifier/dbustypes.cpp',
  'panel/statusnotifier/sniasync.cpp',
  'panel/statusnotifier/statusnotifierbutton.cpp',
  'panel/statusnotifier/statusnotifieriteminterface.cpp',
  'panel/statusnotifier/statusnotifierproxy.cpp',
  'panel/statusnotifier/statusnotifierwatcher.cpp',
  'panel/statusnotifier/statusnotifierwidget.cpp',
  'panel/systray.cpp',
  'panel/taskbar.cpp',
  'panel/taskbutton.cpp',
  'panel/trayicon.cpp',
]

deps = [
  dependency('threads'),
  dependency('gio-2.0'),
  dependency('gio-unix-2.0'),
  dependency('qt5', modules: ['Core', 'Gui', 'Widgets',
                              'Concurrent', 'DBus', 'X11Extras']),
  dependency('dbusmenu-qt5'),
  dependency('KF5WindowSystem'),
  dependency('xcb'),
  dependency('xcomposite'),
  dependency('xdamage'),
  dependency('xrender'),
]

# these are harmless and will be addressed later
add_global_arguments('-Wno-deprecated-declarations', language : 'cpp')

executable('qmpanel', srcs, dependencies: deps, install: true)
