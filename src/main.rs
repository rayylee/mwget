use clap::Parser;
use mwget::{Cli, Downloader, Result};

#[tokio::main]
async fn main() -> Result<()> {
    let cli = Cli::parse();
    let config = cli.to_config();

    let downloader = Downloader::new(config)?;
    downloader.download().await?;

    Ok(())
}
