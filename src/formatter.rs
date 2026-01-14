/// Wget-style formatting utilities
use std::time::Duration;

/// Format bytes in wget style (no space between number and unit)
/// Examples: 1234 -> "1234", 1536 -> "1.5K", 2097152 -> "2.0M"
pub fn format_bytes_wget(bytes: u64) -> String {
    if bytes < 1024 {
        format!("{}", bytes)
    } else if bytes < 1024 * 1024 {
        format!("{:.1}K", bytes as f64 / 1024.0)
    } else if bytes < 1024 * 1024 * 1024 {
        format!("{:.1}M", bytes as f64 / 1024.0 / 1024.0)
    } else {
        format!("{:.1}G", bytes as f64 / 1024.0 / 1024.0 / 1024.0)
    }
}

/// Format speed in wget style
/// Examples: "1.2KB/s", "5.3MB/s"
pub fn format_speed_wget(bytes_per_sec: f64) -> String {
    if bytes_per_sec < 1024.0 {
        format!("{:.0}B/s", bytes_per_sec)
    } else if bytes_per_sec < 1024.0 * 1024.0 {
        format!("{:.1}KB/s", bytes_per_sec / 1024.0)
    } else if bytes_per_sec < 1024.0 * 1024.0 * 1024.0 {
        format!("{:.1}MB/s", bytes_per_sec / 1024.0 / 1024.0)
    } else {
        format!("{:.1}GB/s", bytes_per_sec / 1024.0 / 1024.0 / 1024.0)
    }
}

/// Format duration in wget style
/// Examples: "0.2s", "5s", "2m30s", "1h15m"
pub fn format_duration_wget(duration: Duration) -> String {
    let secs_f = duration.as_secs_f64();
    if secs_f < 1.0 {
        format!("{:.1}s", secs_f)
    } else if secs_f < 60.0 {
        format!("{:.0}s", secs_f)
    } else if secs_f < 3600.0 {
        let mins = (secs_f / 60.0).floor();
        let secs = secs_f - mins * 60.0;
        if secs > 0.0 {
            format!("{:.0}m{:.0}s", mins, secs)
        } else {
            format!("{:.0}m", mins)
        }
    } else {
        let hours = (secs_f / 3600.0).floor();
        let mins = ((secs_f - hours * 3600.0) / 60.0).floor();
        if mins > 0.0 {
            format!("{:.0}h{:.0}m", hours, mins)
        } else {
            format!("{:.0}h", hours)
        }
    }
}

/// Build wget-style progress bar
/// Returns a string of length `width`
pub fn build_progress_bar_wget(percent: u8, width: usize) -> String {
    let percent = percent.min(100);

    if percent == 100 {
        "=".repeat(width.saturating_sub(1)) + ">"
    } else {
        let filled = ((width as f64 * percent as f64 / 100.0) as usize).min(width);
        if filled == 0 {
            " ".repeat(width)
        } else {
            "=".repeat(filled.saturating_sub(1)) + ">" + &" ".repeat(width.saturating_sub(filled))
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_format_bytes() {
        assert_eq!(format_bytes_wget(500), "500");
        assert_eq!(format_bytes_wget(1024), "1.0K");
        assert_eq!(format_bytes_wget(1536), "1.5K");
        assert_eq!(format_bytes_wget(1048576), "1.0M");
        assert_eq!(format_bytes_wget(1073741824), "1.0G");
    }

    #[test]
    fn test_progress_bar() {
        assert_eq!(build_progress_bar_wget(0, 10), "          ");
        assert_eq!(build_progress_bar_wget(50, 10), "====>     ");
        assert_eq!(build_progress_bar_wget(100, 10), "=========>");
    }

    #[test]
    fn test_format_duration() {
        assert_eq!(format_duration_wget(Duration::from_millis(0)), "0.0s");
        assert_eq!(format_duration_wget(Duration::from_millis(200)), "0.2s");
        assert_eq!(format_duration_wget(Duration::from_millis(999)), "1.0s");
        assert_eq!(format_duration_wget(Duration::from_secs(30)), "30s");
        assert_eq!(format_duration_wget(Duration::from_secs(60)), "1m");
        assert_eq!(format_duration_wget(Duration::from_secs(90)), "1m30s");
        assert_eq!(format_duration_wget(Duration::from_secs(150)), "2m30s");
        assert_eq!(format_duration_wget(Duration::from_secs(3600)), "1h");
        assert_eq!(format_duration_wget(Duration::from_secs(3660)), "1h1m");
        assert_eq!(format_duration_wget(Duration::from_secs(3720)), "1h2m");
    }
}
