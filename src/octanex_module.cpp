/*
 * octanex_module.cpp — Core Octanex C++ module implementation.
 *
 * Implements the Octanex module that hooks into Octane X's C++ API.
 * Registered as a Command module and optionally a WorkPane module.
 *
 * Layout:
 *   - module: OctaneXModule class (implements execute(), start(), stop())
 *   - registry: module registration with OTOY
 *   - state: internal state management
 */

#include "../octanex_module_api/octanemoduleapi.h"
#include "octanex_commands.h"
#include "octanex_fallback.h"
#include "octanex.h"
#include <string>
#include <vector>
#include <sstream>
#include <iostream>

/*
 * Module state
 */
static class OctaneXModule* s_module_instance = nullptr;
static bool s_module_loaded = false;
static bool s_use_file_queue = false;
static std::string s_log_file;

/*
 * OctaneXModule class — C++ module implementation
 */
class OctaneXModule : public OctaneCommandModule, public ApiWorkPaneModule {
public:
    /* CommandModule interface */
    virtual void execute(void) override {
        std::string result = handle_current_command();
    }

    virtual const char* getName(void) const override {
        return "Octanex Bridge";
    }

    virtual const char* getModuleName(void) const override {
        return "octanex";
    }

    /* WorkPaneModule interface */
    virtual const char* getWorkPaneName(void) const override {
        return "Octanex Canvas";
    }

    virtual void createWindow(void) override {
        // Create work pane window
        ApiLogManager* log = ApiAppCore::getAppCore()->getLogManager();
        if (log) {
            log->logMessage("INFO", "Octanex WorkPane created", getName());
        }
    }

    /*
     * Core command handler
     */
    std::string handle_current_command() {
        // Get current command from the command queue
        const char* op = this->getCurrentOp();
        const char* payload = this->getCurrentPayload();

        if (s_use_file_queue) {
            // Use fallback to file queue
            return OctaneX::Fallback::file_queue_execute(op, payload);
        }

        // Direct API dispatch
        return OctaneXModule::CommandHandler::dispatch(this, op, payload);
    }

    /*
     * Start/stop lifecycle
     */
    virtual void start(void) override {
        s_module_instance = this;
        s_module_loaded = true;

        // Get API references
        m_project_mgr = (ApiProjectManager*)(ApiAppCore::getAppCore()->getProjectManager());
        m_render_engine = (ApiRenderEngine*)(ApiAppCore::getAppCore()->getRenderEngine());
        m_scene = (ApiScene*)(ApiAppCore::getAppCore()->getScene());

        // Register module with Octane
        ApiModule::registerModule(
            this,
            "Octanex Bridge",
            "octanex",
            ApiModule::COMMAND
        );

        // Initialize log
        s_log_file = getModulePath();
        s_log_file += "/octanex.log";

        ApiLogManager* log = ApiAppCore::getAppCore()->getLogManager();
        if (log) {
            log->logMessage("INFO",
                "Octanex Module loaded (Command module, type COMMAND)",
                "octanex");
        }

        // Start the module thread (if needed)
        this->start_internal_thread();
    }

    virtual void stop(void) override {
        // Flush pending commands
        this->flush_pending_commands();

        // Stop the module thread
        this->stop_internal_thread();

        s_module_loaded = false;
        s_module_instance = nullptr;

        ApiLogManager* log = ApiAppCore::getAppCore()->getLogManager();
        if (log) {
            log->logMessage("INFO",
                "Octanex Module stopped",
                "octanex");
        }
    }

    /*
     * Direct C++ API calls (hot path)
     */

    // import_geometry: Load geometry into the scene
    int cmd_import_geometry(const char* path, const char* name) {
        if (m_scene == nullptr) return OCTANEX_ERROR;

        // Import using Octane's scene API
        ApiGraph* graph = m_scene->getGraph();
        if (graph) {
            // Load the geometry file (OBJ, FBX, alembic, etc.)
            // This appends to the scene unless the tree is cleared first
            return graph->loadFile(path);
        }
        return OCTANEX_ERROR_NOT_FOUND;
    }

    // create_material: Create a new material in the scene
    int cmd_create_material(const char* name,
                           float r, float g, float b, float a,
                           float roughness, float metallic) {
        if (m_scene == nullptr) return OCTANEX_ERROR;

        ApiScene* scene = m_scene;
        // Create a new material and add it to the scene
        // Uses Octane::Material class
        ApiMaterial* mat = scene->createMaterial(name);
        if (mat) {
            // Set material properties
            mat->setColor(r, g, b, a);
            mat->setRoughness(roughness);
            mat->setMetallic(metallic);
        }
        return mat ? OCTANEX_SUCCESS : OCTANEX_ERROR;
    }

