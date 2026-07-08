/*
 * octanemoduleapi.h - Forward declarations for the Octane Modules API.
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
#endif

/* --- OTOY API types --- */
#ifdef __cplusplus

/* Forward declarations used before full class definitions */
class ApiScene;
class ApiGraph;
class ApiMaterial;
class ApiLogManager;
class OctaneXModule;
class ApiModule;

class Vec3 {
public:
    float x{0.0f}, y{0.0f}, z{0.0f};
    Vec3() : x(0.0f), y(0.0f), z(0.0f) {}
    Vec3(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {}
};

class Vec4 {
public:
    float x{0.0f}, y{0.0f}, z{0.0f}, w{1.0f};
    Vec4() : x(0.0f), y(0.0f), z(0.0f), w(1.0f) {}
    Vec4(float _x, float _y, float _z, float _w) : x(_x), y(_y), z(_z), w(_w) {}
};

class ApiRenderResult {
public:
    enum RenderFormat { PNG8 = 0, EXR = 1 };
};

class ApiModule {
public:
    enum ModuleType { COMMAND = 0, WORK_PANE = 1, BOTH = 2 };
    virtual ~ApiModule();
    static void registerModule(void* info);
    static void registerModule(ApiModule* mod, const char* name,
                               const char* id, ModuleType type);
};

class ApiProjectManager {
public:
    virtual ~ApiProjectManager() = default;
    virtual void loadProject(const char* path);
    virtual void saveProject(const char* path);
    virtual void createProject(const char* name);
};

class ApiRenderEngine {
public:
    virtual ~ApiRenderEngine() = default;
    virtual void startRendering(int samples);
    virtual void stopRendering();
    virtual void restart();
    virtual void waitForRendering(int samples);
    virtual void setViewport(int width, int height);
    virtual ApiMaterial* saveImage(const char* path, int format);
    virtual Vec3 getCameraPosition();
    virtual void setCameraPosition(const Vec3& pos);
    virtual Vec3 getCameraTarget();
    virtual void setCameraTarget(const Vec3& target);
    virtual float getCameraFov();
    virtual void setCameraFov(float fov);
    virtual ApiRenderResult* getRenderResult();
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

class ApiGraph {
public:
    virtual ~ApiGraph() = default;
    virtual void getObjects(std::vector<std::string>* out) const;
    virtual void getMaterials(std::vector<std::string>* out) const;
    virtual int loadFile(const char* path);
    virtual int assignMaterial(const char* object_name, const char* material_name);
    virtual ApiGraph* getNodeByName(const char* name);
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

class ApiWorkPaneModule {
public:
    virtual ~ApiWorkPaneModule() = default;
    virtual const char* getWorkPaneName();
    virtual void createWindow();
};

class OctaneCommandModule {
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
    virtual const char* getModuleDirectory() const;
};

/*
 * Module info struct - used by Octane's module loader.
 * Members are compatible with both C and C++.
 */
struct OctaneModuleInfo {
    const char* name;
    const char* display_name;
    const char* module_id;
    int type;
    void (*start)(void);
    void (*stop)(void);
    void (*execute)(void);
};

/*
 * Runtime functions - resolved by Octane at load time.
 */
#ifdef __cplusplus
extern "C" {
#endif
const char* getModuleDirectory();
const char* getNextObjName();
const char* getCurrentOp();
extern OctaneModuleInfo getModuleInfo(void);
#ifdef __cplusplus
} /* extern "C" */
#endif

#else /* !__cplusplus -- C-only mode */

/* Minimal C definitions */
typedef struct OctaneModuleInfo {
    const char* name;
    const char* display_name;
    const char* module_id;
    int type;
    void (*start)(void);
    void (*stop)(void);
    void (*execute)(void);
} OctaneModuleInfo;

typedef void* ApiModuleInfo;
typedef void* ApiLogManager;
typedef void* ApiProjectManager;
typedef void* ApiRenderEngine;
typedef void* ApiScene;
typedef void* ApiGraph;
typedef void* ApiMaterial;
typedef void* ApiWorkPaneModule;

typedef struct { float x, y, z; } Vec3;

typedef struct {
    void* (*getAppCore)(void);
    void* (*getProjectManager)(void);
    void* (*getRenderEngine)(void);
} ApiAppCore;

#endif /* __cplusplus */

#endif /* OCTANE_MODULE_API_H */
