#!/usr/bin/env python3
"""
Bitcoin Puzzle #73 Kangaroo Solver - Main Interface
Implements Pollard's Kangaroo algorithm for solving discrete logarithm problem
"""

import os
import sys
import time
import threading
import signal
from web_server import KangarooWebServer

def main():
    """Main entry point for Bitcoin Puzzle #73 Kangaroo Solver"""
    print("🦘 Bitcoin Puzzle #73 Kangaroo Solver")
    print("=" * 50)
    print("Starting web interface...")
    print()
    print("Target Information:")
    print("  Puzzle: #73")
    print("  Address: 12VVRNPi4SJqUTsp6FmqDqY5sGosDtysn4")
    print("  Range: 0x1000000000000000000 - 0x1ffffffffffffffffff")
    print("  Search Space: 2^72 (≈ 4.7 × 10^21)")
    print()
    
    # Setup signal handlers
    def signal_handler(signum, frame):
        print(f"\n⚠️  Received signal {signum}, shutting down...")
        sys.exit(0)
    
    signal.signal(signal.SIGINT, signal_handler)
    signal.signal(signal.SIGTERM, signal_handler)
    
    try:
        # Create and start the web server
        server = KangarooWebServer()
        print("🌐 Web interface starting on http://0.0.0.0:5000")
        print("   Access the solver dashboard in your browser")
        print("   Press Ctrl+C to stop")
        print("=" * 50)
        
        server.run(host='0.0.0.0', port=5000, debug=False)
        
    except KeyboardInterrupt:
        print("\n✓ Server stopped")
        return 0
    except Exception as e:
        print(f"❌ Error: {e}")
        return 1

if __name__ == "__main__":
    sys.exit(main())
