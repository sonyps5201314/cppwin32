import argparse
import concurrent.futures
import gzip
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import sys
import time

if hasattr(sys.stdout, 'reconfigure'):
    sys.stdout.reconfigure(encoding='utf-8')
    sys.stderr.reconfigure(encoding='utf-8')

HERE = Path(__file__).resolve().parent
TARGETS = {'x86': 'i686-pc-windows-msvc', 'x64': 'x86_64-pc-windows-msvc', 'arm64': 'aarch64-pc-windows-msvc'}


def load(path):
    reader = gzip.open if str(path).endswith('.gz') else open
    with reader(path, 'rt', encoding='utf-8-sig') as stream:
        return json.load(stream)


def dump(path, value):
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(value, ensure_ascii=False, indent=2), encoding='utf-8')


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def rsp(path):
    result, option = {}, None
    if not path.exists():
        return result
    for line in path.read_text(encoding='utf-8-sig').splitlines():
        text = line.strip()
        if text.startswith('--'):
            option = text
        elif text and not text.startswith('#') and option:
            result.setdefault(option, []).append(text)
    return result


def discover_environment(metadata):
    sdk = metadata / 'generation/WinSDK'
    native = metadata / 'bin/Release/net10.0/win-x64'
    msvc_candidates = [*Path('C:/Program Files (x86)/Microsoft Visual Studio/2019/Enterprise/VC/Tools/MSVC').glob('*'), *Path('C:/Program Files/Microsoft Visual Studio/18/Enterprise/VC/Tools/MSVC').glob('*')]
    msvc = next((path for path in sorted(msvc_candidates) if (path / 'include/vector').exists()), None)
    installed = Path('C:/Program Files (x86)/Windows Kits/10/Include/10.0.28000.0')
    resources = [*Path('C:/Program Files/Microsoft Visual Studio/18/Enterprise/VC/Tools/Llvm/lib/clang').glob('*/include'), *Path('C:/Program Files/LLVM/lib/clang').glob('*/include')]
    return sdk, native, msvc, installed, sorted(resources)[-1] if resources else None


def arguments(metadata, arch, character_set, projection=False):
    sdk, native, msvc, installed, resource = discover_environment(metadata)
    result = ['-x', 'c++', '-std=c++17', '--target=' + TARGETS[arch], '-fms-extensions', '-fms-compatibility', '-fms-compatibility-version=19.29', '-ferror-limit=0', '-Wno-everything']
    if resource:
        result += ['-isystem', str(resource)]
    if msvc:
        result += ['-isystem', str(msvc / 'include')]
    if not projection:
        result += ['-I' + str(sdk / 'inc')]
        result += ['-I' + str(sdk / 'RecompiledIdlHeaders' / name) for name in ('shared', 'um', 'ucrt', 'winrt')]
        result += rsp(metadata / 'sources/GeneratorSdk/tools/assets/scraper/baseSettings.rsp').get('--additional', [])
        result += rsp(sdk / 'scraper.settings.rsp').get('--additional', [])
    for folder in ('ucrt', 'shared', 'um', 'winrt'):
        if (installed / folder).exists():
            result += ['-isystem', str(installed / folder)]
    if arch == 'x86':
        result += ['-D_X86_=1']
    elif arch == 'x64':
        result += ['-D_AMD64_=1']
    else:
        result += ['-D_ARM64_=1']
    result += ['-DUNICODE=1', '-D_UNICODE=1'] if character_set == 'Unicode' else ['-UUNICODE', '-U_UNICODE']
    return result


def manifest(paths):
    return {str(path.resolve()): digest(path) for path in sorted(set(paths)) if path.is_file()}


