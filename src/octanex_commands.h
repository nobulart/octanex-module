/*
 * octanex_commands.h — Command dispatch table and helpers.
 *
 * Defines the command dispatch table that maps operation names
 * to handler functions. This is the bridge between the Python
 * JSON commands and the C++ module.
 */

#ifndef OCTANEX_COMMANDS_H
#define OCTANEX_COMMANDS_H

#include <string>
#include <vector>

/*
 * OTOY MODULE API types (vendored)
 *
 * These types match OTOY's API structure. They include:
 *   - OctaneModuleInfo: module registration info
 *   - ApiModule: base module class
 *   - ApiProjectManager
 *   - ApiRenderEngine
 *   - ApiScene
 *   - ApiGraph
 *   - OctaneX: Octane X specific types
 *   - ApiLogManager
 *   - ApiRenderResult
 */

/*
 * Module registration macros (from OTOY)
 *
 * OCTANE_MODULE_REGISTER() registers a module with Octane.
 * Called by the module's start() function.
 */

#define OCTANE_MODULE_TYPE_COMMAND   "COMMAND"
#define OCTANE_MODULE_TYPE_WORK_PANE "WORK_PANE"
#define OCTANE_MODULE_TYPE_BOTH      "COMMAND|WORK_PANE"

#define OCTANE_MODULE_REGISTER(name, display_name, module_id, module_type) \
    OctaneModuleInfo info; \
    info.name = name; \
    info.display_name = display_name; \
    info.module_id = module_id; \
    info.type = module_type; \
    ApiModule::registerModule(&info);

#define OCTANE_MODULE_NAME      "Octanex Bridge"
#define OCTANE_MODULE_ID        "octanex"
#define OCTANE_MODULE_TYPE      OCTANE_MODULE_TYPE_COMMAND

/*
 * Command dispatch
 */

// Command handler signature
typedef int (*CommandHandler)(const char* payload_json, char* out_json, size_t max_len);

// Command entry
struct CommandEntry {
    const char* op;           // Operation name (e.g., "import_geometry")
    CommandHandler handler;   // Handler function
    const char* description;  // Human-readable description
};

// Command dispatch table
extern const CommandEntry COMMAND_TABLE[];
extern const size_t COMMAND_TABLE_SIZE;

/*
 * Command execution
 */

// Execute a command by name
int octave_execute_command(const char* op, const char* payload_json);

// Register a command handler
int register_command(const char* op, CommandHandler handler, const char* description);

// Unregister a command handler
int unregister_command(const char* op);

/*
 * JSON helpers
 */

// Parse a JSON string (simple implementation)
// Returns a pointer to the parsed value, or nullptr on error
const char* parse_json_value(const char* json, const char* key);

// Parse a JSON value as int
int parse_json_int(const char* json, const char* key, int default_val);

// Parse a JSON value as float
float parse_json_float(const char* json, const char* key, float default_val);

/*
 * Utility functions
 */

// Convert a command string to lowercase
char* to_lower(const char* str);

// Escape a JSON string
char* escape_json_string(const char* str);

// Free a JSON string
void free_json_string(char* str);

/*
 * OTOY module API types (vendored)
 */

// OctaneModuleInfo: module registration info
struct OctaneModuleInfo {
    const char* name;          // Module name (e.g., "OctaneX")
    const char* display_name;  // Display name (e.g., "Octane X Module")
    const char* module_id;     // Module ID (e.g., "module_id")
    const char* type;          // Module type (e.g., "COMMAND|WORK_PANE")
};

// Module registration
void register_module(const OctaneModuleInfo* info);

// Module unregistration
void unregister_module(const char* module_id);

/*
 * OTOY API types (vendored)
 */

// Octane X types
typedef struct {
    float x, y, z;
} vec3_t;

typedef struct {
    float x, y, z, w;
} vec4_t;

typedef struct {
    float m[12];           // 3x4 matrix (row-major)
} mat3x4_t;

/*
 * OTOY API functions (extern C)
 */

// ProjectManager methods
typedef struct {
    void (*loadProject)(const char* path);
    void (*saveProject)(const char* path);
    void (*createProject)(const char* name);
} ProjectManagerFuncs;

// RenderEngine methods
typedef struct {
    void (*startRendering)(int samples);
    void (*stopRendering)(void);
    void (*restart)(void);
    void (*waitForRendering)(int samples);
    void (*setViewport)(int width, int height);
    void (*saveImage)(const char* path, int format);
    vec3_t (*getCameraPosition)(void);
    void (*setCameraPosition)(vec3_t pos);
    vec3_t (*getCameraTarget)(void);
    void (*setCameraTarget)(vec3_t target);
    float (*getCameraFov)(void);
    void (*setCameraFov)(float fov);
} RenderEngineFuncs;

// Scene methods
typedef struct {
    void (*getObjects)(std::vector<std::string>* objects);
    void (*getMaterials)(std::vector<std::string>* materials);
    int (*getGraph)(void);
} SceneFuncs;

/*
 * OTOY logging
 */

typedef enum {
    LOG_LEVEL_DEBUG = 0,
    LOG_LEVEL_INFO = 1,
    LOG_LEVEL_WARNING = 2,
    LOG_LEVEL_ERROR = 3
} LogLevel;

extern void moduleLog(LogLevel level, const char* message, const char* module_id);

#endif /* OCTANEX_COMMANDS_H */
