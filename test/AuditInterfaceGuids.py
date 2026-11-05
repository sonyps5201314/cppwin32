import argparse
import collections
import gzip
import json
from pathlib import Path
import re

from AuditForwardDeclarations import architecture_mask


def qualified(scope, name):
    return (scope + '::' if scope else '::') + name


def guid_value(initializer):
    values = [int(value, 16) for value in re.findall(r'0x([0-9a-fA-F]+)', initializer)]
    if len(values) != 11:
        raise ValueError('GUID 初始化值不完整：' + initializer)
    return values


def uuid_string(values):
    return f'{values[0]:08X}-{values[1]:04X}-{values[2]:04X}-{values[3]:02X}{values[4]:02X}-' + ''.join(f'{value:02X}' for value in values[5:])


def lines(path):
    namespace, namespace_depth, depth, mask, stack = '', None, 0, 7, []
    for number, line in enumerate(path.read_text(encoding='utf-8-sig').splitlines(), 1):
        text = line.strip()
        directive = re.match(r'#(if|ifdef|ifndef|elif|else|endif)\b\s*(.*)', text)
        if directive:
            operation, expression = directive.groups()
            if operation in ('if', 'ifdef', 'ifndef'):
                arches = architecture_mask(expression) if operation == 'if' else None
                stack.append((mask, arches))
                if arches is not None:
                    mask &= arches
            elif operation == 'else':
                parent, arches = stack[-1]
                mask = parent if arches is None else parent & ~arches
            elif operation == 'elif':
                raise ValueError('GUID 声明检查不支持该条件：' + str(path))
            else:
                mask = stack.pop()[0]
            continue
        match = re.match(r'(?:WIN32_EXPORT )?namespace ([\w:]+)', text)
        if match:
            namespace, namespace_depth = match[1], depth + 1
        yield namespace, number, text, mask
        code = re.sub(r'"(?:[^"\\]|\\.)*"|//.*', '""', line)
        depth += code.count('{') - code.count('}')
        if namespace_depth is not None and depth < namespace_depth and not match:
            namespace, namespace_depth = '', None


def main():
    parser = argparse.ArgumentParser(description='逐项核对旧接口 GUID 与公开 IID、前置声明 UUID、完整接口 UUID，并生成原生编译检查。')
    parser.add_argument('--baseline', required=True, type=Path)
    parser.add_argument('--headers', required=True, type=Path)
    parser.add_argument('--output', required=True, type=Path)
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    expected = {}
    for path in sorted((args.baseline / 'impl').glob('*.0.h')):
        for match in re.finditer(r'template <> inline constexpr guid guid_v<([^>]+)>\{(.*?)\};', path.read_text(encoding='utf-8-sig')):
            name = match[1]
            name = name if name.startswith('win32::') else 'win32::' + name if '::' in name else '::' + name
            values = guid_value(match[2])
            if name in expected:
                raise ValueError('基准存在重复 GUID：' + name)
            expected[name] = {'file': path.name, 'values': values, 'uuid': uuid_string(values), 'initializer': match[2].strip()}
    if not expected:
        raise ValueError('没有找到旧 guid_v 接口特化')
    with gzip.open(args.output / 'expected-guids.json.gz', 'wt', encoding='utf-8') as stream:
        json.dump(expected, stream, ensure_ascii=False, indent=2)

    constants, forwards, definitions = {}, collections.defaultdict(list), collections.defaultdict(list)
    errors = []
    for path in sorted((args.headers / 'impl').glob('*.0.h')):
        if 'guid_v<' in path.read_text(encoding='utf-8-sig'):
            errors.append('仍生成 guid_v 特化：' + path.name)
        for scope, number, text, mask in lines(path):
            constant = re.fullmatch(r'inline constexpr IID IID_(\w+) = \{(.*?)\};', text)
            if constant:
                name = qualified(scope, constant[1])
                if name in constants:
                    errors.append('重复 IID：' + name)
                constants[name] = {'file': path.name, 'line': number, 'values': guid_value(constant[2]), 'mask': mask}
            forward = re.fullmatch(r'struct __declspec\(uuid\("([\w-]+)"\)\) (\w+);', text)
            if forward:
                forwards[qualified(scope, forward[2])].append({'file': path.name, 'line': number, 'uuid': forward[1], 'mask': mask})
    for scope, number, text, mask in lines(args.headers / 'impl' / 'complex_interfaces.h'):
        definition = re.match(r'struct __declspec\(uuid\("([\w-]+)"\)\) __declspec\(novtable\) (\w+)\b', text)
        if definition:
            definitions[qualified(scope, definition[2])].append({'line': number, 'uuid': definition[1], 'mask': mask})

    for name, expected_entry in expected.items():
        constant = constants.get(name)
        if not constant or constant['file'] != expected_entry['file'] or constant['values'] != expected_entry['values']:
            errors.append('IID 名称、位置或值不符：' + name)
        forward = [item for item in forwards[name] if item['file'] == expected_entry['file']]
        if not forward or any(item['uuid'] != expected_entry['uuid'] for item in forwards[name]):
            errors.append('前置声明 UUID 不符：' + name)
        if not definitions[name] or any(item['uuid'] != expected_entry['uuid'] for item in definitions[name]):
            errors.append('完整接口 UUID 不符：' + name)
    if constants.keys() != expected.keys():
        errors.append('IID 集合与原有接口 GUID 集合不符')
    if definitions.keys() != expected.keys():
        errors.append('接口 UUID 集合与原有接口 GUID 集合不符')
    report = {'expected_guids': len(expected), 'iid_constants': len(constants), 'uuid_interface_definitions': len(definitions), 'uuid_forward_types': len(forwards), 'errors': errors}
    (args.output / 'audit.json').write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
    print(json.dumps({**report, 'errors': errors[:20], 'error_count': len(errors)}, ensure_ascii=False, indent=2))
    if errors:
        raise SystemExit('接口 GUID 检查失败')

    includes = ['#pragma once'] + ['#include "win32/impl/' + path.name + '"' for path in sorted((args.headers / 'impl').glob('*.0.h'))]
    (args.output / 'InterfaceDeclarations.h').write_text('\n'.join(includes) + '\n', encoding='utf-8')
    checks = ['''#pragma once
template<typename Left, typename Right>
constexpr bool InterfaceGuidMatches(Left const& left, Right const& right)
{
    if (left.Data1 != right.Data1 || left.Data2 != right.Data2 || left.Data3 != right.Data3) return false;
    for (int index = 0; index < 8; ++index) if (left.Data4[index] != right.Data4[index]) return false;
    return true;
}
''']
    for name, entry in sorted(expected.items()):
        scope, leaf = name.rsplit('::', 1)
        iid = qualified(scope, 'IID_' + leaf)
        mask = constants[name]['mask']
        if mask != 7:
            checks.append('#if ' + ' || '.join('defined(' + macro + ')' for bit, macro in ((1, '_M_IX86'), (2, '_M_AMD64'), (4, '_M_ARM64')) if bit & mask))
        checks.append(f'static_assert(InterfaceGuidMatches({iid}, IID{{ {entry["initializer"]} }}) && InterfaceGuidMatches(__uuidof({name}), {iid}) && InterfaceGuidMatches(win32::guid_of<{name}>(), {iid}));')
        if mask != 7:
            checks.append('#endif')
    (args.output / 'InterfaceGuidChecks.h').write_text('\n'.join(checks) + '\n', encoding='utf-8')


if __name__ == '__main__':
    main()