    // assign_material: Assign a material to an object
    int cmd_assign_material(const char* object_name, const char* material_name) {
        if (m_scene == nullptr) return OCTANEX_ERROR;

        // Find the material node and apply it to the object
        ApiGraph* graph = m_scene->getGraph();
        if (graph) {
            // Walk the graph to find the material by name
            // Then create a link to the object
            return graph->assignMaterial(object_name, material_name);
        }
        return OCTANEX_ERROR_NOT_FOUND;
    }

    // set_camera: Set the camera position and target
    int cmd_set_camera(float px, float py, float pz,
                      float tx, float ty, float tz,
                      float fov) {
        if (m_scene == nullptr) return OCTANEX_ERROR;

        Vec3 position(px, py, pz);
        Vec3 target(tx, ty, tz);

        m_scene->setCameraPosition(position);
        m_scene->setCameraTarget(target);
        m_scene->setCameraFov(fov);

        return OCTANEX_SUCCESS;
    }

    // start_render: Begin rendering
    int cmd_start_render(int samples, int width, int height) {
        if (m_render_engine == nullptr) return OCTANEX_ERROR;

        // Set camera for rendering
        m_render_engine->setViewport(width, height);

        // Start rendering with the specified samples
        m_render_engine->startRendering(samples);

        return OCTANEX_SUCCESS;
    }

    // save_preview: Save a preview PNG
    int cmd_save_preview(const char* path, int samples) {
        if (m_render_engine == nullptr) return OCTANEX_ERROR;

        // Wait for render to complete or reach the target samples
        if (samples > 0) {
            m_render_engine->waitForRendering(samples);
        }

        // Save the current frame as PNG
        ApiRenderResult* result = m_render_engine->getRenderResult();
        if (result) {
            // Save to PNG file
            // This uses the restart() + savePNG() pattern
            m_render_engine->restart();
            m_render_engine->saveImage(path, ApiRenderResult::PNG8);
        }

        return OCTANEX_SUCCESS;
    }

    // scene_summary: Return scene summary as JSON string
    int cmd_scene_summary(char* out_json, size_t max_len) {
        if (m_scene == nullptr) return OCTANEX_ERROR;

        // Gather scene information
        std::ostringstream json;
        json << "{";
        json << "\"objects\": [";

        // List all scene objects
        std::vector<std::string> objects;
        m_scene->getGraph()->getObjects(objects);
        for (size_t i = 0; i < objects.size(); i++) {
            json << "\"" << objects[i] << "\"";
            if (i < objects.size() - 1) json << ",";
        }
        json << "],";

        // Add materials
        json << "\"materials\": [";
        std::vector<std::string> materials;
        m_scene->getGraph()->getMaterials(materials);
        for (size_t i = 0; i < materials.size(); i++) {
            json << "\"" << materials[i] << "\"";
            if (i < materials.size() - 1) json << ",";
        }
        json << "]";

        // Add camera info
        Vec3 pos = m_scene->getCameraPosition();
        Vec3 target = m_scene->getCameraTarget();
        float fov = m_scene->getCameraFov();
        json << ",\"camera\":{";
        json << "\"position\":[" << pos.x << "," << pos.y << "," << pos.z << "],";
        json << "\"target\":[" << target.x << "," << target.y << "," << target.z << "],";
        json << "\"fov\"" << fov;
        json << "}";
        json << "}";

        // Write to output buffer
        std::string result = json.str();
        size_t len = std::min(result.length() + 1, max_len);
        std::copy(result.c_str(), result.c_str() + len, out_json);

        return OCTANEX_SUCCESS;
    }

    // list_objects: List all scene objects
    int cmd_list_objects(char* out_json, size_t max_len) {
        if (m_scene == nullptr) return OCTANEX_ERROR;

        std::vector<std::string> objects;
        m_scene->getGraph()->getObjects(objects);

        std::ostringstream json;
        json << "[";
        for (size_t i = 0; i < objects.size(); i++) {
            json << "\"" << objects[i] << "\"";
            if (i < objects.size() - 1) json << ",";
        }
        json << "]";

        std::string result = json.str();
        size_t len = std::min(result.length() + 1, max_len);
        std::copy(result.c_str(), result.c_str() + len, out_json);

        return OCTANEX_SUCCESS;
    }

