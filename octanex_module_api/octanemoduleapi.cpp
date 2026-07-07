/**
 * octanemoduleapi.cpp - Stub/convenience wrappers for the Octane Modules API.
 *
 * Octane X resolves these symbols at runtime via dynamic lookup when the
 * module dylib is loaded by the Octane process. The actual symbol bodies
 * live in Octane's embedded SDK library, but we provide weak extern
 * placeholders here to satisfy the linker.
 */

#include "octanemoduleapi.h"

// Forward-declarations to satisfy the CMake build.
// At runtime Octane provides the real implementations.
extern "C" {
    void* octaneGetRenderEngine();
    void* octaneGetGraph();
    void  octaneRegisterCallback(int id);
    int   octaneGetCurrentScene();
    void* octaneFindNodeByName(const char* name);
    int   setCameraProperties(int node, const char* key, double value);
    int   setNodeProperty(int node, const char* key, double value);
    int   getSceneObjectCount();
    int   getSceneObjectName(int index, char* out, int outSize);
    int   getMaterialCount();
    int   getMaterialName(int index, char* out, int outSize);
    int   startRendering();
    void  stopRendering();
    int   hasRenderProgress();
    double getRenderProgress();
    int   savePreviewImage(const char* path);
    int   importGeometry(const char* path);
    int   createMaterial(const char* name);
    int   assignMaterial(int object, const char* material);
}
