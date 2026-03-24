NAME := mwget
VERSION := $(shell grep '^version = ' Cargo.toml | cut -d '"' -f 2)
TARGET := x86_64-unknown-linux-gnu
RELEASE_DIR := target/$(TARGET)/release
DIST_DIR := dist
BINARY := $(RELEASE_DIR)/$(NAME)
ARCHIVE_NAME := $(NAME)-$(TARGET)_$(VERSION).tar.gz

all: build

# Install the Rust target if not installed
setup:
	@echo "Setting up target $(TARGET)..."
	@if rustup target list --installed | grep -q "$(TARGET)"; then \
		echo "✓ Target $(TARGET) already installed"; \
	else \
		echo "Installing target $(TARGET)..."; \
		rustup target add $(TARGET); \
		echo "✓ Target $(TARGET) installed"; \
	fi

# Compile the project for the specified target
build: setup
	@echo "Building $(NAME) v$(VERSION) for target $(TARGET)..."
	@cargo build --release --target $(TARGET)
	@echo "✓ Build completed: $(BINARY)"

# Create distribution package
package: build
	@echo "Creating distribution package..."
	@mkdir -p $(DIST_DIR)/$(NAME)-$(TARGET)_$(VERSION)
	@cp $(BINARY) $(DIST_DIR)/$(NAME)-$(TARGET)_$(VERSION)/
	@cd $(DIST_DIR) && tar -czf $(ARCHIVE_NAME) $(NAME)-$(TARGET)_$(VERSION)/
	@echo "✓ Package created: $(DIST_DIR)/$(ARCHIVE_NAME)"

# Install locally
install: build
	@echo "Installing $(NAME) to /usr/local/bin..."
	@cp $(BINARY) /usr/local/bin/$(NAME)
	@echo "✓ Installation completed"

# Uninstall locally
uninstall:
	@echo "Removing $(NAME) from /usr/local/bin..."
	@rm -f /usr/local/bin/$(NAME)
	@echo "✓ Uninstallation completed"

# Clean build artifacts
clean:
	@echo "Cleaning build artifacts..."
	@cargo clean
	@rm -rf $(DIST_DIR)
	@echo "✓ Clean completed"

# Show build info
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
test:
	@echo "Running code formatting check..."
	@cargo fmt --check
	@echo "Running clippy lints..."
	@cargo clippy -- -D warnings
	@echo "Running tests..."
	@cargo test

# Build and run binary (for quick testing)
run: build
	@echo "Running $(BINARY) with --help..."
	@$(BINARY) --help

# Release preparation (build + package + verify)
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
help:
	@echo "=== $(NAME) Makefile ==="
	@echo ""
	@echo "Available targets:"
	@echo "  setup         - Install target"
	@echo "  build         - Build binary"
	@echo "  package       - Create distribution tar.gz package"
	@echo "  install       - Install binary to /usr/local/bin"
	@echo "  uninstall     - Remove binary from /usr/local/bin"
	@echo "  clean         - Clean build artifacts"
	@echo "  test          - Run fmt check, clippy, and cargo tests"
	@echo "  run           - Build and run with --help"
	@echo "  release       - Full release preparation"
	@echo "  info          - Show build information"
	@echo "  help          - Show this help message"
	@echo ""
	@echo "Examples:"
	@echo "  make package                    # Build and create package"
	@echo "  make release                    # Full release preparation"
	@echo "  make build TARGET=x86_64-unknown-linux-musl  # Build for musl"

.PHONY: all setup build package info clean uninstall install help release run test
