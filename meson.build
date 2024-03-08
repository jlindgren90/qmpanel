project('qmpanel', 'cpp', version: '0.1')

qt6 = import('qt6')
mocs = qt6.compile_moc(headers: [
  'dbusmenu/dbusmenu_interface.h',
  'dbusmenu/dbusmenuimporter.h',
  'panel/statusnotifier/statusnotifieriteminterface.h',
  'panel/statusnotifier/statusnotifierwatcher.h',
])

srcs = [
  mocs,
  'dbusmenu/dbusmenu_interface.cpp',
  'dbusmenu/dbusmenuimporter.cpp',
  'dbusmenu/dbusmenushortcut_p.cpp',
  'dbusmenu/dbusmenutypes_p.cpp',
  'dbusmenu/utils.cpp',
  'panel/actionview.cpp',
  'panel/clocklabel.cpp',
  'panel/main.cpp',
  'panel/mainmenu.cpp',
  'panel/mainpanel.cpp',
  'panel/quicklaunch.cpp',
  'panel/resources.cpp',
  'panel/statusnotifier/dbustypes.cpp',
  'panel/statusnotifier/statusnotifier.cpp',
  'panel/statusnotifier/statusnotifiericon.cpp',
  'panel/statusnotifier/statusnotifieriteminterface.cpp',
  'panel/statusnotifier/statusnotifierwatcher.cpp',
  'panel/taskbar.cpp',
  'panel/taskbutton.cpp',
]

deps = [
  dependency('threads'),
  dependency('gio-2.0'),
  dependency('gio-unix-2.0'),
  dependency('qt6', modules: ['Core', 'Gui', 'Widgets', 'DBus'],
             private_headers: true),
  dependency('KF6WindowSystem'),
  dependency('xcb'),
  dependency('xcomposite'),
  dependency('xdamage'),
  dependency('xrender'),
]

# these are harmless and will be addressed later
add_global_arguments('-Wno-deprecated-declarations', language : 'cpp')

executable('qmpanel', srcs, dependencies: deps, install: true)
