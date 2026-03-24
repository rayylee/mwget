use crate::formatter;
use crossterm::terminal;
use std::collections::HashMap;
use std::io::{stderr, Write};
use std::sync::atomic::{AtomicU64, Ordering};
use std::sync::Arc;
use std::sync::Mutex;
use std::time::{Duration, Instant};

pub struct ProgressTracker {
    total: Option<u64>,
    downloaded: Arc<AtomicU64>,
    chunk_progress: Arc<Mutex<HashMap<usize, u64>>>,
    total_chunks: usize,
    start_time: Instant,
    last_update: Arc<Mutex<Instant>>,
    quiet: bool,
    filename: String,
}

impl ProgressTracker {
    pub fn new(total: Option<u64>, filepath: &str, quiet: bool) -> Self {
        Self::with_chunks(total, filepath, quiet, 1)
    }

    pub fn with_chunks(
        total: Option<u64>,
        filepath: &str,
        quiet: bool,
        total_chunks: usize,
    ) -> Self {
        // Extract filename from path
        let filename = filepath
            .split('/')
            .next_back()
            .unwrap_or(filepath)
            .to_string();

        Self {
            total,
            downloaded: Arc::new(AtomicU64::new(0)),
            chunk_progress: Arc::new(Mutex::new(HashMap::new())),
            total_chunks,
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

    pub fn inc_chunk(&self, chunk_id: usize, amount: u64) {
        self.downloaded.fetch_add(amount, Ordering::Relaxed);

        let mut chunks = self.chunk_progress.lock().unwrap();
        *chunks.entry(chunk_id).or_insert(0) += amount;
        drop(chunks);

        if !self.quiet {
            let mut last = self.last_update.lock().unwrap();
            if last.elapsed() > Duration::from_millis(200) {
                self.render();
                *last = Instant::now();
            }
        }
    }

    pub fn set_chunk_total(&self, chunk_id: usize, total: u64) {
        let mut chunks = self.chunk_progress.lock().unwrap();
        chunks.insert(chunk_id, total);
    }

    pub fn finish(&self) {
        if !self.quiet {
            self.render_final();
            eprintln!(); // wget adds a newline after the progress bar
        }
    }

    fn render_final(&self) {
        let current = self.downloaded.load(Ordering::Relaxed);

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

        // Format time as elapsed (not ETA) when finishing
        let time_str = format!("in {}", formatter::format_duration_wget(elapsed));

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

        // Build the line with single complete progress bar
        let line = if bar_width > 0 {
            let bar = formatter::build_progress_bar_wget(100, bar_width);
            format!(
                "{:<max_filename_cols$} {:>3}%[{}] {:>7}  {:>8}/s  {}",
                display_name, 100, bar, size_str, speed_num_unit, time_str
            )
        } else {
            // Fallback without bar if too narrow
            format!(
                "{:<max_filename_cols$} {:>3}% {:>7}  {:>8}/s  {}",
                display_name, 100, size_str, speed_num_unit, time_str
            )
        };

        // Clear the line adaptively, then print to stderr
        eprint!("\r{:<cols$}\r{}", " ", line);
        stderr().flush().unwrap();
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

        // Build the line with multi-segment progress bar
        let line = if bar_width > 0 {
            let bar = if self.total_chunks > 1 && self.total.is_some() {
                // Build multi-segment progress bar
                self.build_multi_segment_bar(bar_width)
            } else {
                // Use single segment bar
                formatter::build_progress_bar_wget(percent, bar_width)
            };
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

    fn build_multi_segment_bar(&self, width: usize) -> String {
        let total = self.total.unwrap_or(0);
        if total == 0 {
            return " ".repeat(width);
        }

        let chunks = self.chunk_progress.lock().unwrap();
        let chunk_size = total / self.total_chunks as u64;
        let mut result = String::new();

        for i in 0..self.total_chunks {
            let chunk_start = i as u64 * chunk_size;
            let chunk_end = if i == self.total_chunks - 1 {
                total
            } else {
                chunk_start + chunk_size
            };
            let chunk_total = chunk_end - chunk_start;
            let chunk_downloaded = chunks.get(&i).copied().unwrap_or(0);

            // Calculate how many characters this segment should occupy
            let segment_width = if i == self.total_chunks - 1 {
                width - result.len()
            } else {
                (width as u64 * chunk_total / total) as usize
            };

            if segment_width == 0 {
                continue;
            }

            let segment_percent =
                ((chunk_downloaded as f64 / chunk_total as f64) * 100.0).min(100.0) as u8;

            // Build segment bar using "=>" pattern instead of "==>"
            let segment_bar = if segment_percent == 100 {
                "=".repeat(segment_width.saturating_sub(1)) + ">"
            } else {
                let filled = ((segment_width as f64 * segment_percent as f64 / 100.0) as usize)
                    .min(segment_width);
                if filled == 0 {
                    " ".repeat(segment_width)
                } else if filled >= segment_width {
                    "=".repeat(segment_width.saturating_sub(1)) + ">"
                } else {
                    "=".repeat(filled.saturating_sub(1))
                        + ">"
                        + &" ".repeat(segment_width.saturating_sub(filled))
                }
            };

            result.push_str(&segment_bar);
        }

        // Fill remaining width if any
        if result.len() < width {
            result.push_str(&" ".repeat(width - result.len()));
        }

        result
    }

    pub fn elapsed(&self) -> Duration {
        self.start_time.elapsed()
    }
}

pub struct ChunkProgress {
    tracker: Arc<ProgressTracker>,
    chunk_id: usize,
    downloaded: u64,
}

impl ChunkProgress {
    pub fn new(
        tracker: Arc<ProgressTracker>,
        chunk_id: usize,
        chunk_start: u64,
        chunk_end: u64,
    ) -> Self {
        let total = chunk_end - chunk_start + 1;
        tracker.set_chunk_total(chunk_id, total);

        Self {
            tracker,
            chunk_id,
            downloaded: 0,
        }
    }

    pub fn inc(&mut self, amount: u64) {
        self.downloaded += amount;
        self.tracker.inc_chunk(self.chunk_id, amount);
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
