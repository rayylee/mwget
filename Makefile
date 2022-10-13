Q := @
MAKE := make
BUILD_DIR := .build
CONFIG_H := config/config.h


all:
	$(Q)[ -d $(BUILD_DIR) ] || mkdir -p $(BUILD_DIR)
	$(Q)cd $(BUILD_DIR) && cmake ..
	$(Q)$(MAKE) VERBOSE=1 -C $(BUILD_DIR)

build:
	$(Q)$(MAKE) VERBOSE=1 -C $(BUILD_DIR)

install:
	$(Q)$(MAKE) -C $(BUILD_DIR) install

uninstall:
	$(Q)echo "Uninstall the project..."
	$(Q)xargs rm -v < $(BUILD_DIR)/install_manifest.txt

clean:
	$(Q)rm -rf -v $(BUILD_DIR)
	$(Q)rm -f -v $(CONFIG_H)
