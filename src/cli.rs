use crate::config::DownloadConfig;
use crate::error::{MwgetError, Result};
use clap::Parser;
use std::path::PathBuf;

#[derive(Parser, Debug)]
#[command(name = "mwget")]
#[command(about = "A multi-threaded wget implementation in Rust", long_about = None)]
#[command(version = env!("CARGO_PKG_VERSION"))]
#[command(disable_version_flag = true)]
pub struct Cli {
    /// Display the version of Wget and exit
    #[arg(short = 'V', long = "version")]
    pub version: bool,

    /// URL to download
    #[arg(value_name = "URL")]
    pub url: Option<String>,

    /// Write output to FILE
    #[arg(short = 'O', long = "output-document", value_name = "FILE")]
    pub output: Option<PathBuf>,

    /// Continue getting a partially-downloaded file
    #[arg(short = 'c', long = "continue")]
    pub continue_download: bool,

    /// Set network timeout to SECONDS
    #[arg(
        short = 'T',
        long = "timeout",
        value_name = "SECONDS",
        default_value = "60"
    )]
    pub timeout: u64,

    /// Set number of retries to NUMBER
    #[arg(
        short = 't',
        long = "tries",
        value_name = "NUMBER",
        default_value = "20"
    )]
    pub tries: usize,

    /// Number of concurrent connections
    #[arg(short = 'n', long = "number", value_name = "NUM", default_value = "4")]
    pub concurrent: usize,

    /// Turn off output
    #[arg(short = 'q', long = "quiet")]
    pub quiet: bool,

    /// Be verbose
    #[arg(short = 'v', long = "verbose")]
    pub verbose: bool,

    /// Identify as AGENT-STRING
    #[arg(short = 'U', long = "user-agent", value_name = "AGENT-STRING")]
    pub user_agent: Option<String>,

    /// Add custom header
    #[arg(long = "header", value_name = "STRING")]
    pub header: Vec<String>,
}

impl Cli {
    pub fn to_config(self) -> Result<DownloadConfig> {
        let concurrent = self.concurrent;

        let url = self.url.ok_or(MwgetError::UrlRequired)?;
        let url = if url.contains("://") {
            url
        } else {
            format!("http://{}", url)
        };

        let headers = self
            .header
            .iter()
            .filter_map(|h| {
                let parts: Vec<&str> = h.splitn(2, ':').collect();
                if parts.len() == 2 {
                    Some((parts[0].trim().to_string(), parts[1].trim().to_string()))
                } else {
                    None
                }
            })
            .collect();

        Ok(DownloadConfig {
            url,
            output: self.output,
            continue_download: self.continue_download,
            timeout: self.timeout,
            retry: self.tries,
            concurrent,
            quiet: self.quiet,
            verbose: self.verbose,
            user_agent: self.user_agent.unwrap_or_else(|| "wget/1.21.3".to_string()),
            headers,
        })
    }
}
