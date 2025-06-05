#ifndef KANGAROO_SOLVER_H
#define KANGAROO_SOLVER_H

#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>
#include <unordered_map>
#include <memory>
#include "ecc_utils.h"

struct KangarooStats {
    uint64_t total_jumps;
    uint64_t distinguished_points;
    uint64_t collisions_found;
    uint64_t elapsed_time;
    int threads_active;
    char current_range_start[65];
    char current_range_end[65];
    char found_key[65];
    bool is_solved;
};

struct DistinguishedPoint {
    ECPoint point;
    std::string distance;
    bool is_tame;
    uint64_t timestamp;
};

class KangarooSolver {
private:
    // Configuration
    ECPoint target_point;
    BigInt range_start;
    BigInt range_end;
    int num_threads;
    int distinguished_bits;
    uint64_t distinguished_mask;
    
    // State
    std::atomic<bool> running;
    std::atomic<bool> solved;
    std::atomic<uint64_t> total_jumps;
    std::atomic<uint64_t> total_distinguished_points;
    std::atomic<uint64_t> total_collisions;
    
    // Threading
    std::vector<std::thread> worker_threads;
    std::mutex dp_mutex;
    std::mutex stats_mutex;
    
    // Distinguished points storage
    std::unordered_map<std::string, DistinguishedPoint> distinguished_points;
    
    // Jump distances (precomputed)
    std::vector<BigInt> jump_distances;
    std::vector<ECPoint> jump_points;
    
    // Solution
    BigInt solution_key;
    
    // Timing
    std::chrono::time_point<std::chrono::steady_clock> start_time;
    
public:
    KangarooSolver();
    ~KangarooSolver();
    
    // Initialization
    bool initialize(const std::string& pubkey_hex, 
                   const std::string& range_start_hex,
                   const std::string& range_end_hex,
                   int threads, 
                   int dist_bits);
    
    // Control
    bool start();
    void stop();
    bool is_running() const;
    bool is_solved() const;
    
    // Statistics
    KangarooStats get_stats() const;
    
    // Checkpointing
    bool save_checkpoint(const std::string& filename) const;
    bool load_checkpoint(const std::string& filename);
    
private:
    // Core algorithm
    void worker_thread(int thread_id);
    void tame_kangaroo(int thread_id);
    void wild_kangaroo(int thread_id);
    
    // Distinguished point handling
    bool is_distinguished(const ECPoint& point) const;
    bool add_distinguished_point(const ECPoint& point, const BigInt& distance, bool is_tame);
    bool check_collision(const DistinguishedPoint& dp);
    
    // Jump functions
    void precompute_jumps();
    int get_jump_index(const ECPoint& point) const;
    
    // Utilities
    std::string point_to_string(const ECPoint& point) const;
    ECPoint string_to_point(const std::string& str) const;
    uint64_t get_elapsed_time() const;
    
    // Random number generation
    BigInt generate_random_in_range(const BigInt& start, const BigInt& end) const;
};

// C interface for Python binding
extern "C" {
    bool kangaroo_init(const char* pubkey, const char* range_start, const char* range_end, int threads, int dist_bits);
    bool kangaroo_start();
    void kangaroo_stop();
    bool kangaroo_get_stats(KangarooStats* stats);
    bool kangaroo_save_checkpoint(const char* filename);
    bool kangaroo_load_checkpoint(const char* filename);
}

#endif // KANGAROO_SOLVER_H
