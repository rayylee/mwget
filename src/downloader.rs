use crate::config::DownloadConfig;
use crate::error::{MwgetError, Result};
use crate::formatter;
use crate::http::HttpClient;
use crate::progress::ProgressTracker;
use futures::stream::StreamExt;
use std::fs::OpenOptions;
use std::io::{Seek, SeekFrom, Write};
use std::path::Path;
use std::sync::Arc;
use tokio::sync::Semaphore;
use url::Url;

pub struct Downloader {
    config: DownloadConfig,
    client: HttpClient,
}

#[derive(Debug)]
struct DownloadMetadata {
    hostname: String,
    port: String,
    server_ip: String,
    content_type: String,
    status_code: u16,
    status_text: String,
    total_size: Option<u64>,
}

impl Downloader {
    pub fn new(config: DownloadConfig) -> Result<Self> {
        let client = HttpClient::new(config.clone())?;
        Ok(Self { config, client })
    }

    fn get_hostname(&self) -> String {
        if let Ok(url) = Url::parse(&self.config.url) {
            url.host_str().unwrap_or("unknown").to_string()
        } else {
            "unknown".to_string()
        }
    }

    async fn resolve_ip(&self, host: &str, port: &str) -> Result<String> {
        let addr = format!("{}:{}", host, port);
        let mut addrs = tokio::net::lookup_host(&addr)
            .await
            .map_err(|e| MwgetError::DownloadFailed(format!("DNS resolution failed: {}", e)))?;
        if let Some(sock_addr) = addrs.next() {
            Ok(sock_addr.ip().to_string())
        } else {
            Ok("unknown".to_string())
        }
    }

    fn get_port(&self) -> String {
        if let Ok(url) = Url::parse(&self.config.url) {
            match url.scheme() {
                "https" => "443".to_string(),
                "http" => "80".to_string(),
                _ => "unknown".to_string(),
            }
        } else {
            "unknown".to_string()
        }
    }

    async fn fetch_download_metadata(&self) -> Result<DownloadMetadata> {
        let hostname = self.get_hostname();
        let port = self.get_port();
        let server_ip = self
            .resolve_ip(&hostname, &port)
            .await
            .unwrap_or_else(|_| "unknown".to_string());

        // Get content length and other metadata
        let content_length = self.client.get_content_length().await?;
        let content_type = self.client.get_content_type().await?;
        let content_encoding = self.client.get_content_encoding().await?;
        let (status_code, status_text) = self.client.get_status_info().await?;

        // If content is compressed, the HEAD Content-Length is not reliable
        let total_size = if content_encoding.is_some() {
            None // Use streaming download for compressed content
        } else {
            content_length
        };

        Ok(DownloadMetadata {
            hostname,
            port,
            server_ip,
            content_type,
            status_code,
            status_text,
            total_size,
        })
    }

    fn log_connection_info(&self, metadata: &DownloadMetadata) {
        if !self.config.quiet {
            eprintln!(
                "--{}--  {}",
                chrono::Local::now().format("%Y-%m-%d %H:%M:%S"),
                self.config.url
            );
            eprintln!(
                "Resolving {} ({})... {}",
                metadata.hostname, metadata.hostname, metadata.server_ip
            );
            eprintln!(
                "Connecting to {} ({})|{}|:{}... connected.",
                metadata.hostname, metadata.hostname, metadata.server_ip, metadata.port
            );
        }
    }

    fn log_response_info(&self, metadata: &DownloadMetadata, save_target: &str) {
        if !self.config.quiet {
            eprintln!(
                "HTTP request sent, awaiting response... {} {}",
                metadata.status_code, metadata.status_text
            );
            if let Some(size) = metadata.total_size {
                eprintln!(
                    "Length: {} ({}) [{}]",
                    size,
                    formatter::format_bytes_wget(size),
                    metadata.content_type
                );
            } else {
                eprintln!("Length: unspecified [{}]", metadata.content_type);
            }
            eprintln!("Saving to: '{}'", save_target);
            eprintln!();
        }
    }



    fn print_download_summary(
        &self,
        target: &str,
        downloaded_bytes: u64,
        elapsed: std::time::Duration,
    ) {
        if !self.config.quiet {
            let speed = downloaded_bytes as f64 / elapsed.as_secs_f64();
            let speed_str = crate::formatter::format_speed_wget(speed);

            eprintln!(
                "{} ({}) - '{}' saved [{}]",
                chrono::Local::now().format("%Y-%m-%d %H:%M:%S"),
                speed_str,
                target,
                downloaded_bytes
            );
        }
    }

