import argparse
import collections
import difflib
import hashlib
import gzip
import json
import re
from pathlib import Path


def normalize(value):
    return re.sub(r"\s+", " ", value.strip())


def catalog(root):
    declarations = collections.defaultdict(dict)
    manifest = {}
    for path in sorted(root.rglob("*.h")):
        relative = path.relative_to(root).as_posix()
        partition = re.sub(r"\.[012]$", "", path.stem)
        data = path.read_bytes()
        manifest[relative] = hashlib.sha256(data).hexdigest()
        source = data.decode("utf-8-sig")
        depth, stack, pending = 0, [], None
        base_partition, base_partition_depth = None, None

        def add(key, value, line):
            declarations[key].setdefault(normalize(value), {"file": relative, "line": line})

        for number, line in enumerate(source.splitlines(), 1):
            if line.lstrip().startswith("#"):
                continue
            if path.name == 'base.h':
                namespace = re.match(r'\s*(?:WIN32_EXPORT\s+)?namespace win32::(Windows::Win32(?:::\w+)+)\s*$', line)
                if namespace:
                    base_partition = namespace[1].replace('::', '.')
                    base_partition_depth = depth + 1
            record = re.match(r"\s*(?:typedef\s+)?(struct|union|enum(?: class)?)\s+(?:__declspec\((?:[^()]|\([^()]*\))*\)\s+)*(\w+)(?:\s*[:{]|\s*$)", line)
            if record:
                pending = (record[1], record[2])
            owner = "::".join(item[1] for item in stack)
            callable = re.search(r"\b(__stdcall|__cdecl|__fastcall|__vectorcall)\s+(\w+)\s*\(.*", line)
            if callable and line.rstrip().endswith(";"):
                kind = "interface:" + owner if "virtual " in line else "api:" + partition
                add(kind + "::" + callable[2], line, number)
            callback = re.search(r"\(\s*__\w+\s*\*\s*(\w+)\s*\)", line)
            if callback and line.rstrip().endswith(";"):
                add(("callback:" if "typedef " in line else "field:" + owner + "::") + callback[1], line, number)
            alias = re.match(r"\s*(?:typedef\s+([^;{}()]+?)\s+(\w+)\s*((?:\[\d*\])*)|using\s+(\w+)\s*=\s*([^;]+));", line)
            if alias:
                add("alias:" + (alias[2] or alias[4]), (alias[1] + alias[3]) if alias[2] else alias[5], number)
            if stack and stack[-1][0] in ("struct", "union"):
                field = re.match(r"\s+([\w:*<> ,]+?)\s+(\w+)\s*((?:\[\d*\])*)\s*(?::\s*(\d+))?;\s*$", line)
                if field and not field[1].startswith(("typedef ", "return ", "static ", "using ")):
                    add("field:" + owner + "::" + field[2], field[1] + field[3] + (":" + field[4] if field[4] else ""), number)
            if stack and stack[-1][0].startswith("enum"):
                member = re.match(r"\s+(\w+)\s*=\s*(.*?)[,;]?\s*$", line)
                if member:
                    add("enumerator:" + owner + "::" + member[1], member[2].rstrip(","), number)
            constant = re.match(r"\s*(?:inline )?constexpr\s+(.+?)\s+(\w+)\s*=\s*(.*);\s*$", line)
            if constant:
                add("constant:" + (base_partition or partition) + "::" + constant[2], constant[1] + " = " + constant[3], number)
            code = re.sub(r'"(?:[^"\\]|\\.)*"|//.*', '""', line)
            opened, closed = code.count("{"), code.count("}")
            if pending and opened:
                stack.append((pending[0], pending[1], depth + 1))
                pending = None
            depth += opened - closed
            while stack and stack[-1][2] > depth:
                stack.pop()
            if base_partition_depth is not None and depth < base_partition_depth and '{' not in line and namespace is None:
                base_partition, base_partition_depth = None, None
    return {key: dict(sorted(values.items())) for key, values in sorted(declarations.items())}, manifest


def compare(reference, current):
    changes, tokens = [], collections.Counter()
    for key in sorted(reference.keys() | current.keys()):
        old, new = reference.get(key, {}), current.get(key, {})
        if old.keys() == new.keys():
            continue
        changes.append({"key": key, "before": old, "after": new})
        if len(old) == len(new) == 1:
            a, b = re.findall(r"\w+|[^\w\s]", next(iter(old))), re.findall(r"\w+|[^\w\s]", next(iter(new)))
            for operation, i, j, k, l in difflib.SequenceMatcher(None, a, b, autojunk=False).get_opcodes():
                if operation != "equal":
                    tokens[(" ".join(a[i:j]), " ".join(b[k:l]))] += 1
    return changes, [{"before": old, "after": new, "count": count} for (old, new), count in tokens.most_common()]


def main():
    parser = argparse.ArgumentParser(description="逐项比较公开声明的原文；不将 ABI 等价视为拼写相同。")
    parser.add_argument("--current", required=True, type=Path)
    parser.add_argument("--baseline", required=True, action="append", help="批次名=win32目录")
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--review", action="append", default=[], help="批次名=已审核的精确声明变化JSON或JSON.GZ")
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    current, hashes = catalog(args.current)
    (args.output / "current-catalog.json").write_text(json.dumps(current, ensure_ascii=False, indent=2), encoding="utf-8")
    (args.output / "current-hashes.json").write_text(json.dumps(hashes, indent=2), encoding="utf-8")
    summary = {}
    reviews = dict(entry.split("=", 1) for entry in args.review)
    failures = []
    for entry in args.baseline:
        name, path = entry.split("=", 1)
        baseline, manifest = catalog(Path(path))
        if not manifest:
            raise ValueError("基准没有头文件：" + path)
        changes, tokens = compare(baseline, current)
        report = {"path": path, "files": len(manifest), "declarations": len(baseline), "changes": changes, "tokens": tokens}
        (args.output / (name + ".json")).write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding="utf-8")
        summary[name] = {"files": len(manifest), "declarations": len(baseline), "changes": len(changes), "kinds": dict(collections.Counter(row["key"].split(":", 1)[0] for row in changes)), "top_tokens": tokens[:30]}
        if name in reviews:
            review = Path(reviews[name])
            reader = gzip.open if review.suffix == ".gz" else open
            with reader(review, "rt", encoding="utf-8") as stream:
                expected = json.load(stream)
            expected = sorted([{**row, "key": re.sub(r"^((?:api|constant):[^:]+)\.[012](::)", r"\1\2", row["key"])} for row in expected], key=lambda row: row["key"])
            actual = [{"key": row["key"], "before": sorted(row["before"]), "after": sorted(row["after"])} for row in changes]
            summary[name]["review_passed"] = actual == expected
            if actual != expected:
                failures.append(name)
    summary["current"] = {"files": len(hashes), "declarations": len(current)}
    (args.output / "summary.json").write_text(json.dumps(summary, ensure_ascii=False, indent=2), encoding="utf-8")
    print(json.dumps({key: {name: value for name, value in row.items() if name != "top_tokens"} for key, row in summary.items()}, ensure_ascii=False, indent=2))
    if failures:
        raise SystemExit("出现未审核的声明变化：" + ", ".join(failures))


if __name__ == "__main__":
    main()
