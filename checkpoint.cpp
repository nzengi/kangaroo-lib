#include "checkpoint.h"
#include "kangaroo_solver.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <chrono>
#include <filesystem>
#include <algorithm>
#include <json/json.h>

std::string CheckpointManager::get_checkpoint_version() {
    return "1.0.0";
}

uint64_t CheckpointManager::get_current_timestamp() {
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    return std::chrono::duration_cast<std::chrono::seconds>(duration).count();
}

bool CheckpointManager::save_checkpoint(const KangarooSolver& solver, const std::string& filename) {
    try {
        CheckpointData data;
        
        // Get current statistics
        KangarooStats stats = solver.get_stats();
        
        // Fill checkpoint data
        data.version = get_checkpoint_version();
        data.timestamp = get_current_timestamp();
        data.total_jumps = stats.total_jumps;
        data.distinguished_points_count = stats.distinguished_points;
        data.range_start = stats.current_range_start;
        data.range_end = stats.current_range_end;
        data.num_threads = stats.threads_active;
        data.distinguished_bits = 20; // Default value, should be stored in solver
        
        // Save as JSON for human readability
        bool success = write_json_checkpoint(data, filename);
        
        if (success) {
            std::cout << "Checkpoint saved successfully to " << filename << std::endl;
            
            // Create backup if enabled
            backup_checkpoint(filename);
        }
        
        return success;
        
    } catch (const std::exception& e) {
        std::cerr << "Error saving checkpoint: " << e.what() << std::endl;
        return false;
    }
}

bool CheckpointManager::load_checkpoint(KangarooSolver& solver, const std::string& filename) {
    try {
        if (!std::filesystem::exists(filename)) {
            std::cout << "Checkpoint file not found: " << filename << std::endl;
            return false;
        }
        
        CheckpointData data;
        bool success = read_json_checkpoint(data, filename);
        
        if (success) {
            std::cout << "Checkpoint loaded successfully from " << filename << std::endl;
            std::cout << "  Version: " << data.version << std::endl;
            std::cout << "  Total jumps: " << data.total_jumps << std::endl;
            std::cout << "  Distinguished points: " << data.distinguished_points_count << std::endl;
            
            // Note: In a complete implementation, we would restore the solver state
            // This would require access to the solver's internal state
        }
        
        return success;
        
    } catch (const std::exception& e) {
        std::cerr << "Error loading checkpoint: " << e.what() << std::endl;
        return false;
    }
}

bool CheckpointManager::write_json_checkpoint(const CheckpointData& data, const std::string& filename) {
    try {
        Json::Value root;
        
        // Metadata
        root["version"] = data.version;
        root["timestamp"] = static_cast<Json::UInt64>(data.timestamp);
        root["total_jumps"] = static_cast<Json::UInt64>(data.total_jumps);
        root["distinguished_points_count"] = static_cast<Json::UInt64>(data.distinguished_points_count);
        root["range_start"] = data.range_start;
        root["range_end"] = data.range_end;
        root["num_threads"] = data.num_threads;
        root["distinguished_bits"] = data.distinguished_bits;
        
        // Distinguished points
        Json::Value dp_array(Json::arrayValue);
        for (size_t i = 0; i < data.dp_points.size(); ++i) {
            Json::Value dp_entry;
            dp_entry["point"] = data.dp_points[i];
            dp_entry["distance"] = data.dp_distances[i];
            dp_entry["is_tame"] = data.dp_is_tame[i];
            dp_entry["timestamp"] = static_cast<Json::UInt64>(data.dp_timestamps[i]);
            dp_array.append(dp_entry);
        }
        root["distinguished_points"] = dp_array;
        
        // Write to file
        std::ofstream file(filename);
        if (!file.is_open()) {
            return false;
        }
        
        Json::StreamWriterBuilder builder;
        builder["indentation"] = "  ";
        std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());
        writer->write(root, &file);
        
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Error writing JSON checkpoint: " << e.what() << std::endl;
        return false;
    }
}

