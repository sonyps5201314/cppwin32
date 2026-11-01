import argparse
import collections
import json
import re
from pathlib import Path


def main():
    parser = argparse.ArgumentParser(description='检查生成头的外部依赖及唯一默认 Windows 版本定义。')
    parser.add_argument('--headers', required=True, type=Path)
    parser.add_argument('--output', required=True, type=Path)
    args = parser.parse_args()
    standard = {'array', 'stdarg.h', 'stddef.h', 'stdint.h', 'string.h', 'string', 'string_view', 'type_traits'}
    external, unexpected, defaults, repeated = collections.Counter(), [], [], []
    for path in args.headers.rglob('*.h'):
        source = path.read_text(encoding='utf-8-sig')
        relative = path.relative_to(args.headers).as_posix()
        for match in re.finditer(r'#include\s*[<"]([^>"\r\n]+)[>"]', source):
            name = match[1].replace('\\', '/')
            if name.startswith('win32/'):
                continue
            external[name] += 1
            if name not in standard:
                unexpected.append({'file': relative, 'include': name})
        if re.search(r'(?m)^#define\s+_WIN32_WINNT\s+', source):
            defaults.extend([relative] * len(re.findall(r'(?m)^#define\s+_WIN32_WINNT\s+', source)))
        if relative != 'base.h' and '#ifndef _WIN32_WINNT\n' in source:
            repeated.append(relative)
    result = {'external_headers': dict(external), 'unexpected_headers': unexpected, 'default_version_locations': defaults, 'repeated_defaults': repeated}
    args.output.write_text(json.dumps(result, indent=2), encoding='utf-8')
    print(json.dumps(result, indent=2))
    if unexpected or defaults != ['base.h'] or repeated:
        raise SystemExit('生成头的 SDK 独立性检查失败')


if __name__ == '__main__':
    main()
