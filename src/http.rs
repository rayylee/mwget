use crate::config::DownloadConfig;
use crate::error::{MwgetError, Result};
use reqwest::{Client, RequestBuilder, Response};
use std::collections::HashMap;
use std::sync::Arc;
use std::time::Duration;
use tokio::sync::Mutex as tokio_mutex;
use tokio::time::timeout as tokio_timeout;

#[derive(Clone)]
pub struct CachedHead {
    pub status_code: u16,
    pub status_text: String,
    pub headers: HashMap<String, String>,
}

pub struct HttpClient {
    client: Client,
    config: DownloadConfig,
    cached_head: Arc<tokio_mutex<Option<CachedHead>>>,
}

impl HttpClient {
    pub fn new(config: DownloadConfig) -> Result<Self> {
        let builder = Client::builder()
            .user_agent(&config.user_agent)
            .tls_danger_accept_invalid_certs(config.no_check_certificate)
            .redirect(reqwest::redirect::Policy::limited(10));

        let client = builder.build()?;

        Ok(Self {
            client,
            config,
            cached_head: Arc::new(tokio_mutex::new(None)),
        })
    }

    async fn send_with_timeout(&self, request: RequestBuilder) -> Result<Response> {
        let timeout_duration = Duration::from_secs(self.config.timeout);

        match tokio_timeout(timeout_duration, request.send()).await {
            Ok(response) => Ok(response?),
            Err(_) => Err(MwgetError::DownloadFailed(format!(
                "Timeout after {} seconds (no data received)",
                self.config.timeout
            ))),
        }
    }

    async fn get_cached_head(&self) -> Result<CachedHead> {
        let mut cache = self.cached_head.lock().await;

        if cache.is_none() {
            let mut request = self.client.head(&self.config.url);

            for (key, value) in &self.config.headers {
                request = request.header(key, value);
            }

            let response = self.send_with_timeout(request).await?;
            let status = response.status();

            // Convert headers to HashMap
            let mut headers = HashMap::new();
            for (name, value) in response.headers() {
                if let Ok(value_str) = value.to_str() {
                    headers.insert(name.as_str().to_lowercase(), value_str.to_string());
                }
            }

            let cached = CachedHead {
                status_code: status.as_u16(),
                status_text: status.canonical_reason().unwrap_or("Unknown").to_string(),
                headers,
            };

            *cache = Some(cached);
        }

        Ok(cache.as_ref().unwrap().clone())
    }

    pub async fn head_url(&self, url: &str) -> Result<Response> {
        let mut request = self.client.head(url);

        for (key, value) in &self.config.headers {
            request = request.header(key, value);
        }

        let response = self.send_with_timeout(request).await?;
        Ok(response)
    }

    pub async fn get(&self, range: Option<(u64, u64)>) -> Result<Response> {
        self.get_url(&self.config.url, range).await
    }

    pub async fn get_url(&self, url: &str, range: Option<(u64, u64)>) -> Result<Response> {
        let mut request = self.client.get(url);

        for (key, value) in &self.config.headers {
            request = request.header(key, value);
        }

        if let Some((start, end)) = range {
            request = request.header("Range", format!("bytes={}-{}", start, end));
        }

        let response = self.send_with_timeout(request).await?;

        if !response.status().is_success() && !response.status().is_redirection() {
            return Err(MwgetError::Http(response.error_for_status().unwrap_err()));
        }

        Ok(response)
    }

    pub async fn get_content_length(&self) -> Result<Option<u64>> {
        let cached = self.get_cached_head().await?;
        Ok(cached
            .headers
            .get(&reqwest::header::CONTENT_LENGTH.to_string().to_lowercase())
            .and_then(|v| v.parse().ok()))
    }

    pub async fn supports_range(&self) -> Result<bool> {
        let cached = self.get_cached_head().await?;
        Ok(cached
            .headers
            .get(&reqwest::header::ACCEPT_RANGES.to_string().to_lowercase())
            .map(|v| v != "none")
            .unwrap_or(false))
    }

    pub async fn get_content_type(&self) -> Result<String> {
        let cached = self.get_cached_head().await?;
        Ok(cached
            .headers
            .get(&reqwest::header::CONTENT_TYPE.to_string().to_lowercase())
            .cloned()
            .unwrap_or_else(|| "text/html".to_string()))
    }

    pub async fn get_content_encoding(&self) -> Result<Option<String>> {
        let cached = self.get_cached_head().await?;
        Ok(cached
            .headers
            .get(&reqwest::header::CONTENT_ENCODING.to_string().to_lowercase())
            .cloned())
    }

    pub async fn get_status_info(&self) -> Result<(u16, String)> {
        let cached = self.get_cached_head().await?;
        Ok((cached.status_code, cached.status_text))
    }
}
