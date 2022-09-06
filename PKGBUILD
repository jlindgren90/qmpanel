# Maintainer: John Lindgren <john@jlindgren.net>

pkgname=qmpanel
pkgver=0.1
pkgrel=1
pkgdesc="A Minimal Qt-Based Desktop Panel"
arch=("x86_64")
url="https://github.com/jlindgren90/qmpanel"
license=("LGPL2.1")
makedepends=("cmake")
depends=(
	"glib2"
	"kwindowsystem"
	"libxcb"
	"libxcomposite"
	"libxdamage"
	"libxrender"
)

build() {
	mkdir -p ../build
	cd ../build
	cmake .. -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_EXPORT_COMPILE_COMMANDS=1
	sed -i 's/-fvar-tracking-assignments//' compile_commands.json # for clangd
	make
}

package() {
	cd ../build
	make DESTDIR="$pkgdir" install
}
