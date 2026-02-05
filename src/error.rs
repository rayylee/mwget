use thiserror::Error;

#[derive(Error, Debug)]
pub enum MwgetError {
    #[error("HTTP error: {0}")]
    Http(#[from] reqwest::Error),

    #[error("IO error: {0}")]
    Io(#[from] std::io::Error),

    #[error("Invalid URL: {0}")]
    InvalidUrl(String),

    #[error("Download failed: {0}")]
    DownloadFailed(String),

    #[error("File write error: {0}")]
    FileWrite(String),

    #[error("Network error: {0}")]
    Network(String),

    #[error("Server does not support range requests")]
    RangeNotSupported,

    #[error("Invalid content length")]
    InvalidContentLength,

    #[error("URL is required")]
    UrlRequired,

    #[error("Invalid argument: {0}")]
    InvalidArgument(String),
}

pub type Result<T> = std::result::Result<T, MwgetError>;