    pub async fn download(&self) -> Result<()> {
        // Handle special case: -O - means output to stdout
        if self.config.output_to_stdout() {
            return self.download_to_stdout().await;
        }

        let filename = self.config.get_filename();
        let mut output_path = self
            .config
            .output
            .clone()
            .unwrap_or_else(|| Path::new(&filename).to_path_buf());

        // Handle filename conflicts by numbering if file exists and not continuing
        if !self.config.continue_download && output_path.exists() {
            let original_path = output_path.clone();
            let filename = original_path
                .file_name()
                .unwrap_or_default()
                .to_string_lossy();
            let mut counter = 1;
            loop {
                let new_name = format!("{}.{}", filename, counter);
                output_path = original_path.with_file_name(new_name);
                if !output_path.exists() {
                    break;
                }
                counter += 1;
            }
        }

        // Fetch metadata and log connection info
        let metadata = self.fetch_download_metadata().await?;
        self.log_connection_info(&metadata);

        // Log response info
        self.log_response_info(&metadata, &output_path.display().to_string());

        // Handle continuation
        let start_pos = if self.config.continue_download && output_path.exists() {
            std::fs::metadata(&output_path)?.len()
        } else {
            0
        };

        // Check if file is already complete (only if we know total size)
        if let Some(total) = metadata.total_size
            && start_pos >= total
        {
            if !self.config.quiet {
                eprintln!("File already fully retrieved.");
            }
            return Ok(());
        }

        // Decide download strategy
        if let Some(total) = metadata.total_size {
            // Use concurrent download only if we know total size and file is large enough
            // Note: we need to re-check range support for concurrent download
            let supports_range = self.client.supports_range().await?;
            if supports_range && self.config.concurrent > 1 && total > 1024 * 1024 {
                self.concurrent_download(&output_path, start_pos, total)
                    .await
            } else {
                self.single_download(&output_path, start_pos, Some(total))
                    .await
            }
        } else {
            // Use streaming download when total size is unknown
            self.streaming_download(&output_path, start_pos).await
        }
    }

    async fn single_download(
        &self,
        output_path: &Path,
        start_pos: u64,
        total_size: Option<u64>,
    ) -> Result<()> {
        let range = if start_pos > 0 {
            total_size.map(|total| (start_pos, total - 1))
        } else {
            None
        };

        let response = self.client.get(range).await?;
        let progress = Arc::new(ProgressTracker::new(
            total_size,
            &output_path.display().to_string(),
            self.config.quiet,
        ));

        let mut file = OpenOptions::new()
            .create(true)
            .write(true)
            .append(start_pos > 0)
            .open(output_path)?;

        if start_pos > 0 {
            file.seek(SeekFrom::Start(start_pos))?;
            progress.inc(start_pos);
        }

        let mut stream = response.bytes_stream();

        while let Some(chunk) = stream.next().await {
            let chunk = chunk?;
            file.write_all(&chunk)?;
            progress.inc(chunk.len() as u64);
        }

        file.flush()?;
        progress.finish();

        // Print final summary line like wget
        if !self.config.quiet {
            let actual_size = std::fs::metadata(output_path)?.len();
            self.print_summary(output_path, Some(actual_size), progress.elapsed());
        }

        Ok(())
    }

    async fn streaming_download(&self, output_path: &Path, start_pos: u64) -> Result<()> {
        let response = self.client.get(None).await?;
        let progress = Arc::new(ProgressTracker::new(
            None,
            &output_path.display().to_string(),
            self.config.quiet,
        ));

        let mut file = OpenOptions::new()
            .create(true)
            .write(true)
            .append(start_pos > 0)
            .open(output_path)?;

        if start_pos > 0 {
            file.seek(SeekFrom::Start(start_pos))?;
            progress.inc(start_pos);
        }

        let mut stream = response.bytes_stream();

        while let Some(chunk) = stream.next().await {
            let chunk = chunk?;
            file.write_all(&chunk)?;
            progress.inc(chunk.len() as u64);
        }

        file.flush()?;
        progress.finish();

        // Get final file size for summary
        let final_size = std::fs::metadata(output_path)?.len();

        // Print final summary line like wget
        if !self.config.quiet {
            self.print_summary(output_path, Some(final_size), progress.elapsed());
        }

        Ok(())
    }

