"""
Configuration management for Bitcoin Puzzle #73 Kangaroo Solver
"""

import os
import json
import multiprocessing

class Config:
    def __init__(self, config_file="config.json"):
        self.config_file = config_file
        self.default_config = {
            "threads": min(multiprocessing.cpu_count(), 16),
            "distinguished_bits": 20,
            "checkpoint_interval": 300,  # 5 minutes
            "jump_distance_bits": 16,
            "max_distinguished_points": 1000000,
            "target_address": "12VVRNPi4SJqUTsp6FmqDqY5sGosDtysn4",
            "target_pubkey": "03c9f4a96d8e8d2c1c5b8c8f4e8c5a5c3f5c1c7e8f4b2a1d6c3e9f8d2c5b8e7a4f1c",
            "puzzle_number": 73,
            "range_start": "1000000000000000000",
            "range_end": "1ffffffffffffffffff",
            "web_interface_port": 5000,
            "log_level": "INFO",
            "auto_save": True,
            "backup_checkpoints": True
        }
        
        self.config = self.load_config()
    
    def load_config(self):
        """Load configuration from file or create default"""
        if os.path.exists(self.config_file):
            try:
                with open(self.config_file, 'r') as f:
                    config = json.load(f)
                    # Merge with defaults to ensure all keys exist
                    merged_config = self.default_config.copy()
                    merged_config.update(config)
                    return merged_config
            except Exception as e:
                print(f"Warning: Could not load config file: {e}")
                print("Using default configuration")
        
        # Create default config file
        self.save_config(self.default_config)
        return self.default_config.copy()
    
    def save_config(self, config=None):
        """Save configuration to file"""
        if config is None:
            config = self.config
        
        try:
            with open(self.config_file, 'w') as f:
                json.dump(config, f, indent=4)
        except Exception as e:
            print(f"Warning: Could not save config file: {e}")
    
    def get_threads(self):
        """Get number of threads to use"""
        threads = self.config.get("threads", multiprocessing.cpu_count())
        return min(max(1, threads), 64)  # Clamp between 1 and 64
    
    def get_distinguished_bits(self):
        """Get number of distinguished bits"""
        return self.config.get("distinguished_bits", 20)
    
    def get_checkpoint_interval(self):
        """Get checkpoint save interval in seconds"""
        return self.config.get("checkpoint_interval", 300)
    
    def get_jump_distance_bits(self):
        """Get jump distance bits"""
        return self.config.get("jump_distance_bits", 16)
    
    def get_max_distinguished_points(self):
        """Get maximum distinguished points to store"""
        return self.config.get("max_distinguished_points", 1000000)
    
    def get_target_address(self):
        """Get target Bitcoin address"""
        return self.config.get("target_address", "12VVRNPi4SJqUTsp6FmqDqY5sGosDtysn4")
    
    def get_target_pubkey(self):
        """Get target public key"""
        return self.config.get("target_pubkey", "03c9f4a96d8e8d2c1c5b8c8f4e8c5a5c3f5c1c7e8f4b2a1d6c3e9f8d2c5b8e7a4f1c")
    
    def get_puzzle_number(self):
        """Get puzzle number"""
        return self.config.get("puzzle_number", 73)
    
    def get_range_start(self):
        """Get search range start"""
        return self.config.get("range_start", "1000000000000000000")
    
    def get_range_end(self):
        """Get search range end"""
        return self.config.get("range_end", "1ffffffffffffffffff")
    
    def get_web_port(self):
        """Get web interface port"""
        return self.config.get("web_interface_port", 5000)
    
    def get_log_level(self):
        """Get logging level"""
        return self.config.get("log_level", "INFO")
    
    def is_auto_save_enabled(self):
        """Check if auto-save is enabled"""
        return self.config.get("auto_save", True)
    
    def is_backup_enabled(self):
        """Check if checkpoint backup is enabled"""
        return self.config.get("backup_checkpoints", True)
    
    def update_config(self, key, value):
        """Update a configuration value"""
        self.config[key] = value
        self.save_config()
    
    def reset_to_defaults(self):
        """Reset configuration to defaults"""
        self.config = self.default_config.copy()
        self.save_config()
    
    def print_config(self):
        """Print current configuration"""
        print("Current Configuration:")
        print("=" * 40)
        for key, value in self.config.items():
            print(f"{key:25}: {value}")
        print("=" * 40)
