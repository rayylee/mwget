# Variables
NAME := mwget
VERSION := $(shell grep '^version = ' Cargo.toml | cut -d '"' -f 2)
TARGET := x86_64-unknown-linux-musl
RELEASE_DIR := target/$(TARGET)/release
DIST_DIR := dist
BINARY := $(RELEASE_DIR)/$(NAME)
ARCHIVE_NAME := $(NAME)-linux-x64_$(VERSION).tar.gz

# Default target
.PHONY: all
all: build

# Install musl target if not present
.PHONY: setup
setup:
	@echo "Setting up musl target..."
	@if rustup target list --installed | grep -q "$(TARGET)"; then \
		echo "✓ Musl target $(TARGET) already installed"; \
	else \
		echo "Installing musl target $(TARGET)..."; \
		rustup target add $(TARGET); \
		echo "✓ Musl target $(TARGET) installed"; \
	fi

# Static build with musl
.PHONY: build
build: setup
	@echo "Building $(NAME) v$(VERSION) for Linux x64 with musl..."
	@export RUSTFLAGS='-C target-feature=+crt-static' && \
	 cargo build --release --target $(TARGET)
	@echo "✓ Build completed: $(BINARY)"

# Check if binary is statically linked
.PHONY: check-static
check-static: build
	@echo "Checking binary linkage..."
	@echo "File type: $(shell file $(BINARY))"
	@if ldd $(BINARY) 2>/dev/null | grep -q "statically linked"; then \
		echo "✓ Binary is statically linked"; \
	else \
		echo "⚠️  Binary has dynamic dependencies"; \
		ldd $(BINARY) 2>/dev/null || true; \
	fi

# Create distribution package
.PHONY: package
package: build check-static
	@echo "Creating distribution package..."
	@mkdir -p $(DIST_DIR)/$(NAME)-$(VERSION)
	@cp $(BINARY) $(DIST_DIR)/$(NAME)-$(VERSION)/
	@cd $(DIST_DIR) && tar -czf $(ARCHIVE_NAME) $(NAME)-$(VERSION)/
	@echo "✓ Package created: $(DIST_DIR)/$(ARCHIVE_NAME)"

# Install locally
.PHONY: install
install: build
	@echo "Installing $(NAME) to /usr/local/bin..."
	@sudo cp $(BINARY) /usr/local/bin/$(NAME)
	@echo "✓ Installation completed"

# Uninstall locally
.PHONY: uninstall
uninstall:
	@echo "Removing $(NAME) from /usr/local/bin..."
	@sudo rm -f /usr/local/bin/$(NAME)
	@echo "✓ Uninstallation completed"

# Clean build artifacts
.PHONY: clean
clean:
	@echo "Cleaning build artifacts..."
	@cargo clean
	@rm -rf $(DIST_DIR)
	@echo "✓ Clean completed"

# Show build info
.PHONY: info
info:
	@echo "=== $(NAME) Build Information ==="
	@echo "Name: $(NAME)"
	@echo "Version: $(VERSION)"
	@echo "Target: $(TARGET)"
	@echo "Binary: $(BINARY)"
	@echo "Archive: $(ARCHIVE_NAME)"
	@echo "Rust version: $(shell rustc --version)"
	@echo "Available targets:"
	@rustup target list | grep "$(TARGET)"

# Run tests
.PHONY: test
test:
	@echo "Running tests..."
	@cargo test

# Build and run binary (for quick testing)
.PHONY: run
run: build
	@echo "Running $(BINARY) with --help..."
	@$(BINARY) --help

# Release preparation (build + package + verify)
.PHONY: release
release: clean package
	@echo "=== Release Summary ==="
	@echo "Version: $(VERSION)"
	@echo "Package: $(DIST_DIR)/$(ARCHIVE_NAME)"
	@if [ -f "$(DIST_DIR)/$(ARCHIVE_NAME)" ]; then \
		echo "Size: $$(du -h $(DIST_DIR)/$(ARCHIVE_NAME) | cut -f1)"; \
		echo "SHA256: $$(sha256sum $(DIST_DIR)/$(ARCHIVE_NAME) | cut -d' ' -f1)"; \
	else \
		echo "❌ Package not found!"; \
		exit 1; \
	fi
	@echo "✓ Release ready for distribution"

# Help
.PHONY: help
help:
	@echo "=== $(NAME) Makefile ==="
	@echo ""
	@echo "Available targets:"
	@echo "  setup         - Install musl target"
	@echo "  build         - Build static binary with musl"
	@echo "  check-static  - Verify binary is statically linked"
	@echo "  package       - Create distribution tar.gz package"
	@echo "  install       - Install binary to /usr/local/bin"
	@echo "  uninstall     - Remove binary from /usr/local/bin"
	@echo "  clean         - Clean build artifacts"
	@echo "  test          - Run cargo tests"
	@echo "  run           - Build and run with --help"
	@echo "  release       - Full release preparation"
	@echo "  info          - Show build information"
	@echo "  help          - Show this help message"
	@echo ""
	@echo "Examples:"
	@echo "  make package                    # Build and create package"
	@echo "  make release                    # Full release preparation"
	@echo "  make build TARGET=i686-unknown-linux-musl  # Build for 32-bit"
