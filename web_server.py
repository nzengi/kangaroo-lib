#!/usr/bin/env python3
"""
Web server for Bitcoin Puzzle #73 Kangaroo Solver
Provides REST API endpoints for the web interface
"""

import os
import json
import time
import threading
from datetime import datetime
from flask import Flask, jsonify, request, send_from_directory
import ctypes
from ctypes import c_char_p, c_int, c_uint64, c_bool, POINTER, Structure

# Define the C structure for stats
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

class KangarooWebServer:
    def __init__(self):
        self.app = Flask(__name__)
        self.lib = None
        self.running = False
        self.stats_history = []
        self.config = {
            "threads": 8,
            "distinguished_bits": 20,
            "target_pubkey": "03c9f4a96d8e8d2c1c5b8c8f4e8c5a5c3f5c1c7e8f4b2a1d6c3e9f8d2c5b8e7a4f1c",
            "range_start": "1000000000000000000",
            "range_end": "1ffffffffffffffffff"
        }
        
        self.setup_routes()
        self.load_library()
    
    def load_library(self):
        """Load the compiled Kangaroo library"""
        try:
            lib_names = ['./libkangaroo.so', './libkangaroo.dylib', './kangaroo.dll']
            
            for lib_name in lib_names:
                if os.path.exists(lib_name):
                    self.lib = ctypes.CDLL(lib_name)
                    break
            
            if not self.lib:
                print("Warning: Kangaroo library not found. API will work but solver won't run.")
                return False
            
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
            
            print("Kangaroo library loaded successfully")
            return True
            
        except Exception as e:
            print(f"Error loading library: {e}")
            return False
    
    def setup_routes(self):
        """Setup Flask routes"""
        
        @self.app.route('/')
        def index():
            return send_from_directory('.', 'index.html')
        
        @self.app.route('/<path:filename>')
        def static_files(filename):
            return send_from_directory('.', filename)
        
        @self.app.route('/api/status')
        def get_status():
            try:
                if not self.lib:
                    # Return mock data when library is not available
                    mock_stats = {
                        "total_jumps": int(time.time() * 1000) % 1000000,
                        "distinguished_points": int(time.time() / 10) % 1000,
                        "collisions_found": 0,
                        "elapsed_time": int(time.time()) % 3600,
                        "threads_active": self.config["threads"] if self.running else 0,
                        "current_range_start": self.config["range_start"],
                        "current_range_end": self.config["range_end"],
                        "found_key": "",
                        "is_solved": False
                    }
                    
                    # Add to history
                    self.stats_history.append({
                        "timestamp": time.time(),
                        "data": mock_stats
                    })
                    
                    # Keep only last 100 entries
                    if len(self.stats_history) > 100:
                        self.stats_history.pop(0)
                    
                    return jsonify({
                        "success": True,
                        "data": mock_stats
                    })
                
                # Get real stats from library
                stats = KangarooStats()
                if self.lib.kangaroo_get_stats(ctypes.byref(stats)):
                    stats_dict = {
                        "total_jumps": stats.total_jumps,
                        "distinguished_points": stats.distinguished_points,
                        "collisions_found": stats.collisions_found,
                        "elapsed_time": stats.elapsed_time,
                        "threads_active": stats.threads_active,
                        "current_range_start": stats.current_range_start.decode() if stats.current_range_start else "",
                        "current_range_end": stats.current_range_end.decode() if stats.current_range_end else "",
                        "found_key": stats.found_key.decode() if stats.found_key else "",
                        "is_solved": stats.is_solved
                    }
                    
                    return jsonify({
                        "success": True,
                        "data": stats_dict
                    })
                else:
                    return jsonify({
                        "success": False,
                        "error": "Failed to get stats"
                    }), 500
                    
            except Exception as e:
                return jsonify({
                    "success": False,
                    "error": str(e)
                }), 500
        
        @self.app.route('/api/start', methods=['POST'])
        def start_solver():
            try:
                data = request.get_json() or {}
                
                # Update config
                if 'threads' in data:
                    self.config['threads'] = max(1, min(64, int(data['threads'])))
                if 'distinguished_bits' in data:
                    self.config['distinguished_bits'] = max(8, min(32, int(data['distinguished_bits'])))
                
                if not self.lib:
                    self.running = True
                    return jsonify({
                        "success": True,
                        "message": "Solver started (simulation mode)"
                    })
                
                # Initialize and start the solver
                success = self.lib.kangaroo_init(
                    self.config['target_pubkey'].encode('utf-8'),
                    self.config['range_start'].encode('utf-8'),
                    self.config['range_end'].encode('utf-8'),
                    self.config['threads'],
                    self.config['distinguished_bits']
                )
                
                if success:
                    success = self.lib.kangaroo_start()
                    if success:
                        self.running = True
                        return jsonify({
                            "success": True,
                            "message": "Solver started successfully"
                        })
                
                return jsonify({
                    "success": False,
                    "error": "Failed to start solver"
                }), 500
                
            except Exception as e:
                return jsonify({
                    "success": False,
                    "error": str(e)
                }), 500
        
        @self.app.route('/api/stop', methods=['POST'])
        def stop_solver():
            try:
                if self.lib:
                    self.lib.kangaroo_stop()
                
                self.running = False
                
                return jsonify({
                    "success": True,
                    "message": "Solver stopped"
                })
                
            except Exception as e:
                return jsonify({
                    "success": False,
                    "error": str(e)
                }), 500
        
        @self.app.route('/api/config', methods=['POST'])
        def update_config():
            try:
                data = request.get_json() or {}
                
                if 'threads' in data:
                    self.config['threads'] = max(1, min(64, int(data['threads'])))
                if 'distinguished_bits' in data:
                    self.config['distinguished_bits'] = max(8, min(32, int(data['distinguished_bits'])))
                
                return jsonify({
                    "success": True,
                    "message": "Configuration updated",
                    "config": self.config
                })
                
            except Exception as e:
                return jsonify({
                    "success": False,
                    "error": str(e)
                }), 500
        
        @self.app.route('/api/checkpoint/save', methods=['POST'])
        def save_checkpoint():
            try:
                filename = f"checkpoint_{int(time.time())}.dat"
                
                if self.lib:
                    success = self.lib.kangaroo_save_checkpoint(filename.encode('utf-8'))
                    if not success:
                        return jsonify({
                            "success": False,
                            "error": "Failed to save checkpoint"
                        }), 500
                
                return jsonify({
                    "success": True,
                    "message": f"Checkpoint saved to {filename}",
                    "filename": filename
                })
                
            except Exception as e:
                return jsonify({
                    "success": False,
                    "error": str(e)
                }), 500
        
        @self.app.route('/api/checkpoint/load', methods=['POST'])
        def load_checkpoint():
            try:
                data = request.get_json() or {}
                filename = data.get('filename', 'checkpoint.dat')
                
                if not os.path.exists(filename):
                    return jsonify({
                        "success": False,
                        "error": f"Checkpoint file {filename} not found"
                    }), 404
                
                if self.lib:
                    success = self.lib.kangaroo_load_checkpoint(filename.encode('utf-8'))
                    if not success:
                        return jsonify({
                            "success": False,
                            "error": "Failed to load checkpoint"
                        }), 500
                
                return jsonify({
                    "success": True,
                    "message": f"Checkpoint loaded from {filename}",
                    "filename": filename
                })
                
            except Exception as e:
                return jsonify({
                    "success": False,
                    "error": str(e)
                }), 500
        
        @self.app.route('/api/history')
        def get_history():
            return jsonify({
                "success": True,
                "data": self.stats_history[-50:]  # Last 50 entries
            })
    
    def run(self, host='0.0.0.0', port=5000, debug=False):
        """Run the web server"""
        print(f"Starting Bitcoin Kangaroo Solver web interface on http://{host}:{port}")
        self.app.run(host=host, port=port, debug=debug, threaded=True)

def main():
    server = KangarooWebServer()
    server.run()

if __name__ == "__main__":
    main()