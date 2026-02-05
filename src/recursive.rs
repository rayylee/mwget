use crate::config::DownloadConfig;
use crate::error::{MwgetError, Result};
use crate::http::HttpClient;
use scraper::{Html, Selector};
use std::collections::{HashSet, VecDeque};
use url::Url;

pub struct RecursiveDownloader {
    config: DownloadConfig,
    client: HttpClient,
    base_url: Url,
    visited_urls: HashSet<String>,
}

impl RecursiveDownloader {
    pub fn new(config: DownloadConfig) -> Result<Self> {
        let base_url = Url::parse(&config.url)
            .map_err(|e| MwgetError::DownloadFailed(format!("Invalid URL: {}", e)))?;

        let client = HttpClient::new(config.clone())?;

        Ok(Self {
            config,
            client,
            base_url,
            visited_urls: HashSet::new(),
        })
    }

    pub async fn download(&mut self) -> Result<()> {
        let mut url_queue = VecDeque::new();
        url_queue.push_back(self.base_url.clone());

        while let Some(current_url) = url_queue.pop_front() {
            let url_str = current_url.to_string();

            if self.visited_urls.contains(&url_str) {
                continue;
            }

            self.visited_urls.insert(url_str.clone());

            if !self.config.quiet {
                eprintln!("Downloading: {}", url_str);
            }

            // Download the current URL
            self.download_current(&current_url).await?;

            // If it's an HTML file, extract links and add them to queue
            if self.is_html_content(&current_url).await? {
                let links = self.extract_links(&current_url).await?;
                for link in links {
                    if self.should_download(&link, &current_url) {
                        url_queue.push_back(link);
                    }
                }
            }
        }

        Ok(())
    }

    async fn download_current(&self, url: &Url) -> Result<()> {
        // Create a temporary config for this specific URL
        let mut url_config = self.config.clone();
        url_config.url = url.to_string();

        // Use the regular downloader for this URL
        let downloader = crate::downloader::Downloader::new(url_config)?;
        downloader.download().await
    }

    async fn is_html_content(&self, url: &Url) -> Result<bool> {
        let response = self
            .client
            .head_url(url.as_ref())
            .await
            .map_err(|e| MwgetError::DownloadFailed(format!("HEAD request failed: {}", e)))?;

        let content_type = response
            .headers()
            .get("content-type")
            .and_then(|v| v.to_str().ok())
            .unwrap_or("");

        Ok(content_type.contains("text/html"))
    }

    async fn extract_links(&self, url: &Url) -> Result<Vec<Url>> {
        let response = self
            .client
            .get_url(url.as_ref(), None)
            .await
            .map_err(|e| MwgetError::DownloadFailed(format!("GET request failed: {}", e)))?;

        let html_content = response
            .text()
            .await
            .map_err(|e| MwgetError::DownloadFailed(format!("Failed to read response: {}", e)))?;

        let document = Html::parse_document(&html_content);
        let selector = Selector::parse("a[href]").unwrap();

        let mut links = Vec::new();

        for element in document.select(&selector) {
            if let Some(href) = element.value().attr("href")
                && let Ok(link_url) = self.resolve_url(url, href)
            {
                links.push(link_url);
            }
        }

        Ok(links)
    }

    fn resolve_url(&self, base_url: &Url, href: &str) -> Result<Url> {
        base_url
            .join(href)
            .map_err(|e| MwgetError::DownloadFailed(format!("Invalid URL resolution: {}", e)))
    }

    fn should_download(&self, url: &Url, base_url: &Url) -> bool {
        // Only download from the same domain
        if url.host_str() != self.base_url.host_str() {
            return false;
        }

        // Apply no-parent restriction
        if self.config.no_parent
            && let Some(base_path) = base_url.path_segments()
            && let Some(url_path) = url.path_segments()
        {
            // Check if URL tries to go to parent directory
            let mut base_components: Vec<_> = base_path.collect();
            let url_components: Vec<_> = url_path.collect();

            // Remove the file name from base path
            if !base_components.is_empty() {
                base_components.pop();
            }

            // URL should not be shorter than base path (no parent access)
            if url_components.len() < base_components.len() {
                return false;
            }

            // Check that the prefix matches
            for (i, base_comp) in base_components.iter().enumerate() {
                if url_components.get(i) != Some(base_comp) {
                    return false;
                }
            }
        }

        // Skip common non-content files
        let path = url.path();
        let skip_extensions = [
            ".css", ".js", ".ico", ".png", ".jpg", ".jpeg", ".gif", ".svg",
        ];
        if skip_extensions
            .iter()
            .any(|&ext| path.to_lowercase().ends_with(ext))
        {
            return false;
        }

        true
    }
}
