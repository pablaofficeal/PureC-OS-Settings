ROOT_DIR ?= $(abspath ../..)
include $(ROOT_DIR)/mk/toolchain.mk

NAME := settings
VERSION := $(shell cat VERSION)
PKGREL ?= 1
OUT := $(BIN_DIR)/$(NAME)
OBJ := $(OUT)/obj
LIB_DIR := $(BIN_DIR)/lib
LIBGUI_DIR ?= $(ROOT_DIR)/PureC-OS-LibGui
VB_DIR ?= $(ROOT_DIR)/video-backend
LIBPG := $(BIN_DIR)/libgui/libpg.a
LIBVB := $(BIN_DIR)/video-backend/libvb.a
LIBPUREC := $(LIB_DIR)/libpurec.a
CRT0 := $(LIB_DIR)/crt0.o
LINKER_SCRIPT := $(ROOT_DIR)/linker-userspace.ld

CFLAGS := $(CPPFLAGS) -I$(LIBC_DIR)/include/hosted -I$(LIBGUI_DIR)/include -I$(VB_DIR)/include \
	$(USER_CFLAGS) -O2 -Wall -Wextra $(DEPFLAGS)

OBJECTS := $(patsubst src/%.c,$(OBJ)/%.o,$(wildcard src/*.c))
TARGET := $(OUT)/$(NAME)

.PHONY: all clean package
all: $(TARGET)

$(OBJ)/%.o: src/%.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET): $(OBJECTS) $(LIBPG) $(LIBVB) $(CRT0) $(LIBPUREC) $(LINKER_SCRIPT)
	$(LD) -T $(LINKER_SCRIPT) -o $@ $(CRT0) $(OBJECTS) $(LIBPG) $(LIBVB) $(LIBPUREC)

$(LIBPG):
	$(MAKE) -C $(LIBGUI_DIR) ROOT_DIR=$(ROOT_DIR)

$(LIBVB):
	$(MAKE) -C $(VB_DIR) ROOT_DIR=$(ROOT_DIR)

$(CRT0) $(LIBPUREC):
	$(MAKE) -C $(LIBC_DIR)

PKG_FILE := $(OUT)/$(NAME)-$(VERSION)-$(PKGREL)-x86_64.pkg.tar.gz
SIGN_KEY ?= $(HOME)/.config/purec-pkg/signing.pem

package: all
	rm -f $(OUT)/$(NAME)-*.pkg.tar.gz*
	python3 $(ROOT_DIR)/mk/pkg/mkpkg.py --name $(NAME) --version $(VERSION)-$(PKGREL) \
		--desc "System settings" -o $(OUT) \
		--depend "video-backend>=1.1.0" \
		--file usr/bin/$(NAME)=$(TARGET) \
		--file usr/share/applications/$(NAME).desktop=$(CURDIR)/$(NAME).desktop
	if [ -f "$(SIGN_KEY)" ]; then python3 $(ROOT_DIR)/mk/pkg/pkgsign.py sign $(SIGN_KEY) $(PKG_FILE) $(PKG_FILE).sig; fi
	@ls -l $(OUT)/$(NAME)-*

clean:
	rm -rf $(OUT)

-include $(shell find $(OBJ) -name '*.d' 2>/dev/null)
