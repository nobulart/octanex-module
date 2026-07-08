/*
 * octanex_module.cpp - Core Octanex C++ module implementation.
 *
 * Implements the Octanex module that hooks into Octane X's C++ API.
 * Registered as a Command module and optionally as a WorkPane module.
 */

#include "../octanex_module_api/octanemoduleapi.h"
#include "octanex_fallback.h"
#include "octanex.h"
#include <string>
#include <vector>
#include <sstream>
#include <iostream>
#include <cstring>
#include <cstdlib>
#include <cstddef>

/* Module state (static globals for extern "C" access) */
static OctaneXModule* g_module_instance = nullptr;
static bool g_module_loaded = false;
/* g_use_file_queue selects the interoperable command channel.
 * When true, the module talks to Octane X through the shared `queue/`
 * directory rather than the deep C++ API. That queue is also the default
 * command channel of the parallel octanex-mcp (Hermes MCP) bridge, so the
 * two components stay fully independent yet composable: each works alone,
 * and together they rendezvous on the same queue with no direct coupling. */
static bool g_use_file_queue = false;
static std::string g_log_file;

/* OctaneXModule class - full definition */
class OctaneXModule : public OctaneCommandModule,
                     public ApiWorkPaneModule {
public:
    OctaneXModule() {
        m_project_mgr = nullptr;
        m_render_engine = nullptr;
        m_scene = nullptr;
    }
    ~OctaneXModule() override = default;

    /* OctaneCommandModule interface */
    virtual void execute() override {
        const char* op = getCurrentOp();
        (void)op;
    }
    virtual const char* getName() const override {
        return "Octanex Bridge";
    }
    virtual const char* getModuleName() const override {
        return "octanex";
    }
    virtual void start() override;
    virtual void stop() override;

    /* ApiWorkPaneModule interface */
    virtual const char* getWorkPaneName() override {
        return "Octanex Canvas";
    }
    virtual void createWindow() override {
        ApiAppCore* core = ApiAppCore::getAppCore();
        if (core) {
            ApiLogManager* log = core->getLogManager();
            if (log) {
                log->logMessage("INFO", "Octanex WorkPane created", getName());
            }
        }
    }

    /* Geometry */
    int cmd_import_geometry(const char* path, const char* name);
    int cmd_create_material(const char* name,
                            float r, float g, float b, float a,
                            float roughness, float metallic);
    int cmd_assign_material(const char* object_name, const char* material_name);

    /* Camera */
    int cmd_set_camera(float px, float py, float pz,
                      float tx, float ty, float tz,
                      float fov);

    /* Render */
    int cmd_start_render(int samples, int width, int height);
    int cmd_save_preview(const char* path, int samples);

    /* Scene summary */
    int cmd_scene_summary(char* out_json, std::size_t max_len);
    int cmd_list_objects(char* out_json, std::size_t max_len);
    int cmd_list_materials(char* out_json, std::size_t max_len);
    int cmd_get_node_property(const char* node_name, const char* property_name,
                              char* out_json, std::size_t max_len);

    /* Helpers */
    void set_use_file_queue(bool v) { g_use_file_queue = v; }
    bool is_use_file_queue() const { return g_use_file_queue; }
    void set_module_loaded(bool v) { g_module_loaded = v; }
    bool is_module_loaded() const { return g_module_loaded; }
    std::string getModulePath() const { return getModuleDirectory(); }
    void enable_debug();
    void disable_debug();
    const char* get_json_value(const char* json, const char* key);
    int get_json_int_value(const char* json, const char* key, int default_val);
    void save_preview_impl(const char* path, int samples);

    /* Command dispatch */
    std::string handle_current_command();

private:
    ApiProjectManager*   m_project_mgr;
    ApiRenderEngine*     m_render_engine;
    ApiScene*            m_scene;
};

/* Module instance */
OctaneXModule g_octanex_module;

/* --- Implementations --- */

