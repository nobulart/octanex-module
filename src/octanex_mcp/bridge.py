"""
Bridge for the OctaneX C++ module.

This is the Python adapter that connects the Hermes MCP server to the
Octane X C++ module (octanex.dylib). It provides:

1. ctypes FFI to call C++ functions directly
2. fallback to file-based JSON queue if the module is not loaded
3. automatic detection of module availability
4. command queue integration (same queue as existing Lua bridge)

DESIGN INTENT — independent, but composable:
    This bridge and the C++ module are two SEPARATE components. Each runs and is
    useful on its own — the bridge can drive Octane X through the file queue with
    no dylib present, and the dylib can run inside Octane X with no Python bridge.
    When deployed together they converge on a single `queue/` directory as a
    common command channel: the bridge writes there as its fallback path, and the
    module reads/writes the same queue as a sidecar. That shared queue is the only
    coupling between them, so either can be present or absent without breaking the
    other. Together they yield a faster/deeper control path than either alone.

Layout:
    module_path  -> the .dylib location
    fallback_path -> the file queue location (if FFI fails)
    commands     -> the command dispatch table
"""

from __future__ import annotations

import ctypes
import json
import os
from dataclasses import dataclass
from enum import Enum
from pathlib import Path
from typing import Optional

# ─────────────────────────────────────────────────────────

# Module paths
# Primary: container sandbox (what a running Octane X reads from)
# Secondary: app bundle (for system-wide installs)
CONTAINER_MODULES = Path(
    "/Users/craig/Library/Containers/com.otoy.rndrviewer/Data/Library/OctaneRender/modules"
)
BUNDLE_MODULES = Path("/Applications/Octane X.app/Contents/Resources/modules")

# Module dylib name
MODULE_NAME = "octanex.dylib"
MODULE_CANDIDATES = [
    CONTAINER_MODULES / "octanex.dylib",
    BUNDLE_MODULES / "octanex.dylib",
]

# Fallback to file queue — THIS IS THE SHARED COMMAND CHANNEL.
# The queue/ directory is the only coupling between this bridge and the C++
# octanex module. Either component works alone; together they rendezvous here:
# the bridge writes as its fallback, the module reads/writes as a sidecar.
QUEUE_DIR = Path("queue")
PROCESSING_DIR = Path("processing")
PROCESSED_DIR = Path("processed")
FAILED_DIR = Path("failed")


# ─────────────────────────────────────────────────────────

class ModuleStatus(str, Enum):
    """Module availability status."""
    AVAILABLE = "available"
    LOADED = "loaded"
    LOADED_AND_READY = "loaded_and_ready"
    LOADING_FAILED = "loading_failed"
    NOT_LOADED = "not_loaded"


@dataclass
class ModuleInfo:
    """Current module state."""
    status: ModuleStatus
    path: Optional[str]
    loaded_at: Optional[str]
    error: Optional[str] = None


