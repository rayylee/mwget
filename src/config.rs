use std::path::PathBuf;

#[derive(Debug, Clone)]
pub struct DownloadConfig {
    pub url: String,
    pub output: Option<PathBuf>,
    pub continue_download: bool,
    pub timeout: u64,
    pub retry: usize,
    pub concurrent: usize,
    pub quiet: bool,
    pub verbose: bool,
    pub user_agent: String,
    pub headers: Vec<(String, String)>,
}

impl Default for DownloadConfig {
    fn default() -> Self {
        Self {
            url: String::new(),
            output: None,
            continue_download: false,
            timeout: 60,
            retry: 20,
            concurrent: 4,
            quiet: false,
            verbose: false,
            user_agent: "wget/1.21.3".to_string(),
            headers: Vec::new(),
        }
    }
}

impl DownloadConfig {
    pub fn output_to_stdout(&self) -> bool {
        if let Some(output) = &self.output {
            output.to_string_lossy() == "-"
        } else {
            false
        }
    }

    pub fn get_filename(&self) -> String {
        if let Some(output) = &self.output {
            return output
                .file_name()
                .and_then(|n| n.to_str())
                .unwrap_or("download")
                .to_string();
        }

        // Extract filename from URL
        url::Url::parse(&self.url)
            .ok()
            .and_then(|u| {
                u.path_segments()
                    .and_then(|s| s.last())
                    .filter(|s| !s.is_empty())
                    .map(|s| s.to_string())
            })
            .unwrap_or_else(|| "index.html".to_string())
    }
}