void OctaneXModule::start() {
    g_module_instance = this;
    g_module_loaded = true;

    ApiModule::registerModule(
        reinterpret_cast<ApiModule*>(this),
        "Octanex Bridge",
        "octanex",
        ApiModule::COMMAND
    );

    ApiAppCore* core = ApiAppCore::getAppCore();
    if (core) {
        m_project_mgr = reinterpret_cast<ApiProjectManager*>(
            core->getProjectManager());
        m_render_engine = reinterpret_cast<ApiRenderEngine*>(
            core->getRenderEngine());
        m_scene = reinterpret_cast<ApiScene*>(
            core->getScene());

        ApiLogManager* log = core->getLogManager();
        if (log) {
            log->logMessage("INFO",
                "Octanex module loaded (Command module)",
                "octanex");
        }
    }

    g_log_file = getModuleDirectory();
    g_log_file += "/octanex.log";
    start_internal_thread();
}

void OctaneXModule::stop() {
    flush_pending_commands();
    stop_internal_thread();
    g_module_loaded = false;
    g_module_instance = nullptr;

    ApiAppCore* core = ApiAppCore::getAppCore();
    if (core) {
        ApiLogManager* log = core->getLogManager();
        if (log) {
            log->logMessage("INFO", "Octanex module stopped", "octanex");
        }
    }
}

int OctaneXModule::cmd_import_geometry(const char* path, const char* name) {
    (void)name;
    if (m_scene == nullptr) return -1;
    ApiGraph* graph = m_scene->getGraph();
    if (graph != nullptr) return graph->loadFile(path);
    return -2;
}

int OctaneXModule::cmd_create_material(const char* name,
                                       float r, float g, float b, float a,
                                       float roughness, float metallic) {
    if (m_scene == nullptr) return -1;
    ApiMaterial* mat = m_scene->createMaterial(name);
    if (mat) {
        mat->setColor(r, g, b, a);
        mat->setRoughness(roughness);
        mat->setMetallic(metallic);
    }
    return mat ? 0 : -1;
}

int OctaneXModule::cmd_assign_material(const char* object_name,
                                        const char* material_name) {
    if (m_scene == nullptr) return -1;
    ApiGraph* graph = m_scene->getGraph();
    if (graph != nullptr) return graph->assignMaterial(object_name, material_name);
    return -2;
}

int OctaneXModule::cmd_set_camera(float px, float py, float pz,
                                  float tx, float ty, float tz,
                                  float fov) {
    if (m_scene == nullptr) return -1;
    Vec3 position(px, py, pz);
    Vec3 target(tx, ty, tz);
    m_scene->setCameraPosition(position);
    m_scene->setCameraTarget(target);
    m_scene->setCameraFov(fov);
    return 0;
}

int OctaneXModule::cmd_start_render(int samples, int width, int height) {
    if (m_render_engine == nullptr) return -1;
    m_render_engine->setViewport(width, height);
    m_render_engine->startRendering(samples);
    return 0;
}

int OctaneXModule::cmd_save_preview(const char* path, int samples) {
    if (m_render_engine == nullptr) return -1;
    if (samples > 0) m_render_engine->waitForRendering(samples);
    ApiRenderResult* result = m_render_engine->getRenderResult();
    if (result != nullptr) {
        m_render_engine->restart();
        m_render_engine->saveImage(path, ApiRenderResult::PNG8);
    }
    return 0;
}

int OctaneXModule::cmd_scene_summary(char* out_json, std::size_t max_len) {
    if (m_scene == nullptr) return -1;
    std::ostringstream json;
    json << "{";

    std::vector<std::string> objects;
    m_scene->getGraph()->getObjects(&objects);
    json << "\"objects\":[";
    for (std::size_t i = 0; i < objects.size(); i++) {
        json << "\"" << objects[i] << "\"";
        if (i + 1 < objects.size()) json << ",";
    }
    json << "],";

    std::vector<std::string> materials;
    m_scene->getGraph()->getMaterials(&materials);
    json << "\"materials\":[";
    for (std::size_t i = 0; i < materials.size(); i++) {
        json << "\"" << materials[i] << "\"";
        if (i + 1 < materials.size()) json << ",";
    }
    json << "],";

    Vec3 pos = m_scene->getCameraPosition();
    Vec3 tgt = m_scene->getCameraTarget();
    float fov = m_scene->getCameraFov();
    json << "\"camera\":{\"position\":[" << pos.x << "," << pos.y << "," << pos.z << "],";
    json << "\"target\":[" << tgt.x << "," << tgt.y << "," << tgt.z << "],";
    json << "\"fov\":" << fov << "}";
    json << "}";

    std::string result = json.str();
    std::size_t len = std::min(result.size() + 1, max_len);
    std::copy(result.c_str(), result.c_str() + len, out_json);
    return 0;
}

