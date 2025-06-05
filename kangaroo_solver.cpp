#include "kangaroo_solver.h"
#include "checkpoint.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <random>
#include <chrono>
#include <algorithm>
#include <cstring>

// Global instance for C interface
static std::unique_ptr<KangarooSolver> g_solver;

KangarooSolver::KangarooSolver() :
    running(false),
    solved(false),
    total_jumps(0),
    total_distinguished_points(0),
    total_collisions(0),
    num_threads(1),
    distinguished_bits(20),
    distinguished_mask(0) {
}

KangarooSolver::~KangarooSolver() {
    stop();
}

bool KangarooSolver::initialize(const std::string& pubkey_hex, 
                               const std::string& range_start_hex,
                               const std::string& range_end_hex,
                               int threads, 
                               int dist_bits) {
    try {
        // Parse target public key
        if (!hex_to_point(pubkey_hex, target_point)) {
            std::cerr << "Error: Invalid public key format" << std::endl;
            return false;
        }
        
        // Parse range
        range_start = hex_to_bigint(range_start_hex);
        range_end = hex_to_bigint(range_end_hex);
        
        if (range_start >= range_end) {
            std::cerr << "Error: Invalid range (start >= end)" << std::endl;
            return false;
        }
        
        // Set parameters
        num_threads = std::max(1, std::min(64, threads));
        distinguished_bits = std::max(8, std::min(32, dist_bits));
        distinguished_mask = (1ULL << distinguished_bits) - 1;
        
        // Precompute jump distances
        precompute_jumps();
        
        std::cout << "Kangaroo solver initialized:" << std::endl;
        std::cout << "  Range: 0x" << bigint_to_hex(range_start) << " - 0x" << bigint_to_hex(range_end) << std::endl;
        std::cout << "  Threads: " << num_threads << std::endl;
        std::cout << "  Distinguished bits: " << distinguished_bits << std::endl;
        std::cout << "  Jump table size: " << jump_distances.size() << std::endl;
        
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Initialization error: " << e.what() << std::endl;
        return false;
    }
}

bool KangarooSolver::start() {
    if (running.load()) {
        return false;
    }
    
    running.store(true);
    solved.store(false);
    start_time = std::chrono::steady_clock::now();
    
    // Clear previous state
    distinguished_points.clear();
    total_jumps.store(0);
    total_distinguished_points.store(0);
    total_collisions.store(0);
    
    // Start worker threads
    worker_threads.clear();
    worker_threads.reserve(num_threads);
    
    for (int i = 0; i < num_threads; ++i) {
        worker_threads.emplace_back(&KangarooSolver::worker_thread, this, i);
    }
    
    std::cout << "Kangaroo solver started with " << num_threads << " threads" << std::endl;
    return true;
}

void KangarooSolver::stop() {
    if (!running.load()) {
        return;
    }
    
    running.store(false);
    
    // Wait for all threads to finish
    for (auto& thread : worker_threads) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    
    worker_threads.clear();
    std::cout << "Kangaroo solver stopped" << std::endl;
}

bool KangarooSolver::is_running() const {
    return running.load();
}

bool KangarooSolver::is_solved() const {
    return solved.load();
}

void KangarooSolver::worker_thread(int thread_id) {
    // Half threads do tame kangaroos, half do wild kangaroos
    if (thread_id % 2 == 0) {
        tame_kangaroo(thread_id);
    } else {
        wild_kangaroo(thread_id);
    }
}

void KangarooSolver::tame_kangaroo(int thread_id) {
    // Initialize random number generator
    std::random_device rd;
    std::mt19937_64 gen(rd() ^ thread_id);
    
    // Generate random starting position in range
    BigInt k = generate_random_in_range(range_start, range_end);
    ECPoint current = point_multiply(k, get_generator());
    BigInt distance = k;
    
    uint64_t local_jumps = 0;
    const uint64_t report_interval = 10000;
    
    while (running.load() && !solved.load()) {
        // Check if this is a distinguished point
        if (is_distinguished(current)) {
            if (add_distinguished_point(current, distance, true)) {
                // Collision found!
                solved.store(true);
                solution_key = distance;
                break;
            }
        }
        
        // Make a jump
        int jump_idx = get_jump_index(current);
        current = point_add(current, jump_points[jump_idx]);
        distance = bigint_add(distance, jump_distances[jump_idx]);
        
        local_jumps++;
        
        // Periodically update global counter
        if (local_jumps % report_interval == 0) {
            total_jumps.fetch_add(report_interval);
        }
        
        // Check if we've gone outside the range (restart)
        if (bigint_compare(distance, range_end) > 0) {
            k = generate_random_in_range(range_start, range_end);
            current = point_multiply(k, get_generator());
            distance = k;
        }
    }
    
    // Update final jump count
    total_jumps.fetch_add(local_jumps % report_interval);
}