bool CheckpointManager::read_json_checkpoint(CheckpointData& data, const std::string& filename) {
    try {
        std::ifstream file(filename);
        if (!file.is_open()) {
            return false;
        }
        
        Json::Value root;
        Json::CharReaderBuilder builder;
        std::string errors;
        
        if (!Json::parseFromStream(builder, file, &root, &errors)) {
            std::cerr << "JSON parse error: " << errors << std::endl;
            return false;
        }
        
        // Read metadata
        data.version = root["version"].asString();
        data.timestamp = root["timestamp"].asUInt64();
        data.total_jumps = root["total_jumps"].asUInt64();
        data.distinguished_points_count = root["distinguished_points_count"].asUInt64();
        data.range_start = root["range_start"].asString();
        data.range_end = root["range_end"].asString();
        data.num_threads = root["num_threads"].asInt();
        data.distinguished_bits = root["distinguished_bits"].asInt();
        
        // Read distinguished points
        const Json::Value& dp_array = root["distinguished_points"];
        if (dp_array.isArray()) {
            data.dp_points.clear();
            data.dp_distances.clear();
            data.dp_is_tame.clear();
            data.dp_timestamps.clear();
            
            for (const auto& dp_entry : dp_array) {
                data.dp_points.push_back(dp_entry["point"].asString());
                data.dp_distances.push_back(dp_entry["distance"].asString());
                data.dp_is_tame.push_back(dp_entry["is_tame"].asBool());
                data.dp_timestamps.push_back(dp_entry["timestamp"].asUInt64());
            }
        }
        
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Error reading JSON checkpoint: " << e.what() << std::endl;
        return false;
    }
}

bool CheckpointManager::backup_checkpoint(const std::string& original_filename) {
    try {
        if (!std::filesystem::exists(original_filename)) {
            return false;
        }
        
        // Create backup filename with timestamp
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        std::stringstream ss;
        ss << original_filename << ".backup." << time_t;
        std::string backup_filename = ss.str();
        
        // Copy file
        std::filesystem::copy_file(original_filename, backup_filename, 
                                 std::filesystem::copy_options::overwrite_existing);
        
        std::cout << "Backup created: " << backup_filename << std::endl;
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Error creating backup: " << e.what() << std::endl;
        return false;
    }
}

std::vector<std::string> CheckpointManager::list_checkpoints(const std::string& directory) {
    std::vector<std::string> checkpoints;
    
    try {
        for (const auto& entry : std::filesystem::directory_iterator(directory)) {
            if (entry.is_regular_file()) {
                std::string filename = entry.path().filename().string();
                
                // Look for checkpoint files (*.dat, *.json, *.checkpoint)
                if (filename.find("checkpoint") != std::string::npos ||
                    filename.ends_with(".dat") ||
                    filename.ends_with(".json")) {
                    checkpoints.push_back(entry.path().string());
                }
            }
        }
        
        // Sort by modification time (newest first)
        std::sort(checkpoints.begin(), checkpoints.end(), 
                 [](const std::string& a, const std::string& b) {
                     auto time_a = std::filesystem::last_write_time(a);
                     auto time_b = std::filesystem::last_write_time(b);
                     return time_a > time_b;
                 });
        
    } catch (const std::exception& e) {
        std::cerr << "Error listing checkpoints: " << e.what() << std::endl;
    }
    
    return checkpoints;
}

bool CheckpointManager::validate_checkpoint(const std::string& filename) {
    try {
        if (!std::filesystem::exists(filename)) {
            return false;
        }
        
        CheckpointData data;
        bool success = read_json_checkpoint(data, filename);
        
        if (success) {
            // Basic validation
            if (data.version.empty() || data.timestamp == 0) {
                return false;
            }
            
            // Check if distinguished points data is consistent
            if (data.dp_points.size() != data.dp_distances.size() ||
                data.dp_points.size() != data.dp_is_tame.size() ||
                data.dp_points.size() != data.dp_timestamps.size()) {
                return false;
            }
            
            return true;
        }
        
        return false;
        
    } catch (const std::exception& e) {
        std::cerr << "Error validating checkpoint: " << e.what() << std::endl;
        return false;
    }
}

CheckpointData CheckpointManager::get_checkpoint_info(const std::string& filename) {
    CheckpointData data;
    
    try {
        if (validate_checkpoint(filename)) {
            read_json_checkpoint(data, filename);
        }
    } catch (const std::exception& e) {
        std::cerr << "Error getting checkpoint info: " << e.what() << std::endl;
    }
    
    return data;
}

// Convenience functions
bool save_checkpoint_to_file(const KangarooSolver& solver, const std::string& filename) {
    return CheckpointManager::save_checkpoint(solver, filename);
}

bool load_checkpoint_from_file(KangarooSolver& solver, const std::string& filename) {
    return CheckpointManager::load_checkpoint(solver, filename);
}
