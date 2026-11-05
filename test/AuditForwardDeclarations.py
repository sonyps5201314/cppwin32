import argparse
import collections
import json
from pathlib import Path
import re


ARCHITECTURES = {'_M_IX86': 1, '_M_AMD64': 2, '_M_ARM64': 4}


def architecture_mask(expression):
    if not re.fullmatch(r'[\s()|&!01]+', re.sub(r'defined\((_M_IX86|_M_AMD64|_M_ARM64)\)', '1', expression)):
        return None
    mask = 0
    for name, bit in ARCHITECTURES.items():
        value = re.sub(r'defined\((_M_IX86|_M_AMD64|_M_ARM64)\)', lambda m: str(int(m[1] == name)), expression)
        value = value.replace('&&', ' and ').replace('||', ' or ').replace('!', ' not ')
        if eval(value, {'__builtins__': {}}, {}):
            mask |= bit
    return mask


def audit(headers):
    duplicates, totals = [], collections.Counter()
    paths = sorted((headers / 'impl').glob('*.0.h'))
    for path in paths:
        seen = collections.defaultdict(list)
        conditions = []
        namespace, namespace_depth, depth = '', None, 0
        for number, line in enumerate(path.read_text(encoding='utf-8-sig').splitlines(), 1):
            text = line.strip()
            directive = re.match(r'#(if|ifdef|ifndef|elif|else|endif)\b\s*(.*)', text)
            if directive:
                operation, expression = directive.groups()
                if operation in ('if', 'ifdef', 'ifndef'):
                    if operation != 'if':
                        expression = ('!' if operation == 'ifndef' else '') + 'defined(' + expression + ')'
                    conditions.append({'branches': [expression], 'active': expression, 'guard': operation == 'ifndef' and expression.endswith('_H)') and len(conditions) == 0})
                elif operation in ('elif', 'else'):
                    current = conditions[-1]
                    opposite = '!(' + ' || '.join('(' + item + ')' for item in current['branches']) + ')'
                    current['active'] = opposite if operation == 'else' else '(' + opposite + ') && (' + expression + ')'
                    if operation == 'elif':
                        current['branches'].append(expression)
                else:
                    conditions.pop()
                continue
            match = re.match(r'(?:WIN32_EXPORT )?namespace ([\w:]+)', text)
            if match:
                namespace, namespace_depth = match[1], depth + 1
            declaration = re.fullmatch(r'(?:(?:struct|union) \w+|typedef [^{}]+|using \w+\s*=\s*[^{}]+);', text)
            if declaration:
                mask, other = 7, []
                for condition in conditions:
                    if condition['guard']:
                        continue
                    arches = architecture_mask(condition['active'])
                    if arches is None:
                        other.append(condition['active'])
                    else:
                        mask &= arches
                key = (namespace, re.sub(r'\s+', ' ', text), tuple(other))
                if mask:
                    for previous, previous_mask in seen[key]:
                        overlap = mask & previous_mask
                        if overlap:
                            duplicates.append({'file': path.name, 'line': number, 'previous_line': previous, 'declaration': key[1], 'namespace': namespace, 'architectures': [name for name, bit in ARCHITECTURES.items() if overlap & bit], 'conditions': other})
                    seen[key].append((number, mask))
                    totals['declarations'] += 1
            code = re.sub(r'"(?:[^"\\]|\\.)*"|//.*', '""', line)
            depth += code.count('{') - code.count('}')
            if namespace_depth is not None and depth < namespace_depth and not match:
                namespace, namespace_depth = '', None
        if conditions:
            raise ValueError('条件指令不完整：' + str(path))
    repeated_lines = {(item['file'], item['line']) for item in duplicates}
    return {'files': len(paths), **totals, 'duplicate_declarations': len(repeated_lines), 'duplicate_pairs': len(duplicates), 'duplicates': duplicates}


def main():
    parser = argparse.ArgumentParser(description='逐文件检查同一命名空间、条件及有效架构中的重复前置声明和 typedef。')
    parser.add_argument('--headers', required=True, type=Path)
    parser.add_argument('--output', required=True, type=Path)
    args = parser.parse_args()
    result = audit(args.headers)
    if not result['files']:
        raise ValueError('没有找到 .0.h 文件')
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, ensure_ascii=False, indent=2), encoding='utf-8')
    print(json.dumps({key: value for key, value in result.items() if key != 'duplicates'}, ensure_ascii=False))
    if result['duplicates']:
        raise SystemExit('存在重复前置声明或 typedef')


if __name__ == '__main__':
    main()