    async fn concurrent_download(
        &self,
        output_path: &Path,
        start_pos: u64,
        total_size: u64,
    ) -> Result<()> {
        let remaining = total_size - start_pos;
        let chunk_size = remaining.div_ceil(self.config.concurrent as u64);

        let progress = Arc::new(ProgressTracker::new(
            Some(total_size),
            &output_path.display().to_string(),
            self.config.quiet,
        ));

        if start_pos > 0 {
            progress.inc(start_pos);
        }

        // Create or open file
        let file = OpenOptions::new()
            .create(true)
            .truncate(true)
            .write(true)
            .open(output_path)?;

        file.set_len(total_size)?;
        drop(file);

        let semaphore = Arc::new(Semaphore::new(self.config.concurrent));
        let mut tasks = Vec::new();

        for i in 0..self.config.concurrent {
            let chunk_start = start_pos + i as u64 * chunk_size;
            let chunk_end = std::cmp::min(chunk_start + chunk_size - 1, total_size - 1);

            if chunk_start >= total_size {
                break;
            }

            let client = HttpClient::new(self.config.clone())?;
            let config = self.config.clone();
            let output_path = output_path.to_path_buf();
            let progress = Arc::clone(&progress);
            let semaphore = Arc::clone(&semaphore);

            let task = tokio::spawn(async move {
                let _permit = semaphore.acquire().await.unwrap();
                Self::download_chunk(
                    client,
                    &config,
                    output_path,
                    chunk_start,
                    chunk_end,
                    progress,
                )
                .await
            });

            tasks.push(task);
        }

        // Wait for all tasks to complete
        for task in tasks {
            task.await
                .map_err(|e| MwgetError::DownloadFailed(e.to_string()))??;
        }

        progress.finish();

        // Print final summary line like wget
        if !self.config.quiet {
            self.print_summary(output_path, Some(total_size), progress.elapsed());
        }

        Ok(())
    }

    async fn download_chunk(
        client: HttpClient,
        config: &DownloadConfig,
        output_path: std::path::PathBuf,
        start: u64,
        end: u64,
        progress: Arc<ProgressTracker>,
    ) -> Result<()> {
        let mut retries = 0;
        let max_retries = if config.retry == 0 {
            usize::MAX
        } else {
            config.retry
        };

        loop {
            match Self::try_download_chunk(&client, output_path.as_path(), start, end, &progress)
                .await
            {
                Ok(_) => return Ok(()),
                Err(e) => {
                    retries += 1;
                    if retries >= max_retries {
                        return Err(e);
                    }
                    tokio::time::sleep(tokio::time::Duration::from_secs(1)).await;
                }
            }
        }
    }

    async fn try_download_chunk(
        client: &HttpClient,
        output_path: &Path,
        start: u64,
        end: u64,
        progress: &Arc<ProgressTracker>,
    ) -> Result<()> {
        let response = client.get(Some((start, end))).await?;
        let mut stream = response.bytes_stream();

        let mut file = OpenOptions::new().write(true).open(output_path)?;
        file.seek(SeekFrom::Start(start))?;

        while let Some(chunk) = stream.next().await {
            let chunk = chunk?;
            file.write_all(&chunk)?;
            progress.inc(chunk.len() as u64);
        }

        file.flush()?;
        Ok(())
    }

    async fn download_to_stdout(&self) -> Result<()> {
        // Fetch metadata and log connection info
        let metadata = self.fetch_download_metadata().await?;
        self.log_connection_info(&metadata);

        // Log response info
        self.log_response_info(&metadata, "STDOUT");

        let start_time = std::time::Instant::now();
        let mut downloaded = 0u64;

        let response = self.client.get(None).await?;
        let mut stream = response.bytes_stream();
        let mut stdout = std::io::stdout();

        while let Some(chunk) = stream.next().await {
            let chunk = chunk?;
            stdout.write_all(&chunk)?;
            downloaded += chunk.len() as u64;
        }

        stdout.flush()?;

        self.print_download_summary("STDOUT", downloaded, start_time.elapsed());

        Ok(())
    }

    fn print_summary(
        &self,
        output_path: &Path,
        total_size: Option<u64>,
        elapsed: std::time::Duration,
    ) {
        let output_str = output_path.display().to_string();
        let filename = output_str.split('/').next_back().unwrap_or("unknown");

        // Get actual file size for speed calculation if total_size is None
        let actual_size = total_size
            .or_else(|| std::fs::metadata(output_path).ok().map(|m| m.len()))
            .unwrap_or(0);

        let speed = actual_size as f64 / elapsed.as_secs_f64();

        let speed_str = if speed >= 1024.0 * 1024.0 {
            format!("{:.1} MB/s", speed / 1024.0 / 1024.0)
        } else if speed >= 1024.0 {
            format!("{:.1} KB/s", speed / 1024.0)
        } else {
            format!("{:.1} B/s", speed)
        };

        let total_display = if let Some(total) = total_size {
            total.to_string()
        } else {
            actual_size.to_string()
        };

        eprintln!(
            "{} ({}) - '{}' saved [{}/{}]",
            chrono::Local::now().format("%Y-%m-%d %H:%M:%S"),
            speed_str,
            filename,
            actual_size,
            total_display
        );
    }
}
