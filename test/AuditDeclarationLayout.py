import argparse
import collections
import bisect
import itertools
import json
import re
from pathlib import Path


def mask(source):
    return re.sub(r'//[^\n]*|/\*[\s\S]*?\*/|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'', lambda match: re.sub(r'[^\n]', ' ', match[0]), source)


def record_aliases(source):
    code = mask(source)
    lines = source.splitlines(keepends=True)
    starts = list(itertools.accumulate([0] + [len(line) for line in lines]))
    records = collections.defaultdict(list)
    pattern = r'(?m)^    (?:__declspec\(align\(\d+\)\) )?(?:typedef )?(?:struct|union) (\w+)(?:\s*:[^{};]+)?\s*\{'
    for match in re.finditer(pattern, code):
        depth, end = 1, match.end()
        while depth:
            if code[end] == '{':
                depth += 1
            elif code[end] == '}':
                depth -= 1
            end += 1
        tail = code.find(';', end)
        aliases = set(re.findall(r'\b\w+\b', code[end:tail]))
        end = tail + 1
        line_index = bisect.bisect_right(starts, end) - 1
        following = itertools.chain([source[end:starts[line_index + 1]]], itertools.islice(lines, line_index + 1, None))
        for line in following:
            text = line.strip()
            if not text or text.startswith(('#', 'namespace ', 'WIN32_EXPORT namespace ')) or text in ('{', '}'):
                continue
            if re.fullmatch(r'(?:struct|union|enum class) [\w: ]+;', text):
                continue
            if text.startswith('using ') and text.endswith(';'):
                continue
            if text.startswith('typedef ') and text.endswith(';') and '{' not in text:
                alias = re.search(r'\(\s*(?:__\w+\s+)?[*&]\s*(\w+)\s*\)', text) or re.search(r'\b(\w+)\s*(?:\[[^]]*\]\s*)*;$', text)
                if alias:
                    aliases.add(alias[1])
                continue
            break
        records[match[1]].append({'line': bisect.bisect_right(starts, match.start()), 'aliases': aliases})
    return records


def main():
    parser = argparse.ArgumentParser(description='检查记录及其别名的位置、宏选择顺序和旧辅助头的移除。')
    parser.add_argument('--headers', required=True, type=Path)
    parser.add_argument('--ownership', required=True, type=Path)
    parser.add_argument('--output', required=True, type=Path)
    args = parser.parse_args()
    source = (args.headers / 'impl' / 'complex_structs.h').read_text(encoding='utf-8-sig')
    records = record_aliases(source)
    relations = sorted(set(tuple(line.split('\t')) for line in args.ownership.read_text().splitlines()))
    missing, placed = [], []
    for kind, namespace, owner, tag, alias in relations:
        locations = [record['line'] for record in records.get(tag, []) if alias in record['aliases']]
        entry = {'kind': kind, 'namespace': namespace, 'owner': owner, 'tag': tag, 'alias': alias, 'record_lines': locations}
        (placed if locations else missing).append(entry)
    obsolete = [str(path.relative_to(args.headers)) for path in args.headers.rglob('*.h') if path.name in ('native_declarations.h', 'native_aliases.h')]
    references, protections = [], []
    for path in args.headers.rglob('*.h'):
        text = path.read_text(encoding='utf-8-sig')
        if re.search(r'#include[^\n]*(?:native_declarations|native_aliases)\.h', text):
            references.append(str(path.relative_to(args.headers)))
        for match in re.finditer(r'#pragma (?:push_macro|pop_macro)', text):
            protections.append({'file': str(path.relative_to(args.headers)), 'line': text.count('\n', 0, match.start()) + 1})
    result = {'relationships': len(relations), 'placed': len(placed), 'missing': missing, 'obsolete_headers': obsolete, 'obsolete_includes': references, 'macro_protections': protections, 'placements': placed}
    args.output.write_text(json.dumps(result, ensure_ascii=False, indent=2), encoding='utf-8')
    print(json.dumps({key: value for key, value in result.items() if key != 'placements'}, ensure_ascii=False, indent=2))
    if missing or obsolete or references or protections:
        raise SystemExit('声明组织检查失败')


if __name__ == '__main__':
    main()
