/*
 * octanex.c — extern "C" bridge layer.
 *
 * This file provides the C interface that Python FFI (via ctypes)
 * and other callers can use directly. It wraps the C++ module
 * and provides a stable ABI.
 */

#include "octanex.h"
#include "octanex_module.cpp"  // Include the C++ implementation
#include <stdlib.h>
#include <string.h>

/*
 * Module state
 */
static int s_module_loaded = 0;
static char s_log_buffer[1024] = {0};
static int s_log_buffer_len = 0;

/*
 * Bridge implementation (delegates to C++ module)
 */

/**
 * Start the module.
 */
int octanex_start_impl() {
    // Call the C++ start method
    int result = s_module_instance->cmd_start_render(0, 0, 0);
    if (result == OCTANEX_SUCCESS) {
        s_module_loaded = 1;
        
        // Log the start
        std::ostringstream log;
        log << "Octanex module started";
        std::string log_str = log.str();
        strcpy(s_log_buffer, log_str.c_str());
        
        ApiLogManager* log_manager = ApiAppCore::getAppCore()->getLogManager();
        if (log_manager) {
            log_manager->logMessage("INFO", log_str.c_str(), "octanex");
        }
    }
    return result;
}

/**
 * Stop the module.
 */
void octanex_stop_impl() {
    s_module_instance->stop();
    s_module_loaded = 0;
}

/**
 * Command execution.
 */
char* octanex_command_impl(const char* op, const char* payload_json) {
    // Delegate to the C++ module
    std::string result = s_module_instance->handle_current_command();
    
    // Allocate and return the result
    char* result_str = (char*)malloc(result.length() + 1);
    strcpy(result_str, result.c_str());
    
    return result_str;
}

/**
 * Free a result string.
 */
void octanex_free_impl(void* ptr) {
    free(ptr);
}

/*
 * Command functions
 */

/**
 * Import geometry.
 */
int octanex_cmd_import_geometry_impl(const char* path, const char* name) {
    return s_module_instance->cmd_import_geometry(path, name);
}

/**
 * Create material.
 */
int octanex_cmd_create_material_impl(const char* name,
                                     float r, float g, float b, float a,
                                     float roughness, float metallic) {
    return s_module_instance->cmd_create_material(
        name, r, g, b, a, roughness, metallic
    );
}

/**
 * Assign material.
 */
int octanex_cmd_assign_material_impl(const char* object_name, const char* material_name) {
    return s_module_instance->cmd_assign_material(object_name, material_name);
}

/**
 * Set camera.
 */
int octanex_cmd_set_camera_impl(float px, float py, float pz,
                                float tx, float ty, float tz,
                                float fov) {
    return s_module_instance->cmd_set_camera(px, py, pz, tx, ty, tz, fov);
}

/**
 * Start render.
 */
int octanex_cmd_start_render_impl(int samples, int width, int height) {
    return s_module_instance->cmd_start_render(samples, width, height);
}

/**
 * Save preview.
 */
int octanex_cmd_save_preview_impl(const char* path, int samples) {
    return s_module_instance->cmd_save_preview(path, samples);
}

/**
 * Scene summary.
 */
int octanex_cmd_scene_summary_impl(char* out_json, size_t max_len) {
    return s_module_instance->cmd_scene_summary(out_json, max_len);
}

/*
 * Scene graph queries
 */

/**
 * List objects.
 */
int octanex_cmd_list_objects_impl(char* out_json, size_t max_len) {
    return s_module_instance->cmd_list_objects(out_json, max_len);
}

/**
 * List materials.
 */
int octanex_cmd_list_materials_impl(char* out_json, size_t max_len) {
    return s_module_instance->cmd_list_materials(out_json, max_len);
}

/**
 * Get node property.
 */
int octanex_cmd_get_node_property_impl(const char* node_name, 
                                       const char* property_name,
                                       char* out_json, size_t max_len) {
    return s_module_instance->cmd_get_node_property(node_name, property_name, out_json, max_len);
}

/*
 * Fallback functions
 */

/**
 * Use file queue.
 */
int octanex_use_file_queue_impl() {
    s_module_instance->set_use_file_queue(true);
    return s_module_loaded;
}

/**
 * Use direct API.
 */