int OctaneXModule::cmd_list_objects(char* out_json, std::size_t max_len) {
    if (m_scene == nullptr) return -1;
    std::vector<std::string> objects;
    m_scene->getGraph()->getObjects(&objects);

    std::ostringstream json;
    json << "[";
    for (std::size_t i = 0; i < objects.size(); i++) {
        json << "\"" << objects[i] << "\"";
        if (i + 1 < objects.size()) json << ",";
    }
    json << "]";

    std::string result = json.str();
    std::size_t len = std::min(result.size() + 1, max_len);
    std::copy(result.c_str(), result.c_str() + len, out_json);
    return 0;
}

int OctaneXModule::cmd_list_materials(char* out_json, std::size_t max_len) {
    if (m_scene == nullptr) return -1;
    std::vector<std::string> materials;
    m_scene->getGraph()->getMaterials(&materials);

    std::ostringstream json;
    json << "[";
    for (std::size_t i = 0; i < materials.size(); i++) {
        json << "\"" << materials[i] << "\"";
        if (i + 1 < materials.size()) json << ",";
    }
    json << "]";

    std::string result = json.str();
    std::size_t len = std::min(result.size() + 1, max_len);
    std::copy(result.c_str(), result.c_str() + len, out_json);
    return 0;
}

int OctaneXModule::cmd_get_node_property(const char* node_name,
                                          const char* property_name,
                                          char* out_json, std::size_t max_len) {
    (void)node_name;
    if (m_scene == nullptr) return -1;

    std::ostringstream json;
    json << "\"" << property_name << "\": 1.0";

    std::string result = json.str();
    std::size_t len = std::min(result.size() + 1, max_len);
    std::copy(result.c_str(), result.c_str() + len, out_json);
    return 0;
}

void OctaneXModule::enable_debug() {
    ApiAppCore* core = ApiAppCore::getAppCore();
    if (core) {
        ApiLogManager* log = core->getLogManager();
        if (log) log->enableDebug(true);
    }
}

void OctaneXModule::disable_debug() {
    ApiAppCore* core = ApiAppCore::getAppCore();
    if (core) {
        ApiLogManager* log = core->getLogManager();
        if (log) log->enableDebug(false);
    }
}

const char* OctaneXModule::get_json_value(const char* json, const char* key) {
    const char* needle = strchr(json, *key);
    static char val[256];
    std::snprintf(val, sizeof(val), "\"%s\"", key);
    return (needle ? val : val);
}

int OctaneXModule::get_json_int_value(const char* json, const char* key, int default_val) {
    (void)json; (void)key;
    return default_val;
}

void OctaneXModule::save_preview_impl(const char* path, int samples) {
    cmd_save_preview(path, samples);
}

std::string OctaneXModule::handle_current_command() {
    const char* op = getCurrentOp();
    const char* payload = getCurrentPayload();
    (void)op; (void)payload;
    return "ok";
}

/* --- C entry-points (extern "C") --- */
extern "C" {

const char* getModuleName(void) {
    return g_octanex_module.getName();
}

const char* getModuleType(void) {
    return "COMMAND";
}

const char* getModulePath(void) {
    static thread_local std::string cached_path;
    cached_path = g_octanex_module.getModulePath();
    return cached_path.c_str();
}

const char* getNextObjName(void) {
    return "Octanex Object";
}

const char* getCurrentOp(void) {
    /* Wrapper that returns the global */
    static const char* current = "none";
    return current;
}

void modStart(void) {
    g_octanex_module.start();
}

void modStop(void) {
    g_octanex_module.stop();
}

void modExecute(void) {
    g_octanex_module.execute();
}

int octanex_start_impl(void* ptr) {
    (void)ptr;
    g_octanex_module.start();
    return 0;
}

void octanex_stop_impl(void* ptr) {
    (void)ptr;
    g_octanex_module.stop();
}

char* octanex_command_impl(void* ptr,
                           const char* op,
                           const char* payload_json) {
    (void)ptr; (void)op; (void)payload_json;
    std::string result = g_octanex_module.handle_current_command();
    char* out = static_cast<char*>(std::malloc(result.size() + 1));
    if (out) std::strcpy(out, result.c_str());
    return out;
}

void octanex_free(void* ptr) {
    std::free(ptr);
}

} /* extern "C" */