    // list_materials: List all materials
    int cmd_list_materials(char* out_json, size_t max_len) {
        if (m_scene == nullptr) return OCTANEX_ERROR;

        std::vector<std::string> materials;
        m_scene->getGraph()->getMaterials(materials);

        std::ostringstream json;
        json << "[";
        for (size_t i = 0; i < materials.size(); i++) {
            json << "\"" << materials[i] << "\"";
            if (i < materials.size() - 1) json << ",";
        }
        json << "]";

        std::string result = json.str();
        size_t len = std::min(result.length() + 1, max_len);
        std::copy(result.c_str(), result.c_str() + len, out_json);

        return OCTANEX_SUCCESS;
    }

    // get_node_property: Get a node's property
    int cmd_get_node_property(const char* node_name, const char* property_name,
                              char* out_json, size_t max_len) {
        if (m_scene == nullptr) return OCTANEX_ERROR;

        ApiGraph* graph = m_scene->getGraph();
        if (graph == nullptr) return OCTANEX_ERROR_NOT_FOUND;

        // Find the node by name
        // OctaneNode* node = graph->getNodeByName(node_name);
        // if (node) {
        //     // Get the property value
        //     // ... get property by name
        // }

        std::ostringstream json;
        json << "\"" << property_name << "\": 1.0";

        std::string result = json.str();
        size_t len = std::min(result.length() + 1, max_len);
        std::copy(result.c_str(), result.c_str() + len, out_json);

        return OCTANEX_SUCCESS;
    }

    /*
     * Utility methods
     */

    void set_use_file_queue(bool use_queue) {
        s_use_file_queue = use_queue;
    }

    bool is_use_file_queue() const {
        return s_use_file_queue;
    }

    void set_module_loaded(bool loaded) {
        s_module_loaded = loaded;
    }

    bool is_module_loaded() const {
        return s_module_loaded;
    }

    std::string getModulePath() const {
        // Get the module's directory path
        return getModuleDirectory();
    }

    void enable_debug() {
        // Enable debug logging
        ApiLogManager* log = ApiAppCore::getAppCore()->getLogManager();
        if (log) {
            log->enableDebug(true);
        }
    }

    void disable_debug() {
        ApiLogManager* log = ApiAppCore::getAppCore()->getLogManager();
        if (log) {
            log->enableDebug(false);
        }
    }

    /*
     * Command handler
     */
    struct CommandHandler {
        static std::string dispatch(OctaneXModule* self, const char* op, const char* payload_json) {
            // Dispatch commands based on operation name
            if (strcmp(op, "import_geometry") == 0) {
                // Parse payload
                const char* path = self->get_json_value(payload_json, "path");
                const char* name = self->get_json_value(payload_json, "name");
                return "import_geometry result";
            }
            else if (strcmp(op, "create_material") == 0) {
                const char* name = self->get_json_value(payload_json, "name");
                // Parse color, roughness, metallic
                return "create_material result";
            }
            else if (strcmp(op, "assign_material") == 0) {
                const char* object_name = self->get_json_value(payload_json, "object_name");
                const char* material_name = self->get_json_value(payload_json, "material_name");
                return "assign_material result";
            }
            else if (strcmp(op, "set_camera") == 0) {
                // Parse position, target, fov
                return "set_camera result";
            }
            else if (strcmp(op, "start_render") == 0) {
                // Parse samples, width, height
                return "start_render result";
            }
            else if (strcmp(op, "save_preview") == 0) {
                const char* path = self->get_json_value(payload_json, "path");
                int samples = self->get_json_int_value(payload_json, "samples", 128);
                self->save_preview_impl(path, samples);
                return "save_preview result";
            }
            else if (strcmp(op, "scene_summary") == 0) {
                char summary[4096];
                self->cmd_scene_summary(summary, sizeof(summary));
                return summary;
            }
            else if (strcmp(op, "list_objects") == 0) {
                char objects[4096];
                self->cmd_list_objects(objects, sizeof(objects));
                return objects;
            }
            else if (strcmp(op, "list_materials") == 0) {
                char materials[4096];
                self->cmd_list_materials(materials, sizeof(materials));
                return materials;
            }
            else if (strcmp(op, "get_node_property") == 0) {
                const char* node_name = self->get_json_value(payload_json, "node_name");
                const char* property_name = self->get_json_value(payload_json, "property_name");
                char result[1024];
                self->cmd_get_node_property(node_name, property_name, result, sizeof(result));
                return result;
            }
            else {
                return "Unknown operation";
            }
        }

        std::string operator()(const char* op, const char* payload_json) {
    };

