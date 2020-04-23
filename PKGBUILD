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
	"libdbusmenu-qt5" "libxcomposite" "lxmenu-data"
	"lxqt-globalkeys" "solid" "libxcb"
)
optdepends=(
	"libpulse: Volume control plugin"
	"alsa-lib: Volume control plugin"
	"libstatgrab: CPU monitor and Network monitor plugins"
	"libsysstat: System Statistics plugin"
	"lm_sensors: Sensors plugin"
)
makedepends=(
	"lxqt-build-tools" "liblxqt" "libpulse" "libstatgrab" "libsysstat"
	"lm_sensors" "libxdamage" "alsa-lib"
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
