use clap::Parser;
use mwget::{Cli, Downloader, RecursiveDownloader, Result};

#[tokio::main]
async fn main() -> Result<()> {
    let cli = Cli::parse();

    if cli.version {
        println!("mwget {}", env!("CARGO_PKG_VERSION"));
        println!("A multi-threaded wget implementation in Rust");
        return Ok(());
    }

    let config = cli.to_config()?;

    if config.recursive {
        let mut recursive_downloader = RecursiveDownloader::new(config)?;
        recursive_downloader.download().await?;
    } else {
        let downloader = Downloader::new(config)?;
        downloader.download().await?;
    }

    Ok(())
}
