"""
Monitoring and statistics for Bitcoin Puzzle #73 Kangaroo Solver
"""

import time
import json
import os
from datetime import datetime, timedelta
import threading

class Monitor:
    def __init__(self):
        self.stats = {
            "start_time": None,
            "total_jumps": 0,
            "distinguished_points": 0,
            "collisions_found": 0,
            "threads_active": 0,
            "current_range_start": "",
            "current_range_end": "",
            "found_key": "",
            "is_solved": False,
            "last_checkpoint": None,
            "uptime": 0,
            "jump_rate": 0,
            "dp_rate": 0,
            "memory_usage": 0,
            "cpu_usage": 0
        }
        
        self.history = []
        self.lock = threading.Lock()
        self.log_file = "kangaroo_solver.log"
        
    def start_monitoring(self):
        """Start the monitoring system"""
        self.stats["start_time"] = time.time()
        self.log_event("INFO", "Monitoring started")
    
    def update_stats(self, new_stats):
        """Update statistics"""
        with self.lock:
            self.stats.update(new_stats)
            self.stats["uptime"] = time.time() - self.stats["start_time"] if self.stats["start_time"] else 0
            
            # Calculate rates
            if self.stats["uptime"] > 0:
                self.stats["jump_rate"] = self.stats["total_jumps"] / self.stats["uptime"]
                self.stats["dp_rate"] = self.stats["distinguished_points"] / self.stats["uptime"]
            
            # Add to history (keep last 1000 entries)
            self.history.append({
                "timestamp": time.time(),
                "total_jumps": self.stats["total_jumps"],
                "distinguished_points": self.stats["distinguished_points"],
                "jump_rate": self.stats["jump_rate"]
            })
            
            if len(self.history) > 1000:
                self.history.pop(0)
    
    def get_stats(self):
        """Get current statistics"""
        with self.lock:
            return self.stats.copy()
    
    def get_history(self):
        """Get statistics history"""
        with self.lock:
            return self.history.copy()
    
    def log_event(self, level, message):
        """Log an event with timestamp"""
        timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        log_entry = f"[{timestamp}] {level}: {message}"
        
        print(log_entry)
        
        try:
            with open(self.log_file, 'a') as f:
                f.write(log_entry + "\n")
        except Exception as e:
            print(f"Warning: Could not write to log file: {e}")
    
    def format_number(self, number):
        """Format large numbers with appropriate suffixes"""
        if number >= 1e12:
            return f"{number/1e12:.2f}T"
        elif number >= 1e9:
            return f"{number/1e9:.2f}G"
        elif number >= 1e6:
            return f"{number/1e6:.2f}M"
        elif number >= 1e3:
            return f"{number/1e3:.2f}K"
        else:
            return str(number)
    
    def format_time(self, seconds):
        """Format seconds into human readable time"""
        if seconds < 60:
            return f"{int(seconds)}s"
        elif seconds < 3600:
            return f"{int(seconds//60)}m {int(seconds%60)}s"
        elif seconds < 86400:
            hours = int(seconds // 3600)
            minutes = int((seconds % 3600) // 60)
            return f"{hours}h {minutes}m"
        else:
            days = int(seconds // 86400)
            hours = int((seconds % 86400) // 3600)
            return f"{days}d {hours}h"
    
    def get_performance_metrics(self):
        """Get performance metrics"""
        stats = self.get_stats()
        
        metrics = {
            "runtime": self.format_time(stats["uptime"]),
            "total_jumps": self.format_number(stats["total_jumps"]),
            "distinguished_points": self.format_number(stats["distinguished_points"]),
            "jump_rate": f"{self.format_number(stats['jump_rate'])}/s",
            "dp_rate": f"{stats['dp_rate']:.2f}/s",
            "threads": stats["threads_active"],
            "collisions": stats["collisions_found"]
        }
        
        return metrics
    
    def estimate_completion(self):
        """Estimate completion time (very rough)"""
        stats = self.get_stats()
        
        if stats["jump_rate"] <= 0:
            return "Unknown"
        
        # Bitcoin Puzzle #73 search space
        search_space = 2**73 - 2**72  # Range size
        
        # Kangaroo algorithm expected operations (birthday paradox)
        # This is a very rough estimate
        expected_operations = (search_space ** 0.5) * 1.25  # Efficiency factor
        
        remaining_operations = max(0, expected_operations - stats["total_jumps"])
        
        if remaining_operations <= 0:
            return "Soon"
        
        eta_seconds = remaining_operations / stats["jump_rate"]
        return self.format_time(eta_seconds)
    
    def save_stats_to_file(self, filename="stats.json"):
        """Save current statistics to file"""
        try:
            stats = self.get_stats()
            with open(filename, 'w') as f:
                json.dump(stats, f, indent=4)
            return True
        except Exception as e:
            self.log_event("ERROR", f"Could not save stats to file: {e}")
            return False
    
    def load_stats_from_file(self, filename="stats.json"):
        """Load statistics from file"""
        try:
            if os.path.exists(filename):
                with open(filename, 'r') as f:
                    loaded_stats = json.load(f)
                    self.update_stats(loaded_stats)
                return True
        except Exception as e:
            self.log_event("ERROR", f"Could not load stats from file: {e}")
        return False
    
    def generate_report(self):
        """Generate a comprehensive report"""
        stats = self.get_stats()
        metrics = self.get_performance_metrics()
        
        report = f"""
BITCOIN PUZZLE #73 KANGAROO SOLVER REPORT
========================================
Generated: {datetime.now().strftime("%Y-%m-%d %H:%M:%S")}

PERFORMANCE METRICS:
- Runtime: {metrics['runtime']}
- Total Jumps: {metrics['total_jumps']}
- Distinguished Points: {metrics['distinguished_points']}
- Jump Rate: {metrics['jump_rate']}
- DP Rate: {metrics['dp_rate']}
- Active Threads: {metrics['threads']}
- Collisions Found: {metrics['collisions']}

SEARCH PROGRESS:
- Range Start: 0x{stats['current_range_start']}
- Range End: 0x{stats['current_range_end']}
- Estimated Completion: {self.estimate_completion()}
- Status: {'SOLVED' if stats['is_solved'] else 'RUNNING'}

SYSTEM INFO:
- Last Checkpoint: {stats['last_checkpoint'] or 'None'}
- Log File: {self.log_file}
- Config File: config.json
"""
        
        if stats['is_solved']:
            report += f"\nSOLUTION FOUND:\n- Private Key: {stats['found_key']}\n"
        
        return report
    
    def print_report(self):
        """Print the comprehensive report"""
        print(self.generate_report())
    
    def save_report(self, filename="report.txt"):
        """Save report to file"""
        try:
            with open(filename, 'w') as f:
                f.write(self.generate_report())
            self.log_event("INFO", f"Report saved to {filename}")
            return True
        except Exception as e:
            self.log_event("ERROR", f"Could not save report: {e}")
            return False
