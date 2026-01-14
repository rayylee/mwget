# mwget

A high-performance, multi-threaded wget implementation written in Rust.

## Installation

### From Source

```bash
git clone <repository-url>
cd mwget
cargo build --release
```

The compiled binary will be available at `target/release/mwget`.

## Usage

Basic usage is similar to wget:

```bash
# Download a file
mwget https://example.com/file.zip

# Use 8 concurrent connections
mwget -n 8 https://example.com/large-file.iso
```

## Development

### Building

```bash
cargo build
```

### Running with debug output
```bash
RUST_LOG=debug cargo run -- <url>
```

### Testing
```bash
cargo test
```

### Code quality
```bash
cargo clippy
cargo fmt
```

## Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Add tests for new functionality
5. Ensure all tests pass and code follows project conventions
6. Submit a pull request
