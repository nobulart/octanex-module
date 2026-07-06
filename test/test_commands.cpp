/*
 * C++ test commands.
 *
 * Tests all the core commands in the OctaneX module.
 */

#include <iostream>
#include <cstring>
#include "../octanex_module_api/octanemoduleapi.h"
#include "../src/octanex.h"
#include "../src/octanex_commands.h"

// Test import geometry
int test_import_geometry() {
    const char* path = "test/cube.obj";
    const char* name = "test_cube";
    
    int result = octanex_cmd_import_geometry(path, name);
    
    if (result == OCTANEX_SUCCESS) {
        std::cout << "✓ import_geometry: " << path << " (" << name << ")" << std::endl;
        return 0;
    } else {
        std::cerr << "✗ import_geometry failed" << std::endl;
        return -1;
    }
}

// Test create material
int test_create_material() {
    const char* name = "test_material";
    float r = 0.5f, g = 0.6f, b = 0.7f, a = 1.0f;
    float roughness = 0.3f, metallic = 0.5f;
    
    int result = octanex_cmd_create_material(name, r, g, b, a, roughness, metallic);
    
    if (result == OCTANEX_SUCCESS) {
        std::cout << "✓ create_material: " << name << " (roughness=" << roughness << ", metallic=" << metallic << ")" << std::endl;
        return 0;
    } else {
        std::cerr << "✗ create_material failed" << std::endl;
        return -1;
    }
}

// Test assign material
int test_assign_material() {
    const char* object_name = "test_cube";
    const char* material_name = "test_material";
    
    int result = octanex_cmd_assign_material(object_name, material_name);
    
    if (result == OCTANEX_SUCCESS) {
        std::cout << "✓ assign_material: " << object_name << " -> " << material_name << std::endl;
        return 0;
    } else {
        std::cerr << "✗ assign_material failed" << std::endl;
        return -1;
    }
}

// Test set camera
int test_set_camera() {
    float px = 2.0f, py = 3.0f, pz = 4.0f;
    float tx = 0.0f, ty = 0.0f, tz = 0.0f;
    float fov = 45.0f;
    
    int result = octanex_cmd_set_camera(px, py, pz, tx, ty, tz, fov);
    
    if (result == OCTANEX_SUCCESS) {
        std::cout << "✓ set_camera: pos=(" << px << "," << py << "," << pz << ") fov=" << fov << std::endl;
        return 0;
    } else {
        std::cerr << "✗ set_camera failed" << std::endl;
        return -1;
    }
}

// Test start render
int test_start_render() {
    int samples = 128;
    int width = 1024;
    int height = 1024;
    
    int result = octanex_cmd_start_render(samples, width, height);
    
    if (result == OCTANEX_SUCCESS) {
        std::cout << "✓ start_render: " << samples << " samples, " << width << "x" << height << std::endl;
        return 0;
    } else {
        std::cerr << "✗ start_render failed" << std::endl;
        return -1;
    }
}

// Test save preview
int test_save_preview() {
    const char* path = "test/preview.png";
    int samples = 128;
    
    int result = octanex_cmd_save_preview(path, samples);
    
    if (result == OCTANEX_SUCCESS) {
        std::cout << "✓ save_preview: " << path << " (" << samples << " samples)" << std::endl;
        return 0;
    } else {
        std::cerr << "✗ save_preview failed" << std::endl;
        return -1;
    }
}

// Test scene summary
int test_scene_summary() {
    char out_json[4096];
    memset(out_json, 0, sizeof(out_json));
    
    int result = octanex_cmd_scene_summary(out_json, sizeof(out_json));
    
    if (result == OCTANEX_SUCCESS) {
        std::cout << "✓ scene_summary: " << out_json << std::endl;
        return 0;
    } else {
        std::cerr << "✗ scene_summary failed" << std::endl;
        return -1;
    }
}

// Run all tests
int run_all_tests() {
    int failures = 0;
    
    std::cout << "=== OctaneX Command Tests ===" << std::endl;
    
    // Initialize the module
    int start_result = octanex_start();
    if (start_result != OCTANEX_SUCCESS) {
        std::cerr << "✗ Module init failed" << std::endl;
        return 1;
    }
    std::cout << "✓ Module initialized" << std::endl;
    
    // Run tests
    failures += (test_import_geometry() != 0) ? 1 : 0;
    failures += (test_create_material() != 0) ? 1 : 0;
    failures += (test_assign_material() != 0) ? 1 : 0;
    failures += (test_set_camera() != 0) ? 1 : 0;
    failures += (test_start_render() != 0) ? 1 : 0;
    failures += (test_save_preview() != 0) ? 1 : 0;
    failures += (test_scene_summary() != 0) ? 1 : 0;
    
    // Stop the module
    octanex_stop();
    
    std::cout << std::endl << "=== Test Results ===" << std::endl;
    std::cout << (failures == 0 ? "✓ All tests passed" : "✗ Some tests failed");
    std::cout << " (" << (8 - failures) << "/8 passed)" << std::endl;
    
    return failures;
}

int main() {
    return run_all_tests();
}
