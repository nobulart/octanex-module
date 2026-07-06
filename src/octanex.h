/*
 * octanex.h — Public header for the Octanex C++ module.
 *
 * This header defines the extern "C" interface that any caller
 * (Python via ctypes/FFI, Lua via cqueues, or standalone apps)
 * can use to interact with the module.
 *
 * Layout:
 *   - API surface: public functions (extern "C")
 *   - Internal C++: implementation details (octanex_module.cpp)
 *   - Fallback: file-queue bridge for sidecar mode
 */

#ifndef OCTANEX_H
#define OCTANEX_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * STATUS CODES
 */
#define OCTANEX_SUCCESS          0
#define OCTANEX_ERROR           -1
#define OCTANEX_ERROR_INVALID_O  -2
#define OCTANEX_ERROR_NOT_FOUND  -3
#define OCTANEX_ERROR_RENDERING  -4

/*
 * MODULE LIFECYCLE
 *
 * Called by Octane X at startup and shutdown.
 * Must be extern "C" and use stdcall-like calling convention.
 */

/**
 * Start the Octanex module.
 * 
 * Register as a Command module so Octane shows it in the Modules menu.
 * Initialize all internal state, set up the log manager.
 * 
 * @return OCTANEX_SUCCESS on success, negative error code otherwise.
 */
int octanex_start(void);

/**
 * Stop the Octanex module.
 * 
 * Clean up all resources, flush any pending commands.
 * Called by Octane before it exits.
 */
void octanex_stop(void);

/*
 * COMMAND INTERFACE
 *
 * The core command dispatch. Used both:
 * 1. Direct C++ API calls (through the module's execute())
 * 2. FFI calls from Python (via ctypes calling octanex_command)
 */

/**
 * Execute a single command.
 * 
 * The op is matched against the command dispatch table.
 * The payload_json is a JSON object string with command-specific fields.
 * 
 * @param op The operation name (e.g., "import_geometry", "create_material").
 *           Must match one of the ALLOWED_OPS.
 * @param payload_json JSON object string with command parameters.
 * @return JSON result string (caller must call octanex_free(result)).
 */
char* octanex_command(const char* op, const char* payload_json);

/**
 * Free a result string returned by oC tanex_command() or similar.
 */
void octanex_free(void* ptr);

/*
 * CORE COMMANDS (direct C++ API calls — no JSON parsing overhead)
 *
 * Each command returns OCTANEX_SUCCESS on success.
 * These are the hot path: called for every render command.
 */

/**
 * Import geometry (OBJ, FBX, alembic, etc.).
 */
int octanex_cmd_import_geometry(const char* path, const char* name);

/**
 * Create a new material (glossy by default).
 * 
 * @param name        Material name
 * @param r,g,b,a     Base color (0.0 - 1.0)
 * @param roughness   Surface roughness (0.0 = mirror, 1.0 = rough)
 * @param metallic    Metallic factor (0.0 = dielectric, 1.0 = metal)
 */
int octanex_cmd_create_material(const char* name,
                                float r, float g, float b, float a,
                                float roughness, float metallic);

/**
 * Assign a material to an object.
 */
int octanex_cmd_assign_material(const char* object_name, const char* material_name);

/**
 * Set camera position and target.
 */
int octanex_cmd_set_camera(float px, float py, float pz,
                           float tx, float ty, float tz,
                           float fov);

/**
 * Start rendering.
 */
int octanex_cmd_start_render(int samples, int width, int height);

/**
 * Save a preview PNG.
 */
int octanex_cmd_save_preview(const char* path, int samples);

/**
 * Get scene summary (all objects, materials, camera, lights).
 * 
 * @param out_json    Output buffer
 * @param max_len     Maximum buffer length
 * @return OCTANEX_SUCCESS on success
 */
int octanex_cmd_scene_summary(char* out_json, size_t max_len);

/*
 * SCENE GRAPH QUERIES (new capabilities — not in Lua bridge)
 */

/**
 * List all objects in the scene.
 * 
 * Returns a JSON array of object names (caller frees).
 */
char* octanex_list_objects(char* out_json, size_t max_len);

/**
 * List all materials in the scene.
 * 
 * Returns a JSON array of material names (caller frees).
 */
char* octanex_list_materials(char* out_json, size_t max_len);

/**
 * Query a node property.
 * 
 * @param node_name     Node name (e.g., "Cylinder", "Sphere")
 * @param property_name Property name (e.g., "position", "roughness")
 * @return property value as JSON (caller frees)
 */
char* octanex_get_node_property(const char* node_name, const char* property_name);

/*
 * FALLBACK: File-queue operations
 *
 * When the module is running as a sidecar (not deeply integrated),
 * it reads/writes the same JSON queue as the Lua bridge.
 */

/**
 * Switch to file-queue mode.
 */
int octanex_use_file_queue(void);

/**
 * Switch to direct API mode.
 */
int octanex_use_direct_api(void);

/**
 * Check if the module is available.
 */
int octanex_is_available(void);

/*
 * LOGGING
 */

/**
 * Write a log message (goes to Octane's log).
 */
void octanex_log(const char* message);

/**
 * Enable debug logging.
 */
void octanex_enable_debug(void);

#ifdef __cplusplus
}
#endif

#endif /* OCTANEX_H */
