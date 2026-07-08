/*
 * octanex_fallback.cpp — File-queue fallback / shared-command-channel.
 *
 * INDEPENDENCE + COMPOSABILITY:
 *   This is the seam that lets the C++ module and the octanex-mcp (Hermes MCP)
 *   bridge run independently yet together. Each component is fully usable on its
 *   own; when both are deployed they rendezvous on a single `queue/` directory:
 *
 *     queue/
 *     ├── command.json
 *     ├── command_id1.json
 *     └── command_id2.json
 *
 *   - The C++ module reads/writes this queue as a sidecar when the deep Octane
 *     API path is unavailable (g_use_file_queue).
 *   - The MCP bridge (src/octanex_mcp/bridge.py) writes to the SAME queue as its
 *     fallback when the dylib is not loaded.
 *   Because the queue format is shared, either side can pick up the other's
 *   commands without a direct dependency.
 *
 *   This is a fallback path that processes commands from the file queue when the
 *   C++ API is unavailable — and the bridge's default interoperable channel.
 */

#include "octanex_fallback.h"
#include "../octanex_module_api/octanemoduleapi.h"
#include <fstream>
#include <sstream>
#include <cstring>
#include <iostream>

/*
 * Fallback implementation details
 */

namespace OctaneX {
namespace Fallback {

// Fallback state
char s_next_obj_name[256] = {0};
int s_next_obj_id = 1;

/*
 * Path helpers
 */

/**
 * Get the queue directory path.
 */
std::string get_queue_path() {
    // Return the path to the queue directory
    return "queue";
}

/**
 * Get the module directory (where the module is installed).
 */
std::string get_module_path() {
    const char* path = getModuleDirectory();
    return path;
}

/*
 * JSON serialization
 */

/**
 * Write a JSON command to the queue.
 * 
 * @param op        The operation name
 * @param payload   Payload as a JSON string
 * @param path      Output path (optional, if nullptr uses default queue path)
 * @return 0 on success, -1 on error
 */
int write_queue_command(const char* op, const char* payload, const char* path) {
    std::string full_path = path ? path : get_queue_path();
    
    // Build the JSON object
    std::ostringstream json;
    json << "{\"op\":\"" << op << "\",\"payload\":" << payload << "}";
    
    std::ofstream file(full_path);
    if (file.is_open()) {
        file << json.str();
        file.close();
        return 0;
    }
    return -1;
}

/**
 * Read a JSON command from the file queue.
 * 
 * @param op          Output: operation name
 * @param payload     Output: payload JSON string
 * @param path        Input: path to read (optional)
 */
int read_queue_command(char* op, char* payload, size_t op_max_len, size_t payload_max_len, const char* path) {
    std::string full_path = path ? path : get_queue_path();
    
    std::ifstream file(full_path);
    if (!file.is_open()) {
        return -1;
    }
    
    // Read the JSON
    std::ostringstream json;
    json << file.rdbuf();
    file.close();
    
    // Parse the JSON
    std::string json_str = json.str();
    
    // Extract the op
    size_t op_start = json_str.find("\"op\":\"");
    size_t op_end = json_str.find("\",", op_start);
    if (op_start != std::string::npos && op_end != std::string::npos) {
        size_t op_len = op_end - (op_start + 5);
        if (op_len < op_max_len) {
            strncpy(op, json_str.substr(op_start + 5, op_len).c_str(), op_len);
            op[op_len] = '\0';
        }
    }
    
    // Extract the payload
    size_t payload_start = json_str.find("\"payload\":");
    if (payload_start != std::string::npos) {
        size_t payload_end = json_str.find("}", payload_start);
        if (payload_end != std::string::npos) {
            size_t payload_len = payload_end - payload_start + 1;
            if (payload_len < payload_max_len) {
                strncpy(payload, json_str.substr(payload_start + 10, payload_len).c_str(), payload_len);
                payload[payload_len] = '\0';
            }
        }
    }
    
    return 0;
}

/*
 * Fallback command processing
 */

/**
 * Execute a command via the file queue.
 * 
 * @param op        The operation name
 * @param payload   JSON payload string
 * @return JSON result string (caller frees)
 */
std::string file_queue_execute(const char* op, const char* payload) {
    // Write the command to the file queue
    write_queue_command(op, payload, nullptr);
    
    // Read the result from the file queue
    char result[1024];
    int rc = read_queue_command(
        result, nullptr, sizeof(result), sizeof(result), nullptr
    );
    
    if (rc != 0) {
        return "Error executing command";
    }
    
    return result;
}

/**
 * Get the next object name (for creating unique names).
 */
std::string get_next_obj_name() {
    const char* raw_name = getNextObjName();
    std::string result(raw_name ? raw_name : "octanex_0");
    
    s_next_obj_id++;
    snprintf(s_next_obj_name, sizeof(s_next_obj_name), "octanex_%d", s_next_obj_id);
    
    return result;
}

} // namespace Fallback
} // namespace OctaneX

/*
 * C interface (extern "C")
 */

extern "C" {

/**
 * Write a command to the file queue.
 */
int octanex_fallback_write_command(const char* op, const char* payload, const char* path) {
    return OctaneX::Fallback::write_queue_command(op, payload, path);
}

/**
 * Read a command from the file queue.
 */
int octanex_fallback_read_command(char* op, char* payload, size_t op_max_len, size_t payload_max_len) {
    return OctaneX::Fallback::read_queue_command(op, payload, op_max_len, payload_max_len, nullptr);
}

/**
 * Execute a command via the file queue.
 */
const char* octanex_fallback_execute(const char* op, const char* payload) {
    static std::string result;
    result = OctaneX::Fallback::file_queue_execute(op, payload);
    return result.c_str();
}

/**
 * Get the next object name.
 */
char* octanex_fallback_next_obj_name() {
    std::string name = OctaneX::Fallback::get_next_obj_name();
    static char buffer[256];
    strcpy(buffer, name.c_str());
    return buffer;
}

} // extern "C"
