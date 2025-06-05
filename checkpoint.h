#ifndef CHECKPOINT_H
#define CHECKPOINT_H

#include <string>
#include <fstream>
#include <vector>

// Forward declaration
class KangarooSolver;

struct CheckpointData {
    std::string version;
    uint64_t timestamp;
    uint64_t total_jumps;
    uint64_t distinguished_points_count;
    std::string range_start;
    std::string range_end;
    int num_threads;
    int distinguished_bits;
    
    // Distinguished points data
    std::vector<std::string> dp_points;
    std::vector<std::string> dp_distances;
    std::vector<bool> dp_is_tame;
    std::vector<uint64_t> dp_timestamps;
};

class CheckpointManager {
public:
    static bool save_checkpoint(const KangarooSolver& solver, const std::string& filename);
    static bool load_checkpoint(KangarooSolver& solver, const std::string& filename);
    
    static bool backup_checkpoint(const std::string& original_filename);
    static std::vector<std::string> list_checkpoints(const std::string& directory = ".");
    
    static bool validate_checkpoint(const std::string& filename);
    static CheckpointData get_checkpoint_info(const std::string& filename);
    
private:
    static bool write_binary_checkpoint(const CheckpointData& data, const std::string& filename);
    static bool read_binary_checkpoint(CheckpointData& data, const std::string& filename);
    
    static bool write_json_checkpoint(const CheckpointData& data, const std::string& filename);
    static bool read_json_checkpoint(CheckpointData& data, const std::string& filename);
    
    static std::string get_checkpoint_version();
    static uint64_t get_current_timestamp();
};

// Convenience functions
bool save_checkpoint_to_file(const KangarooSolver& solver, const std::string& filename);
bool load_checkpoint_from_file(KangarooSolver& solver, const std::string& filename);

#endif // CHECKPOINT_H
