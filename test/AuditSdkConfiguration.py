import argparse
import collections
import json
import re
from pathlib import Path


def main():
    parser = argparse.ArgumentParser(description='检查 SDK 配置头仅由 base.h 按指定绝对路径引入。')
    parser.add_argument('--headers', required=True, type=Path)
    parser.add_argument('--output', required=True, type=Path)
    args = parser.parse_args()
    template = Path(__file__).resolve().parents[1] / 'cppwin32' / 'base.h'
    base_matches_template = (args.headers / 'base.h').read_bytes() == template.read_bytes()
    standard = {'array', 'stdarg.h', 'stddef.h', 'stdint.h', 'string.h', 'string', 'string_view', 'type_traits'}
    sdk = {f'C:/Program Files (x86)/Windows Kits/10/Include/10.0.28000.0/shared/{name}' for name in ('sdkddkver.h', 'winapifamily.h')}
    external, unexpected, defaults, repeated, configuration = collections.Counter(), [], [], [], []
    for path in args.headers.rglob('*.h'):
        source = path.read_text(encoding='utf-8-sig')
        relative = path.relative_to(args.headers).as_posix()
        for match in re.finditer(r'#include\s*[<"]([^>"\r\n]+)[>"]', source):
            name = match[1].replace('\\', '/')
            if name.startswith('win32/'):
                continue
            external[name] += 1
            if name in sdk and relative == 'base.h':
                configuration.append(name)
            elif name not in standard:
                unexpected.append({'file': relative, 'include': name})
        if re.search(r'(?m)^#define\s+_WIN32_WINNT\s+', source):
            defaults.extend([relative] * len(re.findall(r'(?m)^#define\s+_WIN32_WINNT\s+', source)))
        if relative != 'base.h' and '#ifndef _WIN32_WINNT\n' in source:
            repeated.append(relative)
    result = {'base_matches_template': base_matches_template, 'external_headers': dict(external), 'sdk_configuration_headers': configuration, 'unexpected_headers': unexpected, 'local_default_version_locations': defaults, 'repeated_defaults': repeated}
    args.output.write_text(json.dumps(result, indent=2), encoding='utf-8')
    print(json.dumps(result, indent=2))
    if not base_matches_template or unexpected or set(configuration) != sdk or len(configuration) != 2 or defaults or repeated:
        raise SystemExit('生成头的 SDK 配置引用检查失败')


if __name__ == '__main__':
    main()