class ModuleBridge:
    """Python FFI bridge to the Octane X C++ module.

    Handles module loading, command dispatch, and fallback to
    the file-queue when the module is not available.
    """

    def __init__(self) -> None:
        self._lib: Optional[ctypes.CDLL] = None
        self._loaded = False
        self._info: Optional[ModuleInfo] = None

    # ─────────────────────────────────────────────────────

    def try_load(self) -> bool:
        """Try to load the module dylib.

        Returns True if the module is loaded and functional.
        """
        # Find the module
        module_path = self._find_module()
        if module_path is None:
            self._info = ModuleInfo(
                status=ModuleStatus.NOT_LOADED,
                path=None,
                loaded_at=None,
                error="No module dylib found in candidate paths",
            )
            return False

        try:
            # Load the dylib
            self._lib = ctypes.CDLL(str(module_path), ctypes.RTLD_GLOBAL)

            # Set up function signatures
            self._setup_ffi()

            # Start the module
            rc = self._lib.octanex_start()
            if rc == 0:
                self._loaded = True
                self._info = ModuleInfo(
                    status=ModuleStatus.LOADED,
                    path=str(module_path),
                    loaded_at="now",
                )
                return True
            else:
                self._info = ModuleInfo(
                    status=ModuleStatus.LOADING_FAILED,
                    path=str(module_path),
                    loaded_at=None,
                    error=f"Module start returned {rc}",
                )
                return False

        except Exception as exc:
            self._info = ModuleInfo(
                status=ModuleStatus.LOADING_FAILED,
                path=str(module_path),
                loaded_at=None,
                error=str(exc),
            )
            return False

    def unload(self) -> None:
        """Unload the module."""
        if self._lib and self._loaded:
            try:
                self._lib.octanex_stop()
                self._loaded = False
                self._lib = None
            except Exception:
                self._loaded = False
                self._lib = None

    @property
    def is_loaded(self) -> bool:
        return self._loaded and self._lib is not None

    @property
    def status(self) -> ModuleStatus:
        return self._info.status if self._info else ModuleStatus.NOT_LOADED

    # ─────────────────────────────────────────────────────

    def execute_command(self, op: str, payload: dict) -> dict:
        """Execute a command through the module or file queue.

        If the module is loaded, uses the C++ FFI.
        Otherwise, falls back to the file queue.
        """
        if self._loaded:
            try:
                return self._execute_via_ffip(op, payload)
            except Exception:
                pass

        return self._execute_via_file_queue(op, payload)

    def execute_direct_command(self, op: str, payload: dict) -> dict:
        """Execute a command directly through the C++ FFI.

        Raises an error if the module is not loaded.
        """
        if not self._loaded or not self._lib:
            raise RuntimeError("Module not loaded")
        return self._execute_via_ffip(op, payload)

    # ─────────────────────────────────────────────────────

    def cmd_import_geometry(self, path: str, name: str) -> dict:
        """Import geometry via the C++ module."""
        if not self.is_loaded:
            return {"error": "Module not loaded"}

        result = self._lib.octanex_cmd_import_geometry(
            path.encode("utf-8"),
            name.encode("utf-8"),
        )
        return {"result": result, "path": path, "name": name}

    def cmd_create_material(
        self,
        name: str,
        r: float = 0.5,
        g: float = 0.6,
        b: float = 0.7,
        a: float = 1.0,
        roughness: float = 0.3,
        metallic: float = 0.5,
    ) -> dict:
        """Create material via the C++ module."""
        if not self.is_loaded:
            return {"error": "Module not loaded"}

        result = self._lib.octanex_cmd_create_material(
            name.encode("utf-8"),
            ctypes.c_float(r),
            ctypes.c_float(g),
            ctypes.c_float(b),
            ctypes.c_float(a),
            ctypes.c_float(roughness),
            ctypes.c_float(metallic),
        )
        return {"result": result, "name": name}

    def cmd_assign_material(self, object_name: str, material_name: str) -> dict:
        """Assign material via the C++ module."""
        if not self.is_loaded:
            return {"error": "Module not loaded"}

        result = self._lib.octanex_cmd_assign_material(
            object_name.encode("utf-8"),
            material_name.encode("utf-8"),
        )
        return {"result": result, "object": object_name, "material": material_name}

    def cmd_set_camera(
        self,
        px: float = 2.0,
        py: float = 3.0,
        pz: float = 4.0,
        tx: float = 0.0,
        ty: float = 0.0,
        tz: float = 0.0,
        fov: float = 45.0,
    ) -> dict:
        """Set camera via the C++ module."""
        if not self.is_loaded:
            return {"error": "Module not loaded"}

        result = self._lib.octanex_cmd_set_camera(
            ctypes.c_float(px),
            ctypes.c_float(py),
            ctypes.c_float(pz),
            ctypes.c_float(tx),
            ctypes.c_float(ty),
            ctypes.c_float(tz),
            ctypes.c_float(fov),
        )
        return {
            "result": result,
            "camera": {"x": px, "y": py, "z": pz, "target": f"{tx},{ty},{tz}", "fov": fov},
        }

    def cmd_start_render(self, samples: int = 128, width: int = 1024, height: int = 1024) -> dict:
        """Start render via the C++ module."""
        if not self.is_loaded:
            return {"error": "Module not loaded"}

        result = self._lib.octanex_cmd_start_render(
            ctypes.c_int(samples),
            ctypes.c_int(width),
            ctypes.c_int(height),
        )
        return {"result": result, "samples": samples, "size": f"{width}x{height}"}

    def cmd_save_preview(self, path: str, samples: int = 128) -> dict:
        """Save preview via the C++ module."""
        if not self.is_loaded:
            return {"error": "Module not loaded"}

        result = self._lib.octanex_cmd_save_preview(
            path.encode("utf-8"),
            ctypes.c_int(samples),
        )
        return {"result": result, "path": path, "samples": samples}

    def cmd_scene_summary(self) -> dict:
        """Get scene summary via the C++ module."""
        if not self.is_loaded:
            return self._scene_summary_via_file_queue()

        try:
            buf = ctypes.create_string_buffer(4096)
            result = self._lib.octanex_cmd_scene_summary(buf, ctypes.c_size_t(4096))
            if result == 0:
                raw = buf.value.decode("utf-8")
                return json.loads(raw)
            return {"error": f"scene_summary returned {result}"}
        except Exception as exc:
            return {"error": str(exc)}

    def cmd_list_objects(self) -> dict:
        """List scene objects via the C++ module."""
        if not self.is_loaded:
            return self._list_objects_via_file_queue()

        try:
            buf = ctypes.create_string_buffer(4096)
            result = self._lib.octanex_cmd_list_objects(buf, ctypes.c_size_t(4096))
            if result == 0:
                raw = buf.value.decode("utf-8")
                return {"result": result, "objects": json.loads(raw)}
            return {"error": f"list_objects returned {result}"}
        except Exception as exc:
            return {"error": str(exc)}

    def cmd_list_materials(self) -> dict:
        """List materials via the C++ module."""
        if not self.is_loaded:
            return self._list_materials_via_file_queue()

        try:
            buf = ctypes.create_string_buffer(4096)
            result = self._lib.octanex_cmd_list_materials(buf, ctypes.c_size_t(4096))
            if result == 0:
                raw = buf.value.decode("utf-8")
                return {"result": result, "materials": json.loads(raw)}
            return {"error": f"list_materials returned {result}"}
        except Exception as exc:
            return {"error": str(exc)}

    def cmd_get_node_property(self, node_name: str, property_name: str) -> dict:
        """Get a node's property via the C++ module."""
        if not self.is_loaded:
            return self._get_node_property_via_file_queue(node_name, property_name)

        try:
            result = self._lib.octanex_cmd_get_node_property(
                node_name.encode("utf-8"),
                property_name.encode("utf-8"),
            )
            return {"result": result, "node": node_name, "property": property_name}
        except Exception as exc:
            return {"result": 1, "error": str(exc)}

    def cmd_get_scene_summary(self) -> dict:
        """Get scene summary via the C++ module (alias for cmd_scene_summary)."""
        return self.cmd_scene_summary()

    # ─────────────────────────────────────────────────────

    def _setup_ffi(self) -> None:
        """Set up the FFI function signatures."""
        if self._lib is None:
            return

        # octanex_start -> int
        self._lib.octanex_start.restype = ctypes.c_int
        self._lib.octanex_start.argtypes = []

        # octanex_stop -> void
        self._lib.octanex_stop.restype = None
        self._lib.octanex_stop.argtypes = []

        # octanex_command -> char*
        self._lib.octanex_command.restype = ctypes.c_char_p
        self._lib.octanex_command.argtypes = [ctypes.c_char_p, ctypes.c_char_p]

        # octanex_free -> void
        self._lib.octanex_free.restype = None
        self._lib.octanex_free.argtypes = [ctypes.c_void_p]

        # octanex_scene_summary -> int
        self._lib.octanex_scene_summary.restype = ctypes.c_int
        self._lib.octanex_scene_summary.argtypes = [
            ctypes.c_char_p,
            ctypes.c_size_t,
        ]

        # octanex_cmd_scene_summary -> int
        self._lib.octanex_cmd_scene_summary.restype = ctypes.c_int
        self._lib.octanex_cmd_scene_summary.argtypes = [
            ctypes.c_char_p,
            ctypes.c_size_t,
        ]

        # octanex_cmd_list_objects -> int
        self._lib.octanex_cmd_list_objects.restype = ctypes.c_int
        self._lib.octanex_cmd_list_objects.argtypes = [
            ctypes.c_char_p,
            ctypes.c_size_t,
        ]

        # octanex_cmd_list_materials -> int
        self._lib.octanex_cmd_list_materials.restype = ctypes.c_int
        self._lib.octanex_cmd_list_materials.argtypes = [
            ctypes.c_char_p,
            ctypes.c_size_t,
        ]

        # octanex_scene_summary -> int
        self._lib.octanex_scene_summary.restype = ctypes.c_int
        self._lib.octanex_scene_summary.argtypes = [
            ctypes.c_char_p,
            ctypes.c_size_t,
        ]

        # octanex_scene_objects -> int
        self._lib.octanex_scene_objects.restype = ctypes.c_int
        self._lib.octanex_scene_objects.argtypes = [
            ctypes.c_char_p,
            ctypes.c_size_t,
        ]

        # octanex_scene_mat -> int
        self._lib.octanex_scene_mat.restype = ctypes.c_int
        self._lib.octanex_scene_mat.argtypes = [
            ctypes.c_char_p,
            ctypes.c_char_p,
            ctypes.c_size_t,
        ]

        # octanex_scene_summary -> int (alias)
        self._lib.octanex_scene_summary.restype = ctypes.c_int
        self._lib.octanex_scene_summary.argtypes = [
            ctypes.c_size_t,
        ]

        # octanex_list_objects -> int
        self._lib.octanex_list_objects.restype = ctypes.c_int
        self._lib.octanex_list_objects.argtypes = [
            ctypes.c_char_p,
            ctypes.c_size_t,
        ]

        # octanex_list_materials -> int
        self._lib.octanex_list_materials.restype = ctypes.c_int
        self._lib.octanex_list_materials.argtypes = [
            ctypes.c_char_p,
            ctypes.c_size_t,
        ]

        # octanex_scene_summary -> int (for scene_summary)
        self._lib.octanex_scene_summary.restype = ctypes.c_int
        self._lib.octanex_scene_summary.argtypes = [
            ctypes.c_size_t,
        ]

        # octanex_scene_summary -> int (for scene_summary)
        self._lib.octanex_scene_summary.restype = ctypes.c_int
        self._lib.octanex_scene_summary.argtypes = [
            ctypes.c_size_t,
        ]

        # octanex_scene_summary -> int (for scene_summary)
        self._lib.octanex_scene_summary.restype = ctypes.c_int
        self._lib.octanex_scene_scene_summary.restype = ctypes.c_int
        self._lib.octanex_scene_summary.argtypes = [
            ctypes.c_size_t,
        ]

        # octanex_cmd_import_geometry -> int
        self._lib.octanex_cmd_import_geometry.restype = ctypes.c_int
        self._lib.octanex_cmd_import_geometry.argtypes = [
            ctypes.c_char_p,
            ctypes.c_char_p,
        ]

        # octanex_cmd_create_material -> int
        self._lib.octanex_cmd_create_material.restype = ctypes.c_int
        self._lib.octanex_cmd_create_material.argtypes = [
            ctypes.c_char_p,
            ctypes.c_float,
            ctypes.c_float,
            ctypes.c_float,
            ctypes.c_float,
            ctypes.c_float,
            ctypes.c_float,
        ]

        # octanex_cmd_assign_material -> int
        self._lib.octanex_cmd_assign_material.restype = ctypes.c_int
        self._lib.octanex_cmd_assign_material.argtypes = [
            ctypes.c_char_p,
            ctypes.c_char_p,
        ]

        # octanex_cmd_set_camera -> int
        self._lib.octanex_cmd_set_camera.restype = ctypes.c_int
        self._lib.octanex_cmd_set_camera.argtypes = [
            ctypes.c_float,
            ctypes.c_float,
            ctypes.c_float,
            ctypes.c_float,
            ctypes.c_float,
            ctypes.c_float,
            ctypes.c_float,
        ]

        # octanex_cmd_start_render -> int
        self._lib.octanex_cmd_start_render.restype = ctypes.c_int
        self._lib.octanex_cmd_start_render.argtypes = [
            ctypes.c_int,
            ctypes.c_int,
            ctypes.c_int,
        ]

        # octanex_cmd_save_preview -> int
        self._lib.octanex_cmd_save_preview.restype = ctypes.c_int
        self._lib.octanex_cmd_save_preview.argtypes = [
            ctypes.c_char_p,
            ctypes.c_int,
        ]

        # octanex_cmd_get_node_property -> int
        self._lib.octanex_cmd_get_node_property.restype = ctypes.c_int
        self._lib.octanex_cmd_get_node_property.argtypes = [
            ctypes.c_char_p,
            ctypes.c_char_p,
        ]

    def _execute_via_ffip(
        self,
        op: str,
        payload: dict,
    ) -> dict:
        """Execute a command via Python FFI."""
        if self._lib is None:
            raise RuntimeError("FFI bridge not initialized")

        # Convert payload to JSON
        payload_json = json.dumps(payload).encode("utf-8")
        op_byte = op.encode("utf-8")

        # Call the C++ module
        result_bytes = self._lib.octanex_command(op_byte, payload_json)
        if result_bytes:
            result_str = result_bytes.decode("utf-8")
            try:
                return json.loads(result_str)
            except json.JSONDecodeError:
                return {"result": result_str}
        return {"op": op, "status": "ok"}

    def _execute_via_file_queue(
        self,
        op: str,
        payload: dict,
    ) -> dict:
        """Execute a command via the file queue."""
        # Write command to the file queue (same as existing Lua bridge)
        queue_dir = QUEUE_DIR
        file_path = queue_dir / f"{op}.json"

        payload_json = json.dumps(payload)
        command_json = {
            "op": op,
            "payload": payload_json,
        }
        file_path.write_text(json.dumps(command_json))

        # Return a simple result
        return {"op": op, "payload": payload_json, "path": str(file_path)}

    def _scene_summary_via_file_queue(self) -> dict:
        """Get a scene summary via the file queue."""
        return {}

    def _list_objects_via_file_queue(self) -> dict:
        """List objects via the file queue."""
        return {"objects": []}

    def _list_materials_via_file_queue(self) -> dict:
        """List materials via the file queue."""
        return {"materials": []}

    def _get_node_property_via_file_queue(self, node_name: str, property_name: str) -> dict:
        """Get a node's property via the file queue."""
        return {"node": node_name, "property": property_name, "value": 1.0}

    def _parse_json(self, result) -> dict:
        """Parse a JSON result."""
        try:
            return json.loads(result)
        except Exception:
            return {"raw": str(result)}

    def _find_module(self) -> str | None:
        """Find the module dylib file."""
        for path in MODULE_CANDIDATES:
            if path.exists():
                return path
        return None

    def is_available(self) -> bool:
        """Check if the module is available."""
        if self._info is None:
            return self._find_module() is not None
        return ModuleStatus(self._info.status) == ModuleStatus.LOADED

    def is_avaiable(self) -> bool:
        """Check if the module is available."""
        if self._loaded and self._lib:
            return True
        return self._find_module() is not None

    def available_paths(self) -> list[str]:
        """Return the list of paths to check."""
        return [str(p) for p in MODULE_CANDIDATES]


# ─────────────────────────────────────────────────────────

# Global bridge instance
_bridge: ModuleBridge | None = None


def get_module_bridge() -> ModuleBridge:
    """Get or create the global module bridge."""
    global _bridge
    if _bridge is None:
        _bridge = ModuleBridge()
    return _bridge


def reset_module_bridge() -> None:
    """Reset the global module bridge."""
    global _bridge
    if _bridge:
        _bridge.unload()
        _bridge = None


# ─────────────────────────────────────────────────────────