int octanex_use_direct_api_impl() {
    s_module_instance->set_use_file_queue(false);
    return s_module_loaded;
}

/**
 * Check if module is available.
 */
int octanex_is_available_impl() {
    return s_module_loaded ? 1 : 0;
}

/*
 * Logging
 */

/**
 * Log a message.
 */
void octanex_log_impl(const char* message) {
    ApiLogManager* log = ApiAppCore::getAppCore()->getLogManager();
    if (log) {
        log->logMessage("INFO", message, "octanex");
    }
}

/**
 * Enable debug logging.
 */
void octanex_enable_debug_impl() {
    s_module_instance->enable_debug();
}

/*
 * Memory management
 */

/**
 * Free allocated memory.
 */
void octanex_free_impl_v2(void* ptr) {
    if (ptr) {
        free(ptr);
    }
}

/*
 * C interface (extern "C")
 */

extern "C" {

/*
 * Module lifecycle
 */

/**
 * Start the module.
 */
int octanex_start(void) {
    return octanex_start_impl();
}

/**
 * Stop the module.
 */
void octanex_stop(void) {
    octanex_stop_impl();
}

/*
 * Command execution
 */

/**
 * Execute a command.
 */
char* octanex_command(const char* op, const char* payload_json) {
    return octanex_command_impl(op, payload_json);
}

/**
 * Free a result string.
 */
octanex_free(void* ptr) {
    free(ptr);
}

/*
 * Core commands
 */

/**
 * Import geometry.
 */
int octanex_cmd_import_geometry(const char* path, const char* name) {
    return octanex_cmd_import_geometry_impl(path, name);
}

/**
 * Create material.
 */
int octanex_cmd_create_material(const char* name,
                                float r, float g, float b, float a,
                                float roughness, float metallic) {
    return octanex_cmd_create_material_impl(name, r, g, b, a, roughness, metallic);
}

/**
 * Assign material.
 */
int octanex_cmd_assign_material(const char* object_name, const char* material_name) {
    return octanex_cmd_assign_material_impl(object_name, material_name);
}

/**
 * Set camera.
 */
int octanex_cmd_set_camera(float px, float py, float pz,
                           float tx, float ty, float tz,
                           float fov) {
    return octanex_cmd_set_camera_impl(px, py, pz, tx, ty, tz, fov);
}

/**
 * Start render.
 */
int octanex_cmd_start_render(int samples, int width, int height) {
    return octanex_cmd_start_render_impl(samples, width, height);
}

/**
 * Save preview.
 */
int octanex_cmd_save_preview(const char* path, int samples) {
    return octanex_cmd_save_preview_impl(path, samples);
}

/**
 * Scene summary.
 */
int octanex_cmd_scene_summary(char* out_json, size_t max_len) {
    return octanex_cmd_scene_summary_impl(out_json, max_len);
}

/*
 * Scene graph queries
 */

/**
 * List objects.
 */
int octanex_list_objects(char* out_json, size_t max_len) {
    return octanex_cmd_list_objects_impl(out_json, max_len);
}

/**
 * List materials.
 */
int octanex_list_materials(char* out_json, size_t max_len) {
    return octanex_cmd_list_materials_impl(out_json, max_len);
}

/**
 * Get node property.
 */
int octanex_get_node_property(const char* node_name, const char* property_name) {
    char out_json[256];
    return octanex_cmd_get_node_property_impl(node_name, property_name, out_json, sizeof(out_json));
}

/*
 * Fallback
 */

/**
 * Use file queue.
 */
int octanex_use_file_queue(void) {
    return octanex_use_file_queue_impl();
}

/**
 * Use direct API.
 */
int octanex_use_direct_api(void) {
    return octanex_use_direct_api_impl();
}

/**
 * Check if module is available.
 */
int octanex_is_available(void) {
    return octanex_is_available_impl();
}

/*
 * Logging
 */

/**
 * Log a message.
 */
void octanex_log(const char* message) {
    octanex_log_impl(message);
}

/**
 * Enable debug logging.
 */
void octanex_enable_debug() {
    octanex_enable_debug_impl();
}

/**
 * Free allocated memory.
 */
void octanex_free(void* ptr) {
    octanex_free_impl_v2(ptr);
}

} // extern "C"