class Runner:
    def __init__(self, args):
        self.args = args
        self.sdk, self.native, _, _, _ = discover_environment(args.metadata)
        self.worker = args.output / 'bin/Extractor.exe'
        if not self.worker.exists():
            raise FileNotFoundError('请先用 devenv.com 构建 AST 提取器：' + str(self.worker))
        inputs = [*args.headers.rglob('*.h'), *self.sdk.joinpath('RecompiledIdlHeaders').rglob('*.h'), *self.sdk.joinpath('inc').rglob('*'), *self.sdk.joinpath('Partitions').rglob('*'), self.sdk / 'enums.json', self.native / 'libclang.dll', self.native / 'libClangSharp.dll', self.native / 'ClangSharp.Interop.dll', self.worker, self.worker.with_suffix('.dll')]
        self.input_manifest = manifest(inputs)
        self.fingerprint = hashlib.sha256(json.dumps(self.input_manifest, sort_keys=True).encode()).hexdigest()
        history = args.output / 'manifests' / (self.fingerprint + '.json.gz')
        history.parent.mkdir(parents=True, exist_ok=True)
        if not history.exists():
            with gzip.open(history, 'wt', encoding='utf-8') as stream:
                json.dump({'fingerprint': self.fingerprint, 'files': self.input_manifest}, stream)
        dump(args.output / 'input-manifest.json', {'fingerprint': self.fingerprint, 'files': self.input_manifest})
        self.count = 0
        self.literal_values = {}

    def job(self, key, source, compiler_args, roots, macro_names='', mode='extract'):
        cache = self.args.output / 'cache' / key
        value = {'source': str(source.resolve()), 'output': str(cache) + '.json.gz', 'nativeLibraries': str(self.native), 'arguments': compiler_args, 'roots': [str(path.resolve()) for path in roots], 'macroNames': str(macro_names), 'mode': mode}
        backend = self.args.output / 'bin/Extractor.Inventory.exe' if mode in ('templates', 'values') else self.worker
        extra_fingerprint = digest(backend.with_suffix('.dll')) if mode in ('templates', 'values') else ''
        signature = hashlib.sha256((self.fingerprint + extra_fingerprint + json.dumps(value, sort_keys=True) + digest(source) + (digest(Path(macro_names)) if macro_names and Path(macro_names).exists() else '')).encode()).hexdigest()
        marker = Path(str(cache) + '.signature')
        if Path(value['output']).exists() and marker.exists() and marker.read_text() == signature:
            return value['output']
        job_file = Path(str(cache) + '.job.json')
        dump(job_file, value)
        Path(value['output']).unlink(missing_ok=True)
        marker.unlink(missing_ok=True)
        process = subprocess.run([str(backend), str(job_file)], capture_output=True, text=True, encoding='utf-8', errors='replace')
        Path(str(cache) + '.log').write_text(process.stdout + process.stderr, encoding='utf-8')
        if not Path(value['output']).exists():
            raise RuntimeError('AST 进程未输出结果：' + key + '\n' + process.stdout + process.stderr)
        result = load(value['output'])
        if not result.get('failure') and result.get('parseCode') == 0:
            marker.write_text(signature)
        self.count += 1
        errors = sum(item['severity'] >= 3 for item in result['diagnostics'])
        print(f"[{self.count}] {key}: {len(result['symbols'])} 个声明，{errors} 个解析错误，{result['seconds']:.1f} 秒", flush=True)
        return value['output']

    def projection(self, arch, character_set):
        folder = self.args.output / 'inputs'
        folder.mkdir(parents=True, exist_ok=True)
        source = folder / 'projection.cpp'
        header_files = [*sorted(self.args.headers.glob('*.h')), *sorted(path for path in self.args.headers.rglob('*.h') if path.parent != self.args.headers)]
        text = 'namespace PSDK { namespace Gdiplus {} }\n' + ''.join('#include "win32/' + str(path.relative_to(self.args.headers)).replace('\\', '/') + '"\n' for path in header_files)
        source.write_text(text, encoding='utf-8')
        return self.job(f'{arch}-{character_set}/projection', source, ['-I' + str(self.args.headers.parent), *arguments(self.args.metadata, arch, character_set, True)], [self.args.headers])

    def sdk_job(self, partition, arch, character_set, macro_names):
        settings_file = self.sdk / 'Partitions' / partition / 'settings.rsp'
        settings = rsp(settings_file)
        source = self.sdk / 'Partitions' / partition / 'main.cpp'
        compiler_args = [*arguments(self.args.metadata, arch, character_set), *settings.get('--additional', [])]
        if settings.get('--std'):
            compiler_args = [item for item in compiler_args if not item.startswith('-std=')] + ['-std=' + settings['--std'][-1]]
        scratch_file = self.sdk / 'obj/scratch' / arch / (partition + '.settings.rsp')
        scratch = rsp(scratch_file if scratch_file.exists() else self.sdk / 'obj/scratch/common' / (partition + '.settings.rsp'))
        compiler_args += ['-I' + path for path in scratch.get('--include-directory', [])]
        return self.job(f'{arch}-{character_set}/sdk-{partition}', source, compiler_args, [self.sdk / 'RecompiledIdlHeaders', self.sdk / 'inc'], macro_names)

    def complete_string_values(self, snapshot_path, restrict_names=None, snapshot=None):
        snapshot = snapshot if snapshot is not None else load(snapshot_path)
        completed = {}
        pure_keys = {}
        def needs_completion(entry):
            def legacy(value):
                return value is not None and (value.get('kind') == 'string' or value.get('kind') == 'aggregate' and any(legacy(item) for item in value['items']))
            if legacy(entry.get('value')):
                return True
            if entry['kind'] != 'variable' or entry['type'] < 0 or entry.get('value') is not None:
                return False
            raw = entry.get('raw') or ''
            if '=' not in raw and '{' not in raw:
                return False
            type = snapshot['types'][entry['type']]
            return type['kind'] in ('Pointer', 'ConstantArray', 'IncompleteArray') and type['element'] >= 0 and snapshot['types'][type['element']]['kind'] in ('Char_U', 'UChar', 'Char_S', 'SChar', 'WChar', 'Char16', 'Char32')
        wanted = []
        for entry in snapshot['symbols']:
            if restrict_names is not None and entry['name'] not in restrict_names:
                continue
            if not needs_completion(entry):
                continue
            identity = entry['usr'], entry['site']['file'], entry['site']['line']
            raw = entry.get('raw') or ''
            expression = raw.split('=', 1)[1].strip().rstrip(';').strip() if '=' in raw else ''
            remainder = re.sub(r'(?:u8|u|U|L)?"(?:[^"\\]|\\.)*"', '', expression)
            if expression and expression.isascii() and not remainder.strip(' \t\r\n(){}'):
                type = snapshot['types'][entry['type']]
                encoding_flags = tuple(item for item in snapshot['arguments'] if item.startswith(('-fexec-charset', '-fwide-exec-charset', '-finput-charset', '-fshort-wchar', '-fno-short-wchar', '-fchar8_t', '-fno-char8_t')))
                key = expression, type['canonical'], Path(snapshot_path).parent.name, encoding_flags
                pure_keys[identity] = key
                if key in self.literal_values:
                    completed[identity] = self.literal_values[key]
                    continue
            wanted.append(entry['name'])
        if not wanted:
            return completed
        job = load(Path(str(snapshot_path)[:-len('.json.gz')] + '.job.json'))
        token = hashlib.sha256(str(snapshot_path).encode()).hexdigest()[:20]
        names = self.args.output / 'inputs' / ('strings-' + token + '.txt')
        names.write_text('\n'.join(sorted(set(wanted))), encoding='utf-8')
        key = Path(snapshot_path).parent.name + '/string-values-' + token
        result = self.job(key, Path(job['source']), job['arguments'], [Path(path) for path in job['roots']], names, mode='values')
        values = load(result)
        clean = not values.get('failure') and values.get('parseCode') == 0 and not any(item['severity'] >= 3 for item in values['diagnostics'])
        if not clean:
            return completed
        for entry in values['symbols']:
            identity = entry['usr'], entry['site']['file'], entry['site']['line']
            completed[identity] = entry['value']
            if identity in pure_keys and entry['value'] is not None:
                self.literal_values[pure_keys[identity]] = entry['value']
        return completed


