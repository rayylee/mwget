use clap::Parser;
use mwget::{Cli, Downloader, RecursiveDownloader, Result};
use std::env;

#[tokio::main]
async fn main() -> Result<()> {
    // Manual parsing of two-character short arguments
    let args: Vec<String> = env::args().collect();
    let mut processed_args = Vec::new();

    let mut i = 0;
    while i < args.len() {
        match args[i].as_str() {
            "-np" => {
                processed_args.push("--no-parent".to_string());
            }
            "-nH" => {
                processed_args.push("--no-host-directories".to_string());
            }
            _ => {
                processed_args.push(args[i].clone());
            }
        }
        i += 1;
    }

    let cli = Cli::parse_from(&processed_args);

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
