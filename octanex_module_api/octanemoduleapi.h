/*
 * octanemoduleapi.h — Forward declarations for the Octane Modules API.
 *
 * These symbols are resolved at runtime by Octane X.
 * This header is only needed for compile-time type information.
 *
 * Designed to be C-compatible: C++ features are guarded with __cplusplus.
 * This header can be included by both C and C++ translation units.
 */

#ifndef OCTANE_MODULE_API_H
#define OCTANE_MODULE_API_H

#ifdef __cplusplus
#include <vector>
#include <string>
#include <stddef.h>
#else
#include <stddef.h>
/* C-compatible forward declarations */
typedef struct Vec3 Vec3;
typedef struct Vec4 Vec4;
typedef struct ApiModule ApiModule;
typedef struct OctaneCommandModule OctaneCommandModule;
typedef struct ApiWorkPaneModule ApiWorkPaneModule;
typedef struct ApiAppCore ApiAppCore;
typedef struct ApiProjectManager ApiProjectManager;
typedef struct ApiRenderEngine ApiRenderEngine;
typedef struct ApiScene ApiScene;
typedef struct ApiRenderResult ApiRenderResult;
typedef struct ApiGraph ApiGraph;
typedef struct OctaneNode OctaneNode;
typedef struct ApiMaterial ApiMaterial;
typedef struct ApiLogManager ApiLogManager;
typedef struct OctaneXModule OctaneXModule;
#endif

/* --- OTOY API types --- */
#ifdef __cplusplus

class Vec3 {
public:
    float x{0.0f}, y{0.0f}, z{0.0f};
    Vec3() : x(0.0f), y(0.0f), z(0.0f) {}
    Vec3(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {}
};

class Vec4 {
public:
    float x{0.0f}, y{0.0f}, z{0.0f}, w{1.0f};
};

class ApiModule {
public:
    enum ModuleType { COMMAND = 0, WORK_PANE = 1, BOTH = 2 };
    virtual ~ApiModule();
    static void registerModule(void* info);
    static void registerModule(ApiModule* mod, const char* name,
                               const char* id, ModuleType type);
};

class OctaneCommandModule : public ApiModule {
public:
    virtual void execute() = 0;
    virtual const char* getName() const = 0;
    virtual const char* getModuleName() const = 0;
    virtual void start();
    virtual void stop();
protected:
    const char* getCurrentOp() const;
    const char* getCurrentPayload() const;
    void flush_pending_commands() const;
    void start_internal_thread();
    void stop_internal_thread();
};

class ApiWorkPaneModule {
public:
    virtual ~ApiWorkPaneModule() = default;
    virtual const char* getWorkPaneName() const;
    virtual void createWindow();
};

/* OctaneXModule forward declaration — definition in octanex_module.cpp */
class OctaneXModule;

#if !defined(__OCTANE_MODULE_INFO_DECLARED)
#define __OCTANE_MODULE_INFO_DECLARED
struct OctaneModuleInfo {
    const char* name;
    const char* display_name;
    const char* module_id;
    const char* type;
};
#endif

class ApiProjectManager {
public:
    virtual ~ApiProjectManager() = default;
    virtual void loadProject(const char* path);
    virtual void saveProject(const char* path);
    virtual void createProject(const char* name);
};

/* Forward declarations needed before ApiScene */
class ApiGraph;
class ApiMaterial;

class ApiRenderEngine {
public:
    virtual ~ApiRenderEngine() = default;
    virtual void startRendering(int samples);
    virtual void stopRendering();
    virtual void restart();
    virtual void waitForRendering(int samples);
    virtual void setViewport(int width, int height);
    virtual void saveImage(const char* path, int format);
    virtual Vec3 getCameraPosition();
    virtual void setCameraPosition(const Vec3& pos);
    virtual Vec3 getCameraTarget();
    virtual void setCameraTarget(const Vec3& target);
    virtual float getCameraFov();
    virtual void setCameraFov(float fov);
    class ApiRenderResult* getRenderResult();
};

class ApiScene {
public:
    virtual ~ApiScene() = default;
    virtual ApiGraph* getGraph();
    virtual void setCameraPosition(const Vec3& pos);
    virtual void setCameraTarget(const Vec3& target);
    virtual float getCameraFov();
    virtual void setCameraFov(float fov);
    virtual Vec3 getCameraPosition();
    virtual Vec3 getCameraTarget();
    virtual ApiMaterial* createMaterial(const char* name);
    virtual int getCurrentOp() const;
};

class ApiRenderResult {
public:
    enum RenderFormat { PNG8 = 0, EXR = 1 };
};

class ApiGraph {
public:
    virtual ~ApiGraph() = default;
    virtual void getObjects(std::vector<std::string>* out) const;
    virtual void getMaterials(std::vector<std::string>* out) const;
    virtual int loadFile(const char* path);
    virtual int assignMaterial(const char* object_name, const char* material_name);
    class OctaneNode* getNodeByName(const char* name);
};

class OctaneNode {
public:
    virtual ~OctaneNode() = default;
    virtual void setProperty(const char* name, float value);
    virtual float getProperty(const char* name) const;
};

class ApiMaterial {
public:
    virtual ~ApiMaterial() = default;
    virtual void setColor(float r, float g, float b, float a);
    virtual void setRoughness(float roughness);
    virtual void setMetallic(float metallic);
};

class ApiLogManager {
public:
    virtual ~ApiLogManager() = default;
    virtual void logMessage(const char* level, const char* msg, const char* module_id);
    virtual void enableDebug(bool on);
};

class ApiAppCore {
public:
    static ApiAppCore* getAppCore();
    virtual ~ApiAppCore();
    virtual void* getProjectManager();
    virtual void* getRenderEngine();
    virtual void* getScene();
    ApiLogManager* getCurrentOp();
    virtual ApiLogManager* getLogManager();
};

/* OctaneXModule forward declaration — definition in octanex_module.cpp */
class OctaneXModule;

#endif /* __cplusplus */

/*
 * Runtime functions — resolved by Octane at load time.
 */
#ifdef __cplusplus
extern "C" {
#endif

const char* getModuleDirectory();
const char* getNextObjName();
const char* getCurrentOp();

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* OCTANE_MODULE_API_H */
