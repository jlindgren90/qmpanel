# Maintainer: John Lindgren <john@jlindgren.net>

pkgname=qmpanel
pkgver=0.5
pkgrel=1
pkgdesc="A Minimal Qt-Based Desktop Panel"
arch=("x86_64")
url="https://github.com/jlindgren90/qmpanel"
license=("LGPL2.1")
makedepends=(
  "meson"
  "cmake" # needed to find layer-shell-qt
)
depends=(
  "glib2"
  "kwindowsystem"
  "layer-shell-qt"
  "libxcb"
)

build() {
  cd ..
  arch-meson build
  meson compile -C build
}

package() {
  cd ..
  meson install -C build --destdir "$pkgdir"
}
