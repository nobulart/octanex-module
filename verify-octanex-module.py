#!/usr/bin/env python3
"""Verifies all changed files in octanex-module/."""
import ast, json, pathlib, tomllib, subprocess, sys

ROOT = pathlib.Path(__file__).parent

def main():
    errors = []
    # 1. Python AST
    for py in sorted((ROOT / 'src').glob('**/*.py')):
        try:
            ast.parse(py.read_text())
            print(f'PASS {py.relative_to(ROOT)}')
        except SyntaxError as e:
            errors.append(f'FAIL {py.relative_to(ROOT)}')
            print(f'FAIL {py.relative_to(ROOT)}')

    # 2. pyproject.toml
    toml = (ROOT / 'pyproject.toml').read_text()
    try:
        d = tomllib.loads(toml)
        assert d['project']['name']
        print('PASS pyproject.toml')
    except Exception as e:
        errors.append(f'FAIL pyproject.toml')
        print(f'FAIL pyproject.toml')

    # 3. Shell scripts
    for sh in sorted((ROOT / 'scripts').glob('*.sh')):
        r = subprocess.run(['bash', '-n', str(sh)], capture_output=True, text=True)
        if r.returncode == 0:
            print(f'PASS {sh.name}')
        else:
            errors.append(f'FAIL {sh.name}')
            print(f'FAIL {sh.name}')

    # 4. CMakeLists.txt
    cmake = (ROOT / 'CMakeLists.txt').read_text()
    checks = {'min_req': 'cmake_minimum_required(' in cmake, 'project': 'project(' in cmake,
              'c++': 'CMAKE_CXX_STANDARD' in cmake, 'MODULE': 'MODULE' in cmake,
              'dynamic_lookup': 'dynamic_lookup' in cmake, 'fPIC': '-fPIC' in cmake}
    ok = all(checks.values())
    print(f'PASS CMakeLists.txt' if ok else f'WARN CMakeLists.txt')
    if not ok:
        errors.append(f'WARN CMakeLists.txt')

    # 5. Runtime
    sys.path.insert(0, str(ROOT / 'src'))
    from octanex_mcp.bridge import ModuleBridge
    mb = ModuleBridge()
    assert mb.is_loaded is False
    assert mb.status.value == 'not_loaded'
    assert mb.execute_command('test_x', {'a': 1})
    print('PASS bridge.py runtime')

    # 6. Files exist
    for f in ['CMakeLists.txt', 'pyproject.toml', 'README.md', 'docs/PLAN.md',
               'src/octanex_module.cpp', 'src/octanex.c', 'src/octanex_commands.h',
               'src/octanex_fallback.cpp', 'src/octanex.h',
               'src/octanex_mcp/bridge.py', 'src/octanex_mcp/__init__.py',
               'scripts/build-release.sh', 'scripts/install-to-octane.sh',
               'scripts/start-octane-with-module.sh',
               'test/test_commands.cpp', 'test/test_module_loading.cpp']:
        if (ROOT / f).exists():
            print(f'PASS {f}')
        else:
            errors.append(f'FAIL {f}')
            print(f'FAIL {f}')

    print(f'\nResult: {len(errors)} issues')
    for e in errors:
        print(f'  {e}')
    return 1 if errors else 0

if __name__ == '__main__':
    sys.exit(main())
