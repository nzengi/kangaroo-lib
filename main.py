#!/usr/bin/env python3
"""
Bitcoin Puzzle #73 Kangaroo Solver - Main Interface
Implements Pollard's Kangaroo algorithm for solving discrete logarithm problem
"""

import os
import sys
import time
import json
import threading
import subprocess
import signal
from datetime import datetime, timedelta
from config import Config
from monitor import Monitor
import ctypes
from ctypes import c_char_p, c_int, c_uint64, c_bool, POINTER, Structure

class KangarooStats(Structure):
    _fields_ = [
        ("total_jumps", c_uint64),
        ("distinguished_points", c_uint64),
        ("collisions_found", c_uint64),
        ("elapsed_time", c_uint64),
        ("threads_active", c_int),
        ("current_range_start", c_char_p),
        ("current_range_end", c_char_p),
        ("found_key", c_char_p),
        ("is_solved", c_bool)
    ]

class KangarooSolver:
    def __init__(self):
        self.config = Config()
        self.monitor = Monitor()
        self.lib = None
        self.running = False
        self.stats_thread = None
        self.web_server_process = None
        
        # Load the compiled library
        self.load_library()
        
        # Setup signal handlers
        signal.signal(signal.SIGINT, self.signal_handler)
        signal.signal(signal.SIGTERM, self.signal_handler)
    
    def load_library(self):
        """Load the compiled C++ library"""
        try:
            # Try different library names based on platform
            lib_names = ['./libkangaroo.so', './kangaroo.dll', './libkangaroo.dylib']
            
            for lib_name in lib_names:
                if os.path.exists(lib_name):
                    self.lib = ctypes.CDLL(lib_name)
                    break
            
            if not self.lib:
                print("Error: Kangaroo library not found. Please compile first using 'make' or './build.sh'")
                sys.exit(1)
            
            # Setup function signatures
            self.lib.kangaroo_init.argtypes = [c_char_p, c_char_p, c_char_p, c_int, c_int]
            self.lib.kangaroo_init.restype = c_bool
            
            self.lib.kangaroo_start.argtypes = []
            self.lib.kangaroo_start.restype = c_bool
            
            self.lib.kangaroo_stop.argtypes = []
            self.lib.kangaroo_stop.restype = None
            
            self.lib.kangaroo_get_stats.argtypes = [POINTER(KangarooStats)]
            self.lib.kangaroo_get_stats.restype = c_bool
            
            self.lib.kangaroo_save_checkpoint.argtypes = [c_char_p]
            self.lib.kangaroo_save_checkpoint.restype = c_bool
            
            self.lib.kangaroo_load_checkpoint.argtypes = [c_char_p]
            self.lib.kangaroo_load_checkpoint.restype = c_bool
            
            print("✓ Kangaroo library loaded successfully")
            
        except Exception as e:
            print(f"Error loading library: {e}")
            sys.exit(1)
    
    def signal_handler(self, signum, frame):
        """Handle interrupt signals gracefully"""
        print(f"\n⚠️  Received signal {signum}, shutting down gracefully...")
        self.stop()
    
    def initialize(self):
        """Initialize the Kangaroo solver with puzzle parameters"""
        try:
            # Bitcoin Puzzle #73 parameters
            target_pubkey = "03c9f4a96d8e8d2c1c5b8c8f4e8c5a5c3f5c1c7e8f4b2a1d6c3e9f8d2c5b8e7a4f1c".encode('utf-8')
            range_start = "1000000000000000000".encode('utf-8')  # 2^72
            range_end = "1ffffffffffffffffff".encode('utf-8')    # 2^73 - 1
            
            num_threads = self.config.get_threads()
            distinguished_bits = self.config.get_distinguished_bits()
            
            print(f"🔧 Initializing Kangaroo solver...")
            print(f"   Target range: 0x{range_start.decode()} - 0x{range_end.decode()}")
            print(f"   Threads: {num_threads}")
            print(f"   Distinguished bits: {distinguished_bits}")
            
            success = self.lib.kangaroo_init(
                target_pubkey,
                range_start,
                range_end,
                num_threads,
                distinguished_bits
            )
            
            if not success:
                print("❌ Failed to initialize Kangaroo solver")
                return False
            
            print("✓ Kangaroo solver initialized successfully")
            return True
            
        except Exception as e:
            print(f"❌ Initialization error: {e}")
            return False
    
    def load_checkpoint(self, checkpoint_file):
        """Load previous computation state"""
        if os.path.exists(checkpoint_file):
            print(f"📂 Loading checkpoint from {checkpoint_file}...")
            success = self.lib.kangaroo_load_checkpoint(checkpoint_file.encode('utf-8'))
            if success:
                print("✓ Checkpoint loaded successfully")
                return True
            else:
                print("⚠️  Failed to load checkpoint, starting fresh")
        return False
    
    def save_checkpoint(self, checkpoint_file):
        """Save current computation state"""
        try:
            success = self.lib.kangaroo_save_checkpoint(checkpoint_file.encode('utf-8'))
            if success:
                print(f"💾 Checkpoint saved to {checkpoint_file}")
            else:
                print("⚠️  Failed to save checkpoint")
            return success
        except Exception as e:
            print(f"❌ Checkpoint save error: {e}")
            return False
    
    def start_stats_monitoring(self):
        """Start statistics monitoring thread"""
        def monitor_stats():
            last_save = time.time()
            save_interval = self.config.get_checkpoint_interval()
            
            while self.running:
                try:
                    stats = KangarooStats()
                    if self.lib.kangaroo_get_stats(ctypes.byref(stats)):
                        self.display_stats(stats)
                        
                        # Check if solved
                        if stats.is_solved:
                            print(f"\n🎉 PUZZLE SOLVED! Private key found: {stats.found_key.decode()}")
                            self.running = False
                            break
                        
                        # Auto-save checkpoint
                        current_time = time.time()
                        if current_time - last_save >= save_interval:
                            self.save_checkpoint("checkpoint.dat")
                            last_save = current_time
                    
                    time.sleep(1)
                    
                except Exception as e:
                    print(f"⚠️  Stats monitoring error: {e}")
                    time.sleep(5)
        
        self.stats_thread = threading.Thread(target=monitor_stats, daemon=True)
        self.stats_thread.start()
    
    def display_stats(self, stats):
        """Display current statistics"""
        # Clear screen for real-time updates
        os.system('clear' if os.name == 'posix' else 'cls')
        
        print("=" * 80)
        print("🦘 BITCOIN PUZZLE #73 KANGAROO SOLVER")
        print("=" * 80)
        
        # Calculate rates
        elapsed = stats.elapsed_time
        if elapsed > 0:
            jump_rate = stats.total_jumps / elapsed
            dp_rate = stats.distinguished_points / elapsed
        else:
            jump_rate = 0
            dp_rate = 0
        
        print(f"⏱️  Runtime: {self.format_time(elapsed)}")
        print(f"🧵 Active threads: {stats.threads_active}")
        print(f"🦘 Total jumps: {stats.total_jumps:,}")
        print(f"📍 Distinguished points: {stats.distinguished_points:,}")
        print(f"💥 Collisions found: {stats.collisions_found:,}")
        print(f"⚡ Jump rate: {jump_rate:,.0f} jumps/sec")
        print(f"🎯 DP rate: {dp_rate:,.2f} DPs/sec")
        
        if stats.current_range_start and stats.current_range_end:
            print(f"📊 Current range: 0x{stats.current_range_start.decode()}")
            print(f"              to 0x{stats.current_range_end.decode()}")
        
        # Estimated time to solution (very rough estimate)
        if jump_rate > 0:
            # Rough estimate based on birthday paradox
            range_size = 2**73 - 2**72
            expected_ops = (range_size ** 0.5) * 1.25  # Kangaroo efficiency factor
            remaining_ops = max(0, expected_ops - stats.total_jumps)
            eta_seconds = remaining_ops / jump_rate if jump_rate > 0 else 0
            print(f"⏳ ETA (rough): {self.format_time(int(eta_seconds))}")
        
        print("=" * 80)
        print("Press Ctrl+C to stop and save checkpoint")
    
    def format_time(self, seconds):
        """Format seconds into human readable time"""
        if seconds < 60:
            return f"{seconds}s"
        elif seconds < 3600:
            return f"{seconds//60}m {seconds%60}s"
        elif seconds < 86400:
            hours = seconds // 3600
            minutes = (seconds % 3600) // 60
            return f"{hours}h {minutes}m"
        else:
            days = seconds // 86400
            hours = (seconds % 86400) // 3600
            return f"{days}d {hours}h"
    
    def start_web_interface(self):
        """Start web interface for monitoring"""
        try:
            import http.server
            import socketserver
            
            # Start simple HTTP server for the web interface
            def run_server():
                PORT = 5000
                Handler = http.server.SimpleHTTPRequestHandler
                with socketserver.TCPServer(("0.0.0.0", PORT), Handler) as httpd:
                    print(f"🌐 Web interface available at http://localhost:{PORT}")
                    httpd.serve_forever()
            
            web_thread = threading.Thread(target=run_server, daemon=True)
            web_thread.start()
            
        except Exception as e:
            print(f"⚠️  Could not start web interface: {e}")
    
    def run(self):
        """Main execution loop"""
        print("🚀 Starting Bitcoin Puzzle #73 Kangaroo Solver")
        print("=" * 60)
        
        # Initialize solver
        if not self.initialize():
            return False
        
        # Try to load checkpoint
        checkpoint_file = "checkpoint.dat"
        self.load_checkpoint(checkpoint_file)
        
        # Start web interface
        self.start_web_interface()
        
        # Start the computation
        print("🔥 Starting Kangaroo algorithm...")
        self.running = True
        
        if not self.lib.kangaroo_start():
            print("❌ Failed to start Kangaroo computation")
            return False
        
        # Start monitoring
        self.start_stats_monitoring()
        
        try:
            # Keep main thread alive
            while self.running:
                time.sleep(1)
        except KeyboardInterrupt:
            pass
        
        print("\n🛑 Stopping computation...")
        self.stop()
        return True
    
    def stop(self):
        """Stop the solver gracefully"""
        self.running = False
        
        if self.lib:
            self.lib.kangaroo_stop()
            
        # Save final checkpoint
        self.save_checkpoint("final_checkpoint.dat")
        print("✓ Solver stopped and checkpoint saved")

def main():
    """Main entry point"""
    print("Bitcoin Puzzle #73 Kangaroo Solver")
    print("==================================")
    
    # Check if library exists
    if not any(os.path.exists(f) for f in ['./libkangaroo.so', './kangaroo.dll', './libkangaroo.dylib']):
        print("❌ Kangaroo library not found!")
        print("Please compile the C++ code first:")
        print("  Linux/macOS: make")
        print("  Windows: See build instructions")
        return 1
    
    solver = KangarooSolver()
    
    try:
        success = solver.run()
        return 0 if success else 1
    except Exception as e:
        print(f"❌ Fatal error: {e}")
        return 1

if __name__ == "__main__":
    sys.exit(main())
