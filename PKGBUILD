# Maintainer: Jerome Leclanche <jerome@leclan.ch>

pkgname=lxqt-panel
pkgver=0.14.1
pkgrel=2
pkgdesc="The LXQt desktop panel"
arch=("x86_64")
groups=("lxqt")
url="https://lxqt.org"
license=("LGPL2.1")
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
	cmake .. -DCMAKE_INSTALL_PREFIX=/usr
	make
}

package() {
	cd ../build
	make DESTDIR="$pkgdir" install
}
