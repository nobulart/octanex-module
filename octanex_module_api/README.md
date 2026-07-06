/*
 * C++ Octane API headers (vendored from OTOY)
 *
 * Directory layout for development:
 *   $OCTANEMODULE_DIR/
 *   ├── apimodule.h           ← module registration macros
 *   ├── api.h                 ← Vec3, Vec4, Matrix3x4, etc.
 *   ├── apiProjectManager.h   ← projects, scenes
 *   ├── apiRenderEngine.h     ← render control
 *   ├── apiMaterial.h         ← materials
 *   ├── apiScene.h            ← scene graph, geometry
 *   ├── apiGraph.h            ← node system
 *   ├── apiNode.h             ← OctaneNode
 *   └── octanemoduleapi.h/.cpp ← convenience single-include wrapper
 */

/*
 * BUILD INSTRUCTIONS:
 *
 *   1. Copy these headers into the octanex_module_api/ directory.
 *      They come from the OctaneRender SDK package shipped with Octane X.
 *
 *   2. On macOS:
 *      mkdir -p build && cd build
 *      cmake .. -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"
 *      make
 *      # Result: modules/octanex.dylib
 *
 *   3. Install to Octane X:
 *      cp modules/octanex.dylib /path/to/Octane\ X.app/Contents/Resources/modules/
 *
 *   4. Verify:
 *      /Applications/Octane\ X.app/Contents/MacOS/Octane\ X --module-log
 *      # Look for: "Octanex Module loaded (Command module, type COMMAND)"
 *
 * ABI NOTES:
 *   - All exports use extern "C"
 *   - All API calls come from the main message thread
 *   - Use OCTANE_API_ASSERT_MESSAGE_THREAD macro to verify
 */

/*
 * KEY OTOY API TYPES (used by this module):
 *
 * vec3_t, vec4_t, mat3x4_t   ← defined in api.h
 * ApiModule::ApiObject*       ← scene node handle
 * ApiProjectManager           ← project / scene manager
 * ApiRenderEngine             ← render control (start_render, restart, etc.)
 * ApiRenderEngine::Statistics ← getRenderResultStatistics()
 * ApiLogManager               ← module logging
 *
 * MODULE REGISTRATION:
 *
 *   // In start():
 *   APIMODULE_REGISTER_MODULE(MyModule, "Octanex Bridge", "octanex", "COMMAND");
 *
 *   // Or manually:
 *   ApiModule::registerModule(
 *       moduleInstance,
 *       "Octanex Bridge",           // display name
 *       "octanex",                  // module ID
 *       ApiModule::COMMAND          // module type
 *   );
 *
 * COMMAND EXECUTION:
 *
 *   // Command modules get:
 *   void execute(void);
 *
 *   // Call API from execute():
 *   projectManager->loadProject(path);
 *   renderEngine->startRendering();
 *
 * WORK PANE MODULES:
 *
 *   // For WorkPane (optional, Phase 4):
 *   class OctaneXWorkPane : public ApiWorkPaneModule {
 *   public:
 *       virtual const char* getName() const override;
 *       virtual void createWindow() override;
 *   };
 */

/*
 * MODULE ID ASSIGNMENT:
 *
 *   - Request "octanex" as module ID from OTOY support
 *   - During development, use 0xFFFFFFFF (auto-assign)
 *   - Each module type (COMMAND, WORK_PANE) gets same ID
 *   - Once assigned, cannot be reused
 */

/*
 * THREADING:
 *
 *   - All API calls → message thread (main thread)
 *   - Exception: ApiRenderEngine, ApiLogManager (documented per-class)
 *   - Verify: OCTANE_API_ASSERT_MESSAGE_THREAD macro
 */

/*
 * CRASH SAFETY:
 *
 *   - Modules are NOT sandboxed
 *   - A module crash → Octane crashes
 *   - Use try/catch in all public entry points
 *   - Enable moduleLoader log flag during development
 */
