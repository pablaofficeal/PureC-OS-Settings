ROOT_DIR ?= $(abspath ../..)
BIN_DIR ?= $(ROOT_DIR)/bin
BUILD := $(BIN_DIR)/settings-build
OUT := $(BIN_DIR)/repo
VERSION := $(shell cat VERSION)
PKGREL ?= 1
JOBS ?= 4
PKG_FILE := $(OUT)/settings-$(VERSION)-$(PKGREL)-x86_64.pkg.tar.gz
SIGN_KEY ?= $(HOME)/.config/purec-pkg/signing.pem

.PHONY: all configure package clean

all: $(BUILD)/build.ninja
	nice -n 19 cmake --build $(BUILD) --parallel $(JOBS)

$(BUILD)/build.ninja: CMakeLists.txt
	BIN_DIR=$(BIN_DIR) cmake -G Ninja -S $(CURDIR) -B $(BUILD) \
		-DCMAKE_TOOLCHAIN_FILE=$(ROOT_DIR)/mk/sysroot/toolchain.cmake \
		-DCMAKE_BUILD_TYPE=Release \
		-DQT_HOST_PATH=/usr -DQT_HOST_PATH_CMAKE_DIR=/usr/lib/cmake

package: all
	@mkdir -p $(OUT)
	rm -f $(OUT)/settings-*.pkg.tar.gz*
	python3 $(ROOT_DIR)/mk/pkg/mkpkg.py --name settings --version $(VERSION)-$(PKGREL) \
		--desc "System settings (Qt)" --depend "video-backend>=1.1.0" -o $(OUT) \
		--file usr/bin/settings=$(BUILD)/settings \
		--file usr/share/applications/settings.desktop=$(CURDIR)/settings.desktop \
		$(foreach f,DejaVuSans.ttf DejaVuSans-Bold.ttf,--file usr/share/settings/fonts/$(f)=$(ROOT_DIR)/assets/fonts/$(f))
	if [ -f "$(SIGN_KEY)" ]; then python3 $(ROOT_DIR)/mk/pkg/pkgsign.py sign $(SIGN_KEY) $(PKG_FILE) $(PKG_FILE).sig; fi
	@ls -l $(OUT)/settings-*

clean:
	rm -rf $(BUILD)
