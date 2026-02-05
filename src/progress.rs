use crate::formatter;
use crossterm::terminal;
use std::io::{Write, stderr};
use std::sync::Arc;
use std::sync::Mutex;
use std::sync::atomic::{AtomicU64, Ordering};
use std::time::{Duration, Instant};

pub struct ProgressTracker {
    total: Option<u64>,
    downloaded: Arc<AtomicU64>,
    start_time: Instant,
    last_update: Arc<Mutex<Instant>>,
    quiet: bool,
    filename: String,
}

impl ProgressTracker {
    pub fn new(total: Option<u64>, filepath: &str, quiet: bool) -> Self {
        // Extract filename from path
        let filename = filepath
            .split('/')
            .next_back()
            .unwrap_or(filepath)
            .to_string();

        Self {
            total,
            downloaded: Arc::new(AtomicU64::new(0)),
            start_time: Instant::now(),
            last_update: Arc::new(Mutex::new(Instant::now())),
            quiet,
            filename,
        }
    }

    pub fn inc(&self, amount: u64) {
        self.downloaded.fetch_add(amount, Ordering::Relaxed);

        if !self.quiet {
            let mut last = self.last_update.lock().unwrap();
            // Update every 200ms for smooth display without excessive CPU usage
            if last.elapsed() > Duration::from_millis(200) {
                self.render();
                *last = Instant::now();
            }
        }
    }

    pub fn finish(&self) {
        if !self.quiet {
            self.render();
            eprintln!(); // wget adds a newline after the progress bar
        }
    }

    fn render(&self) {
        let current = self.downloaded.load(Ordering::Relaxed);

        // Calculate percentage (capped at 100%)
        let percent = match self.total {
            Some(total) if total > 0 => ((current as f64 / total as f64) * 100.0).min(100.0) as u8,
            _ => 0,
        };

        // Calculate elapsed time and speed
        let elapsed = self.start_time.elapsed();
        let speed = if elapsed.as_secs_f64() > 0.0 {
            current as f64 / elapsed.as_secs_f64()
        } else {
            0.0
        };

        // Format values using wget conventions
        let size_str = formatter::format_bytes_wget(current);
        let speed_str = formatter::format_speed_wget(speed);
        let speed_num_unit = speed_str.trim_end_matches("/s").trim().to_string();

        // Calculate and format time remaining or elapsed
        let time_str = if percent >= 100 {
            format!("in {}", formatter::format_duration_wget(elapsed))
        } else {
            let eta = match self.total {
                Some(total) if speed > 0.0 => {
                    Duration::from_secs_f64((total - current) as f64 / speed)
                }
                _ => Duration::from_secs(0),
            };
            format!("eta {}", formatter::format_duration_wget(eta))
        };

        // Get terminal columns
        const DEFAULT_COLS: usize = 80;
        const MIN_COLS: usize = 51;
        let cols = terminal::size()
            .map(|(c, _)| c as usize)
            .unwrap_or(DEFAULT_COLS)
            .max(MIN_COLS);
        let usable = cols.saturating_sub(1);

        // Adaptive filename width (like wget: width / 4)
        let max_filename_cols = usable / 4;
        let display_name = truncate_string(&self.filename, max_filename_cols);

        // Calculate bar width (subtract fixed allocations like wget)
        const PERCENT_LEN: usize = 4;
        const DECOR_LEN: usize = 2;
        const FILESIZE_LEN: usize = 8; // >7 + space
        const RATE_LEN: usize = 11; // >8 + /s + space
        const ETA_LEN: usize = 15;
        let filename_alloc = max_filename_cols + 1; // + space
        let fixed =
            filename_alloc + PERCENT_LEN + DECOR_LEN + 1 + FILESIZE_LEN + RATE_LEN + ETA_LEN; // +1 for space after ]
        let mut bar_width = usable.saturating_sub(fixed);
        if bar_width < 5 {
            bar_width = 0;
        }

        // Build the line
        let line = if bar_width > 0 {
            let bar = formatter::build_progress_bar_wget(percent, bar_width);
            format!(
                "{:<max_filename_cols$} {:>3}%[{}] {:>7}  {:>8}/s  {}",
                display_name, percent, bar, size_str, speed_num_unit, time_str
            )
        } else {
            // Fallback without bar if too narrow
            format!(
                "{:<max_filename_cols$} {:>3}% {:>7}  {:>8}/s  {}",
                display_name, percent, size_str, speed_num_unit, time_str
            )
        };

        // Clear the line adaptively, then print to stderr
        eprint!("\r{:<cols$}\r{}", " ", line);
        stderr().flush().unwrap();
    }

    pub fn elapsed(&self) -> Duration {
        self.start_time.elapsed()
    }
}

/// Truncate a string to max_len, adding "..." if truncated
fn truncate_string(s: &str, max_len: usize) -> String {
    if s.len() <= max_len {
        s.to_string()
    } else {
        format!("{}...", &s[..max_len.saturating_sub(3)])
    }
}