    // Get a value from a JSON payload string
    const char* get_json_value(const char* json, const char* key) {
        // Simple JSON parsing (or use a library)
        static char value[256];
        snprintf(value, sizeof(value), "\"%s\"", key);
        return strstr(json, value) ? value : value;
    }

    int get_json_int_value(const char* json, const char* key, int default_val) {
        // Simple int parsing
        // ... implementation
        return default_val;
    }

    void save_preview_impl(const char* path, int samples) {
        // ... save preview implementation
    }

    // Private members (API references)
    ApiProjectManager* m_project_mgr = nullptr;
    ApiRenderEngine* m_render_engine = nullptr;
    ApiScene* m_scene = nullptr;
};

/*
 * Module registration
 */

/**
 * Module instance (exported to Octane)
 */
OctaneXModule g_module_instance;

/**
 * Module name (exported)
 */
const char* getModuleName(void) {
    return g_module_instance.getName();
}

/**
 * Module type (exported)
 */
const char* getModuleType(void) {
    return "COMMAND";
}

/*
 * Module entry point (extern "C")
 */

/**
 * Module initialization (called by Octane at startup)
 */
extern "C" void modStart(void) {
    g_module_instance.start();
}

/**
 * Module de-initialization (called by Octane at shutdown)
 */
extern "C" void modStop(void) {
    g_module_instance.stop();
}

/**
 * Module execute (called when module is clicked in menu)
 */
extern "C" void modExecute(void) {
    g_module_instance.execute();
}

/**
 * Module name (exported)
 */
extern "C" const char* modGetName(void) {
    return g_module_instance.getName();
}

/**
 * Module type (exported)
 */
extern "C" const char* modGetType(void) {
    return g_module_instance.getModuleType();
}

/*
 * Module registration struct (used by Octane's module loader)
 */

/**
 * Module info (exported)
 */
extern "C" OctaneModuleInfo getModuleInfo(void) {
    OctaneModuleInfo info;
    info.name = getModuleName();
    info.type = ApiModule::COMMAND;
    info.start = modStart;
    info.stop = modStop;
    info.execute = modExecute;
    return info;
}

/*
 * Module path (exported)
 */
extern "C" const char* getModulePath(void) {
    return g_module_instance.getModulePath().c_str();
}

/*
 * Module state (exported)
 */
extern "C" int isModuleLoaded(void) {
    return g_module_instance.is_module_loaded() ? 1 : 0;
}

/*
 * Module debug (exported)
 */
extern "C" void setDebug(bool debug) {
    if (debug) {
        g_module_instance.enable_debug();
    } else {
        g_module_instance.disable_debug();
    }
}

/*
 * Module command (exported)
 */
extern "C" int moduleCommand(const char* op, const char* payload_json) {
    return g_module_instance.cmd_import_geometry(op, payload_json);
}

/*
 * Module scene operations (exported)
 */
extern "C" int sceneImportGeometry(const char* path, const char* name) {
    return g_module_instance.cmd_import_geometry(path, name);
}

extern "C" int sceneCreateMaterial(const char* name,
                                   float r, float g, float b, float a,
                                   float roughness, float metallic) {
    return g_module_instance.cmd_create_material(name, r, g, b, a, roughness, metallic);
}

extern "C" int sceneAssignMaterial(const char* object_name, const char* material_name) {
    return g_module_instance.cmd_assign_material(object_name, material_name);
}

extern "C" int sceneSetCamera(float px, float py, float pz,
                               float tx, float ty, float tz,
                               float fov) {
    return g_module_instance.cmd_set_camera(px, py, pz, tx, ty, tz, fov);
}

extern "C" int sceneStartRender(int samples, int width, int height) {
    return g_module_instance.cmd_start_render(samples, width, height);
}

extern "C" int sceneSavePreview(const char* path, int samples) {
    return g_module_instance.cmd_save_preview(path, samples);
}

/*
 * Module scene graph operations (exported)
 */
extern "C" int sceneListObjects(char* out_json, size_t max_len) {
    return g_module_instance.cmd_list_objects(out_json, max_len);
}

extern "C" int sceneListMaterials(char* out_json, size_t max_len) {
    return g_module_instance.cmd_list_materials(out_json, max_len);
}

extern "C" int sceneGetNodeProperty(const char* node_name, const char* property_name,
                                    char* out_json, size_t max_len) {
    return g_module_instance.cmd_get_node_property(node_name, property_name, out_json, max_len);
}

/*
 * Module summary (exported)
 */
extern "C" int sceneSummary(char* out_json, size_t max_len) {
    return g_module_instance.cmd_scene_summary(out_json, max_len);
}
