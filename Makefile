Q := @
MAKE := make
BUILD_DIR := .build
CONFIG_H := config/config.h


all:
	$(Q)cmake -S . -B $(BUILD_DIR)
	$(Q)$(MAKE) -C $(BUILD_DIR)

build:
	$(Q)$(MAKE) -C $(BUILD_DIR)

install:
	$(Q)$(MAKE) -C $(BUILD_DIR) install

uninstall:
	$(Q)echo "Uninstall the project..."
	$(Q)xargs rm < $(BUILD_DIR)/install_manifest.txt

clean:
	$(Q)rm -rf $(BUILD_DIR)
	$(Q)rm -f $(CONFIG_H)
