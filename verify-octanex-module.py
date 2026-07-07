#!/usr/bin/env python3
"""Fresh verification for octanex-module/ changed files (current session)."""
import ast, tomllib, subprocess, pathlib, sys, json

ROOT = pathlib.Path('/Users/craig/octanex-module')

def check_py_ast():
    """Python AST for all .py files."""
    ok = True
    for py in sorted((ROOT / 'src').glob('**/*.py')):
        try:
            ast.parse(py.read_text(encoding='utf-8'))
            print(f'  ✓ {py.relative_to(ROOT)}')
        except SyntaxError as e:
            print(f'  ✗ {py.relative_to(ROOT)}: {e}')
            ok = False
    return ok

def check_pyproject():
    """pyproject.toml loads correctly — specifically the changed file."""
    toml_path = ROOT / 'pyproject.toml'
    src = toml_path.read_text(encoding='utf-8')
    try:
        d = tomllib.loads(src)
        assert d['project']['name'] == 'octanex-bridge'
        assert d['project']['version'] == '0.1.0'
        assert d['build-system']['requires']
        print(f'  ✓ pyproject.toml — {d["project"]["name"]} {d["project"]["version"]}')
        return True
    except Exception as e:
        print(f'  ✗ pyproject.toml: {e}')
        return False

def check_script_syntax():
    """All .sh scripts pass syntax (bash -n)."""
    ok = True
    for sh in sorted((ROOT / 'scripts').glob('*.sh')):
        r = subprocess.run(['bash', '-n', str(sh)], capture_output=True, text=True)
        if r.returncode == 0:
            print(f'  ✓ {sh.name}')
        else:
            print(f'  ✗ {sh.name}: {r.stderr.strip()}')
            ok = False
    return ok

def check_cmakelists():
    """CMakeLists.txt has all required sections."""
    cmake = (ROOT / 'CMakeLists.txt').read_text(encoding='utf-8')
    required = [
        ('minimal', 'cmake_minimum_required' in cmake),
        ('project(', 'project(' in cmake),
        ('C++17', 'CMAKE_CXX_STANDARD' in cmake),
        ('MODULE', 'MODULE' in cmake),
        ('dynamic_lookup', 'dynamic_lookup' in cmake),
        ('fPIC', '-fPIC' in cmake),
    ]
    ok = all(v for _, v in required)
    for name, pass_ in required:
        print(f'  {"✓" if pass_ else "✗"} CMakeLists.txt — {name}')
    return ok

def check_bridge_runtime():
    """bridge.py loads and all core methods work."""
    sys.path.insert(0, str(ROOT / 'src'))
    try:
        from octanex_mcp.bridge import ModuleBridge
        mb = ModuleBridge()
        assert mb.is_loaded is False
        assert mb.status.value == 'not_loaded'
        assert len(mb.available_paths()) == 2
        if mb._info is None:
            assert mb.is_available() is False  # should call _find_module without error
        else:
            assert mb.is_available() in (True, False)  # depends on module presence
        result = mb.execute_command('test_verify', {'x': 1})
        assert result is not None
        assert 'test_verify' in result['op']
        print(f'  ✓ bridge.py — {len(mb.available_paths())} paths, is_available={mb.is_available()}')
        # cleanup test file
        tf = ROOT / 'queue' / 'test_verify.json'
        if tf.exists():
            tf.unlink()
        return True
    except Exception as e:
        print(f'  ✗ bridge.py runtime: {e}')
        return False

def check_key_files():
    """All critical changed files exist."""
    files = [
        'CMakeLists.txt', 'pyproject.toml', 'README.md', 'docs/PLAN.md',
        'src/octanex_module.cpp', 'src/octanex.c', 'src/octanex_commands.h',
        'src/octanex_fallback.cpp', 'src/octanex.h', 'src/octanex_bridge.egg-info',
        'src/octanex_mcp/bridge.py', 'src/octanex_mcp/__init__.py',
        'scripts/build-release.sh', 'scripts/install-to-octane.sh',
        'scripts/start-octane-with-module.sh', 'test/test_commands.cpp',
        'test/test_module_loading.cpp', 'octanex_module_api/README.md',
        'src/octanex_mcp/README.md',
    ]
    ok = True
    for f in files:
        exists = (ROOT / f).exists()
        print(f'  {"✓" if exists else "✗"} {f}')
        if not exists:
            ok = False
            print(f'      MISSING: {f}')
    return ok

def main():
    errors = []
    print('=' * 55)
    print('AD-HOC VERIFICATION — octanex-module/ changed files')
    print('=' * 55)

    if not check_py_ast(): errors.append('AST')
    if not check_pyproject(): errors.append('pyproject.toml')
    if not check_script_syntax(): errors.append('sh')
    if not check_cmakelists(): errors.append('CMake')
    if not check_bridge_runtime(): errors.append('bridge.py')
    if not check_key_files(): errors.append('files')

    print(f'\n{"=" * 55}')
    if errors:
        print(f'RESULT: {len(errors)} issue(s): {", ".join(errors)}')
    else:
        print('RESULT: ALL CHECKS PASS')
    print(f'{"=" * 55}')
    return 1 if errors else 0

if __name__ == '__main__':
    sys.exit(main())