void KangarooSolver::wild_kangaroo(int thread_id) {
    // Initialize random number generator
    std::random_device rd;
    std::mt19937_64 gen(rd() ^ thread_id ^ 0x12345678);
    
    // Start from target point
    ECPoint current = target_point;
    BigInt distance = bigint_zero();
    
    uint64_t local_jumps = 0;
    const uint64_t report_interval = 10000;
    
    while (running.load() && !solved.load()) {
        // Check if this is a distinguished point
        if (is_distinguished(current)) {
            if (add_distinguished_point(current, distance, false)) {
                // Collision found!
                solved.store(true);
                solution_key = distance;
                break;
            }
        }
        
        // Make a jump
        int jump_idx = get_jump_index(current);
        current = point_add(current, jump_points[jump_idx]);
        distance = bigint_add(distance, jump_distances[jump_idx]);
        
        local_jumps++;
        
        // Periodically update global counter
        if (local_jumps % report_interval == 0) {
            total_jumps.fetch_add(report_interval);
        }
        
        // Wild kangaroos can go anywhere, but reset if distance gets too large
        if (bigint_bit_length(distance) > 80) { // Restart if getting too far
            current = target_point;
            distance = bigint_zero();
        }
    }
    
    // Update final jump count
    total_jumps.fetch_add(local_jumps % report_interval);
}

bool KangarooSolver::is_distinguished(const ECPoint& point) const {
    // A point is distinguished if its x-coordinate has enough trailing zeros
    std::string x_hex = bigint_to_hex(point.x);
    
    // Convert last few hex digits to integer and check bits
    if (x_hex.length() >= 8) {
        std::string last_chars = x_hex.substr(x_hex.length() - 8);
        uint64_t last_bits = std::stoull(last_chars, nullptr, 16);
        return (last_bits & distinguished_mask) == 0;
    }
    
    return false;
}

bool KangarooSolver::add_distinguished_point(const ECPoint& point, const BigInt& distance, bool is_tame) {
    std::lock_guard<std::mutex> lock(dp_mutex);
    
    std::string point_str = point_to_string(point);
    
    auto it = distinguished_points.find(point_str);
    if (it != distinguished_points.end()) {
        // Collision found!
        const DistinguishedPoint& existing = it->second;
        
        if (existing.is_tame != is_tame) {
            // Different types - this is what we want!
            total_collisions.fetch_add(1);
            
            // Calculate the private key
            BigInt key_diff;
            if (is_tame) {
                // Current is tame, existing is wild
                key_diff = bigint_subtract(distance, existing.distance);
            } else {
                // Current is wild, existing is tame
                key_diff = bigint_subtract(existing.distance, distance);
            }
            
            // Verify the solution
            ECPoint test_point = point_multiply(key_diff, get_generator());
            if (point_equals(test_point, target_point)) {
                solution_key = key_diff;
                return true;
            }
        }
    } else {
        // New distinguished point
        DistinguishedPoint dp;
        dp.point = point;
        dp.distance = bigint_to_hex(distance);
        dp.is_tame = is_tame;
        dp.timestamp = get_elapsed_time();
        
        distinguished_points[point_str] = dp;
        total_distinguished_points.fetch_add(1);
    }
    
    return false;
}

void KangarooSolver::precompute_jumps() {
    const int num_jumps = 256; // 2^8 different jump distances
    jump_distances.clear();
    jump_points.clear();
    jump_distances.reserve(num_jumps);
    jump_points.reserve(num_jumps);
    
    // Generate jump distances based on the range size
    BigInt range_size = bigint_subtract(range_end, range_start);
    int range_bits = bigint_bit_length(range_size);
    
    // Jump distances should be around sqrt(range_size) / num_jumps
    int base_jump_bits = std::max(1, range_bits / 2 - 8);
    
    for (int i = 0; i < num_jumps; ++i) {
        // Create jump distance: base + variation
        BigInt jump_size = bigint_shift_left(bigint_one(), base_jump_bits);
        BigInt variation = bigint_from_int(i + 1);
        BigInt jump_distance = bigint_add(jump_size, variation);
        
        // Precompute the corresponding point
        ECPoint jump_point = point_multiply(jump_distance, get_generator());
        
        jump_distances.push_back(jump_distance);
        jump_points.push_back(jump_point);
    }
    
    std::cout << "Precomputed " << num_jumps << " jump distances" << std::endl;
}

