use clap::Parser;
use mwget::{Cli, Downloader, Result};

#[tokio::main]
async fn main() -> Result<()> {
    let cli = Cli::parse();

    if cli.version {
        println!("mwget {}", env!("CARGO_PKG_VERSION"));
        println!("A multi-threaded wget implementation in Rust");
        return Ok(());
    }

    let config = cli.to_config()?;

    let downloader = Downloader::new(config)?;
    downloader.download().await?;

    Ok(())
}