def main():
    parser = argparse.ArgumentParser(description='对导出头和原始 SDK 的 Clang AST 做逐符号比较。')
    parser.add_argument('--metadata', type=Path, default=Path('L:/Open_Code/win32metadata'))
    parser.add_argument('--headers', type=Path, default=Path('Z:/ms/win32'))
    parser.add_argument('--output', type=Path, default=Path('Z:/sdk-deep-audit-20261105'))
    parser.add_argument('--architectures', nargs='+', choices=TARGETS, default=['x86', 'x64', 'arm64'])
    parser.add_argument('--character-sets', nargs='+', choices=['Ansi', 'Unicode'], default=['Ansi', 'Unicode'])
    parser.add_argument('--partitions', nargs='*')
    parser.add_argument('--jobs', type=int, default=2)
    parser.add_argument('--extract-only', action='store_true')
    parser.add_argument('--compare-only', action='store_true')
    parser.add_argument('--configuration', help='只比较已提取的一组，例如 x64-Unicode')
    parser.add_argument('--fail-on-difference', action='store_true')
    args = parser.parse_args()
    runner = Runner(args)
    if args.compare_only:
        run_manifest = load(args.output / 'run.json')
        if run_manifest['fingerprint'] != runner.fingerprint:
            raise RuntimeError('输入或 AST 提取器已变化，不能复用旧快照；请先重新提取')
        if args.configuration:
            run_manifest['configurations'] = [item for item in run_manifest['configurations'] if item['architecture'] + '-' + item['characterSet'] == args.configuration]
            if not run_manifest['configurations']:
                raise ValueError('找不到该配置的 AST 快照')
        from compare import compare_run
        compare_run(args, runner, run_manifest)
        return
    partitions = args.partitions or sorted(path.parent.name for path in runner.sdk.glob('Partitions/*/main.cpp'))
    run_manifest = {'fingerprint': runner.fingerprint, 'configurations': [], 'partitions': partitions, 'started': time.time()}
    dump(args.output / 'run.json', run_manifest)
    for arch in args.architectures:
        for character_set in args.character_sets:
            projection = runner.projection(arch, character_set)
            inventory = load(projection)
            names = {symbol['name'] for symbol in inventory['symbols'] if re.fullmatch(r'[A-Za-z_]\w*', symbol['name'])}
            macro_names = args.output / 'inputs' / f'{arch}-{character_set}-names.txt'
            macro_names.write_text('\n'.join(sorted(names)), encoding='utf-8')
            if inventory.get('failure') or inventory.get('parseCode'):
                raise RuntimeError('导出头 AST 提取失败：' + projection)
            with concurrent.futures.ThreadPoolExecutor(max_workers=args.jobs) as pool:
                futures = {pool.submit(runner.sdk_job, partition, arch, character_set, macro_names): partition for partition in partitions}
                sources = {futures[future]: future.result() for future in concurrent.futures.as_completed(futures)}
            config = {'architecture': arch, 'characterSet': character_set, 'projection': projection, 'sdk': sources}
            template_job = load(Path(str(projection)[:-len('.json.gz')] + '.job.json'))
            config['templates'] = runner.job(f'{arch}-{character_set}/projection-templates', Path(template_job['source']), template_job['arguments'], [args.headers], mode='templates')
            run_manifest['configurations'].append(config)
            dump(args.output / 'run.json', run_manifest)
    if not args.extract_only:
        from compare import compare_run
        compare_run(args, runner, run_manifest)
    run_manifest['finished'] = time.time()
    dump(args.output / 'run.json', run_manifest)


if __name__ == '__main__':
    main()