int KangarooSolver::get_jump_index(const ECPoint& point) const {
    // Use the last byte of the x-coordinate to select jump
    std::string x_hex = bigint_to_hex(point.x);
    if (x_hex.length() >= 2) {
        std::string last_byte = x_hex.substr(x_hex.length() - 2);
        return std::stoi(last_byte, nullptr, 16) % jump_distances.size();
    }
    return 0;
}

std::string KangarooSolver::point_to_string(const ECPoint& point) const {
    return bigint_to_hex(point.x) + ":" + bigint_to_hex(point.y);
}

ECPoint KangarooSolver::string_to_point(const std::string& str) const {
    size_t colon_pos = str.find(':');
    if (colon_pos == std::string::npos) {
        return ECPoint(); // Invalid
    }
    
    ECPoint point;
    point.x = hex_to_bigint(str.substr(0, colon_pos));
    point.y = hex_to_bigint(str.substr(colon_pos + 1));
    return point;
}

uint64_t KangarooSolver::get_elapsed_time() const {
    auto now = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - start_time);
    return duration.count();
}

BigInt KangarooSolver::generate_random_in_range(const BigInt& start, const BigInt& end) const {
    // Simple random generation - could be improved
    static thread_local std::random_device rd;
    static thread_local std::mt19937_64 gen(rd());
    
    BigInt range_size = bigint_subtract(end, start);
    
    // For simplicity, generate a random offset and add to start
    // This is not perfectly uniform but good enough for our purposes
    std::uniform_int_distribution<uint64_t> dis(0, UINT64_MAX);
    uint64_t random_val = dis(gen);
    
    BigInt offset = bigint_mod(bigint_from_uint64(random_val), range_size);
    return bigint_add(start, offset);
}

KangarooStats KangarooSolver::get_stats() const {
    KangarooStats stats = {};
    
    stats.total_jumps = total_jumps.load();
    stats.distinguished_points = total_distinguished_points.load();
    stats.collisions_found = total_collisions.load();
    stats.elapsed_time = get_elapsed_time();
    stats.threads_active = running.load() ? num_threads : 0;
    
    // Copy range information
    std::string start_hex = bigint_to_hex(range_start);
    std::string end_hex = bigint_to_hex(range_end);
    
    strncpy(stats.current_range_start, start_hex.c_str(), sizeof(stats.current_range_start) - 1);
    strncpy(stats.current_range_end, end_hex.c_str(), sizeof(stats.current_range_end) - 1);
    
    stats.is_solved = solved.load();
    if (stats.is_solved) {
        std::string key_hex = bigint_to_hex(solution_key);
        strncpy(stats.found_key, key_hex.c_str(), sizeof(stats.found_key) - 1);
    }
    
    return stats;
}

bool KangarooSolver::save_checkpoint(const std::string& filename) const {
    return save_checkpoint_to_file(*this, filename);
}

bool KangarooSolver::load_checkpoint(const std::string& filename) {
    return load_checkpoint_from_file(*this, filename);
}

// C interface implementation
extern "C" {
    bool kangaroo_init(const char* pubkey, const char* range_start, const char* range_end, int threads, int dist_bits) {
        try {
            g_solver = std::make_unique<KangarooSolver>();
            return g_solver->initialize(pubkey, range_start, range_end, threads, dist_bits);
        } catch (...) {
            return false;
        }
    }
    
    bool kangaroo_start() {
        if (!g_solver) return false;
        return g_solver->start();
    }
    
    void kangaroo_stop() {
        if (g_solver) {
            g_solver->stop();
        }
    }
    
    bool kangaroo_get_stats(KangarooStats* stats) {
        if (!g_solver || !stats) return false;
        *stats = g_solver->get_stats();
        return true;
    }
    
    bool kangaroo_save_checkpoint(const char* filename) {
        if (!g_solver || !filename) return false;
        return g_solver->save_checkpoint(filename);
    }
    
    bool kangaroo_load_checkpoint(const char* filename) {
        if (!g_solver || !filename) return false;
        return g_solver->load_checkpoint(filename);
    }
}
