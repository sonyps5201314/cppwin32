import collections
import csv
from dataclasses import dataclass, field
import gzip
import hashlib
import html
import json
import pickle
import math
from pathlib import Path
import re
import sys

from run import dump, load, rsp

COMPARISON_HASH = hashlib.sha256(Path(__file__).read_bytes()).hexdigest()
POINTER_SIZE = 8


def normalized(name):
    return re.sub(r'^win32::Windows::Win32(?:::\w+)*::', '', name) if name.startswith('win32::Windows::Win32::') else name


def leaf(name):
    return name.rsplit('::', 1)[-1]


def spelling(text):
    return tuple(re.findall(r'[A-Za-z_]\w*|\d+|[^\w\s]', re.sub(r'win32::Windows::Win32(?:::\w+)*::', '', text)))


@dataclass(frozen=True)
class Type:
    written: str
    canonical: str
    kind: str
    size: int
    alignment: int
    declaration: str
    declaration_usr: str
    uuid: str
    const: bool
    volatile: bool
    restrict: bool
    element: object
    count: int
    result: object
    arguments: tuple
    calling_convention: str
    exception: str
    variadic: bool
    _cached_hash: int = field(init=False, repr=False, compare=False)

    def __post_init__(self):
        object.__setattr__(self, '_cached_hash', hash((self.written, self.canonical, self.kind, self.size, self.alignment, self.declaration, self.declaration_usr, self.uuid, self.const, self.volatile, self.restrict, self.element, self.count, self.result, self.arguments, self.calling_convention, self.exception, self.variadic)))

    def __hash__(self):
        return self._cached_hash

    def __getstate__(self):
        return {key: value for key, value in self.__dict__.items() if key != '_cached_hash'}

    def __setstate__(self, state):
        for key, value in state.items():
            object.__setattr__(self, key, value)
        self.__post_init__()

    def brief(self):
        return {'written': self.written, 'canonical': self.canonical, 'kind': self.kind, 'size': self.size, 'alignment': self.alignment}


class Types:
    def __init__(self):
        self.interned = {}

    def unpack(self, entries, uuids=None):
        cache = {}

        def get(index):
            if index < 0:
                return None
            if index in cache:
                return cache[index]
            entry = entries[index]
            value = Type(*(sys.intern(entry[key]) for key in ('written', 'canonical', 'kind')), entry['size'], entry['alignment'], sys.intern(entry['declaration']), sys.intern(entry['declarationUsr']), (uuids or {}).get(entry['declarationUsr'], ''), entry['const'], entry['volatile'], entry['restrict'], get(entry['element']), entry['count'], get(entry['result']), tuple(get(item) for item in entry['arguments']), entry['callingConvention'], entry['exceptionSpecification'], entry['variadic'])
            cache[index] = self.interned.setdefault(value, value)
            return cache[index]

        return get


@dataclass
class Declaration:
    row: dict
    type: object
    result: object
    parameters: tuple
    bases: tuple
    job: str
    clean: bool

    @property
    def name(self):
        return self.row['name']

    @property
    def kind(self):
        return self.row['kind']

    def brief(self):
        scope = 'scraper_support' if '/winsdk/inc/' in self.row['site']['file'].lower() else 'requested_sdk' if '/recompiledidlheaders/' in self.row['site']['file'].lower() else 'projection_or_fixture'
        return {'kind': self.kind, 'name': self.row['qualified'], 'site': self.row['site'], 'source_scope': scope, 'type': self.type.brief() if self.type else None, 'result': self.result.brief() if self.result else None, 'parameters': [{'name': item[0], 'type': item[1].brief() if item[1] else None, 'site': item[2], 'raw': item[3]} for item in self.parameters], 'raw': self.row.get('raw'), 'value': self.row['value'], 'uuid': self.row['uuid'], 'snapshot': self.job, 'clean_ast': self.clean}


def valid_snapshot(snapshot):
    return not snapshot.get('failure') and snapshot.get('parseCode') == 0 and not any(item['severity'] >= 3 for item in snapshot['diagnostics'])


def declarations(path, types, completed_values=None, snapshot=None):
    snapshot = snapshot if snapshot is not None else load(path)
    uuids = {row['usr']: row['uuid'] for row in snapshot['symbols'] if row['kind'] == 'record' and row['uuid']}
    get = types.unpack(snapshot['types'], uuids)
    clean = valid_snapshot(snapshot)
    for row in snapshot['symbols']:
        if completed_values:
            key = row['usr'], row['site']['file'], row['site']['line']
            if key in completed_values:
                row['value'] = completed_values[key]
        yield Declaration(row, get(row['type']), get(row['result']), tuple((parameter['name'], get(parameter['type']), parameter['site'], parameter.get('raw')) for parameter in row['parameters']), tuple(get(base) for base in row['bases']), str(path), clean)


def value_key(value):
    if not value:
        return None
    if value['kind'] == 'aggregate':
        values = tuple(value_key(item) for item in value['items'])
        return None if any(item is None for item in values) else values
    if value['kind'] == 'integer':
        return int(value['text'])
    if value['kind'] == 'float':
        value = float(value['text'])
        return None if math.isnan(value) else value
    if value['kind'] == 'string':
        return None
    if value['kind'] == 'string_bytes':
        return ('string_bytes', value['elementWidth'], value['bytes'])
    return value['text']


def guid_value(value):
    value = value_key(value)
    if not isinstance(value, tuple):
        return None
    flat = []

    def walk(item):
        if isinstance(item, tuple):
            for element in item:
                walk(element)
        else:
            flat.append(item)
    walk(value)
    if len(flat) != 11 or not all(isinstance(item, int) for item in flat):
        return None
    return f'{flat[0]:08X}-{flat[1]:04X}-{flat[2]:04X}-{flat[3]:02X}{flat[4]:02X}-' + ''.join(f'{item:02X}' for item in flat[5:])


class Index:
    def __init__(self):
        self.by_name = collections.defaultdict(list)
        self.by_usr = collections.defaultdict(list)
        self.members = collections.defaultdict(list)
        self.macros = collections.defaultdict(list)
        self.unique = {}
        self.sources = []

    def add(self, declaration):
        row = declaration.row
        key = (declaration.kind, row['usr'] or row['qualified'], row['site']['file'].lower(), row['site']['line'], declaration.type, declaration.result, tuple((p[0], p[1]) for p in declaration.parameters), row['offsetBits'], row['bitWidth'], json.dumps(row['value'], sort_keys=True), row['uuid'], tuple(row['tokens'] or []), row['definition'])
        existing = self.unique.get(key)
        if existing is not None:
            if declaration.clean and not existing.clean:
                existing.clean, existing.job = True, declaration.job
            return
        self.unique[key] = declaration
        self.by_name[declaration.name].append(declaration)
        if row['usr']:
            self.by_usr[row['usr']].append(declaration)
        if row['ownerUsr']:
            self.members[row['ownerUsr']].append(declaration)
        if declaration.kind == 'macro':
            self.macros[declaration.name].append(declaration)

    def add_file(self, path, types, completed_values=None, snapshot=None):
        virtual_orders = collections.Counter()
        for declaration in declarations(path, types, completed_values, snapshot):
            if declaration.kind == 'method':
                owner = declaration.row['ownerUsr']
                declaration.row['order'] = virtual_orders[owner] if declaration.row['virtual'] else -1
                if declaration.row['virtual']:
                    virtual_orders[owner] += 1
            self.add(declaration)
        self.sources.append(path)


def choose(candidates):
    candidates = list(candidates)
    definitions = {(entry.kind, entry.row['usr']) for entry in candidates if entry.kind in ('record', 'enum') and entry.row['definition']}
    result = {}
    for candidate in candidates:
        row = candidate.row
        if candidate.kind in ('record', 'enum') and not row['definition'] and (candidate.kind, row['usr']) in definitions:
            continue
        if candidate.kind == 'macro':
            key = ('macro', row['name'], row['site']['file'], row['site']['line'], tuple(row['tokens'] or []))
        else:
            key = (candidate.kind, row['usr'] or row['qualified'], row['site']['file'], row['site']['line'], candidate.type, row['uuid'])
        previous = result.get(key)
        rank = (candidate.clean, row['definition'], candidate.type is not None and candidate.type.size >= 0, row['value'] is not None, row['uuid'] is not None)
        if previous is None or rank > previous[0]:
            result[key] = rank, candidate
    return [entry[1] for entry in result.values()]


class EnumRules:
    def __init__(self, path):
        data = load(path)
        self.items = data.get('items', []) if isinstance(data, dict) else data
        self.names = {item.get('name', item.get('addUsesTo')) for item in self.items}
        self.fields = collections.defaultdict(set)
        self.parameters = collections.defaultdict(set)
        self.owner_aliases = collections.defaultdict(set)
        self.renames = collections.defaultdict(set)
        configurations = [path.parent / 'scraper.settings.rsp', *path.parent.glob('Partitions/*/settings.rsp')]
        for configuration in configurations:
            for mapping in rsp(configuration).get('--remap', []):
                if '=' not in mapping:
                    continue
                original, generated = mapping.split('=', 1)
                if original != generated and re.fullmatch(r'[A-Za-z_]\w*', original) and re.fullmatch(r'[A-Za-z_]\w*', generated):
                    self.renames[generated].add(original)
        for item in self.items:
            name = item.get('name', item.get('addUsesTo'))
            if not name:
                raise ValueError('枚举配置缺少 name 和 addUsesTo')
            for use in item.get('uses', []):
                if 'struct' in use:
                    self.fields[use['struct'], use['field']].add(name)
                elif 'method' in use:
                    self.parameters[use.get('interface', ''), use['method'], use['parameter']].add(name)

    def allowed(self, declaration, slot, projected_type, reference):
        if projected_type is None:
            return False
        while projected_type and projected_type.kind in ('Pointer', 'LValueReference', 'RValueReference', 'ConstantArray', 'IncompleteArray'):
            projected_type = projected_type.element
        if not projected_type or projected_type.kind != 'Enum':
            return False
        enum = leaf(projected_type.declaration)
        owners = {leaf(reference.row['owner']), *self.owner_aliases.get(reference.row['ownerUsr'], ())}
        if declaration.kind == 'field':
            names = set().union(*(self.fields[owner, reference.name] for owner in owners))
        else:
            names = set().union(*(self.parameters[owner if declaration.kind == 'method' else '', reference.name, slot] for owner in owners))
        return enum in names


def abi_shape(value, enum_replacement=False, indirect=False):
    if value is None:
        return None
    kind = value.kind
    if kind == 'Enum' and enum_replacement:
        underlying = abi_shape(value.element, True, indirect)
        if underlying is None:
            return None
        shape = list(underlying)
        shape[3] = shape[3] or value.const
        shape[4] = shape[4] or value.volatile
        shape[5] = shape[5] or value.restrict
        return tuple(shape)
    if enum_replacement and kind in ('Bool', 'Char_U', 'UChar', 'UShort', 'UInt', 'ULong', 'ULongLong', 'UInt128', 'Char_S', 'SChar', 'WChar', 'Short', 'Int', 'Long', 'LongLong', 'Int128'):
        kind = 'configured_integer'
    if kind in ('Record', 'Enum'):
        identity = (leaf(value.declaration), value.uuid)
    else:
        identity = ''
    ignore_layout = kind == 'Record' or indirect and kind == 'Enum'
    pointee = kind in ('Pointer', 'LValueReference', 'RValueReference')
    reference = kind in ('LValueReference', 'RValueReference')
    return (kind, None if ignore_layout else POINTER_SIZE if reference else value.size, None if ignore_layout else POINTER_SIZE if reference else value.alignment, value.const, value.volatile, value.restrict, identity, value.count, abi_shape(value.element, enum_replacement, pointee), abi_shape(value.result, enum_replacement), tuple(abi_shape(argument, enum_replacement) for argument in value.arguments), value.calling_convention, value.variadic)


def difference(kind, site, before, after):
    return {'kind': kind, 'site': site, 'sdk': before, 'projection': after}


def compare_constant_value(projected, reference):
    actual, expected = value_key(projected.row['value']), value_key(reference.row['value'])
    if actual is None or expected is None:
        return [], ['constant_value:not_evaluated']
    for declaration in (projected, reference):
        if isinstance(value_key(declaration.row['value']), (int, float)) and declaration.type and declaration.type.size > 8:
            return [], ['constant_value:libclang_scalar_precision_exceeds_64_bits']
    if actual == expected:
        return [], []
    if isinstance(actual, int) and isinstance(expected, int) and projected.type and reference.type and projected.type.size == reference.type.size and 0 < projected.type.size <= 8:
        mask = (1 << (projected.type.size * 8)) - 1
        if actual & mask == expected & mask:
            return [difference('constant_numeric_representation', '', expected, actual)], []
    return [difference('constant_value', '', expected, actual)], []


class MacroTypes:
    def __init__(self, sdk):
        self.bodies = {}
        for name, declarations in sdk.macros.items():
            bodies = {tuple(entry.row['tokens'][1:]) for entry in declarations if not entry.row['functionLike'] and entry.row['tokens'] and all(re.fullmatch(r'[A-Za-z_]\w*|[*&]', word) for word in entry.row['tokens'][1:])}
            if len(bodies) == 1:
                body = next(iter(bodies))
                if body:
                    self.bodies[name] = body

    def display(self, type, raw):
        tokens = list(spelling(type.written))
        if not raw:
            return tuple(tokens)
        for name in re.findall(r'\b[A-Za-z_]\w*\b', raw):
            if name not in self.bodies:
                continue
            body, active = self.bodies[name], {name}
            for _ in range(16):
                if len(body) != 1 or body[0] not in self.bodies or body[0] in active:
                    break
                active.add(body[0])
                body = self.bodies[body[0]]
            for index in range(len(tokens) - len(body) + 1):
                if tuple(tokens[index:index + len(body)]) == body:
                    tokens[index:index + len(body)] = [name]
                    break
        return tuple(tokens)


def compare_type(projected, reference, site, allow_enum=False, macro_types=None, projected_raw=None, reference_raw=None, abi_storage=False):
    changes, unknown = [], []
    if projected is None or reference is None:
        return changes, [site + ':缺少类型']
    measurable = not (projected.kind.startswith('Function') or reference.kind.startswith('Function') or projected.kind == reference.kind == 'Void')
    if measurable:
        for field in ('size', 'alignment'):
            a, b = getattr(reference, field), getattr(projected, field)
            if abi_storage:
                if reference.kind in ('LValueReference', 'RValueReference'):
                    a = POINTER_SIZE
                if projected.kind in ('LValueReference', 'RValueReference'):
                    b = POINTER_SIZE
            if a >= 0 and b >= 0:
                if a != b:
                    kind = 'cxx_sizeof_' + field if not abi_storage and (reference.kind in ('LValueReference', 'RValueReference') or projected.kind in ('LValueReference', 'RValueReference')) else 'type_' + field
                    changes.append(difference(kind, site, a, b))
            else:
                unknown.append(site + ':' + field + ':incomplete_or_invalid')
    if abi_shape(projected, allow_enum) != abi_shape(reference, allow_enum):
        changes.append(difference('type_structure', site, reference.brief(), projected.brief()))
    actual = macro_types.display(projected, projected_raw) if macro_types else spelling(projected.written)
    expected = macro_types.display(reference, reference_raw) if macro_types else spelling(reference.written)
    if not allow_enum and actual != expected:
        changes.append(difference('type_spelling', site, {'ast': reference.written, 'source_tokens': expected, 'declaration': reference_raw}, {'ast': projected.written, 'source_tokens': actual, 'declaration': projected_raw}))
    return changes, unknown


def compare_pair(projected, reference, rules, macro_types=None):
    changes, unknown, exemptions = [], [], []
    p, r = projected.row, reference.row
    if p['name'] != r['name']:
        changes.append(difference('name', '', r['name'], p['name']))
    if projected.kind == 'record':
        if reference.type and projected.type:
            for field in ('size', 'alignment'):
                a, b = getattr(reference.type, field), getattr(projected.type, field)
                if a >= 0 and b >= 0:
                    if a != b:
                        changes.append(difference('record_' + field, '', a, b))
                else:
                    unknown.append('record_' + field + ':incomplete_or_invalid')
        if p['uuid'] != r['uuid']:
            changes.append(difference('uuid_attribute', '', r['uuid'], p['uuid']))
        if tuple(leaf(base.declaration) for base in projected.bases) != tuple(leaf(base.declaration) for base in reference.bases):
            changes.append(difference('base_classes', '', [base.brief() for base in reference.bases], [base.brief() for base in projected.bases]))
    elif projected.kind in ('method', 'function'):
        if len(projected.parameters) != len(reference.parameters):
            changes.append(difference('parameter_count', '', len(reference.parameters), len(projected.parameters)))
        allow = rules.allowed(projected, 'return', projected.result, reference)
        actual_return = re.split(r'\b' + re.escape(projected.name) + r'\b', p.get('raw') or '', maxsplit=1)[0]
        expected_return = re.split(r'\b' + re.escape(reference.name) + r'\b', r.get('raw') or '', maxsplit=1)[0]
        a, b = compare_type(projected.result, reference.result, 'return', allow, macro_types, actual_return, expected_return, True)
        changes += a; unknown += b
        if allow:
            exemptions.append('return:enums.json')
        for index, (actual, expected) in enumerate(zip(projected.parameters, reference.parameters)):
            if actual[0] != expected[0]:
                changes.append(difference('parameter_name', str(index), expected[0], actual[0]))
            allow = rules.allowed(projected, expected[0], actual[1], reference)
            a, b = compare_type(actual[1], expected[1], 'parameter:' + str(index) + ':' + expected[0], allow, macro_types, actual[3], expected[3], True)
            changes += a; unknown += b
            if allow:
                exemptions.append(expected[0] + ':enums.json')
        if projected.type and reference.type:
            for field in ('calling_convention', 'variadic', 'exception'):
                a, b = getattr(reference.type, field), getattr(projected.type, field)
                if a != b:
                    changes.append(difference(field, '', a, b))
        if projected.kind == 'method':
            for field in ('virtual', 'pure', 'order'):
                if p[field] != r[field]:
                    changes.append(difference('method_' + field, '', r[field], p[field]))
    elif projected.kind == 'field':
        for field in ('offsetBits', 'bitWidth'):
            if p[field] >= 0 and r[field] >= 0 or field == 'bitWidth':
                if p[field] != r[field]:
                    changes.append(difference(field, '', r[field], p[field]))
            else:
                unknown.append('field_offset:incomplete_or_invalid')
        allow = rules.allowed(projected, p['name'], projected.type, reference)
        a, b = compare_type(projected.type, reference.type, 'field', allow, macro_types, p.get('raw'), r.get('raw'), True)
        changes += a; unknown += b
        if allow:
            exemptions.append('field:enums.json')
    elif projected.kind == 'macro':
        if p['functionLike'] != r['functionLike'] or p['tokens'] != r['tokens']:
            changes.append(difference('macro_tokens', '', r['tokens'], p['tokens']))
    else:
        a, b = compare_type(projected.type, reference.type, 'type', False, macro_types, p.get('raw'), r.get('raw'))
        changes += a; unknown += b
        if projected.kind in ('variable', 'enumerator'):
            a, b = compare_constant_value(projected, reference)
            changes += a; unknown += b
    return changes, sorted(set(unknown)), exemptions


def candidates(projected, sdk, owner_matches, projection=None):
    row = projected.row
    kind = projected.kind
    def members(owner):
        all_members = sdk.members[owner.row['usr']]
        local = [member for member in all_members if member.row['site']['file'] == owner.row['site']['file']]
        return local or all_members
    if kind in ('field', 'method'):
        owners = owner_matches.get(row['ownerUsr'], [])
        exact = choose(member for owner in owners for member in members(owner) if member.kind == kind and member.name == projected.name and (projected.name or member.row['offsetBits'] == row['offsetBits']))
        if exact or kind == 'method':
            return exact
        spatial = choose(member for owner in owners for member in members(owner) if member.kind == 'field' and row['offsetBits'] >= 0 and member.row['offsetBits'] == row['offsetBits'] and member.row['bitWidth'] == row['bitWidth'])
        return spatial if len(spatial) == 1 else []
    matches = [entry for entry in sdk.by_name[projected.name] if entry.kind == kind] if projected.name else []
    if kind in ('record', 'enum'):
        for alias in sdk.by_name[projected.name]:
            if alias.kind == 'typedef' and alias.type:
                matches += [entry for entry in sdk.by_usr[alias.type.declaration_usr] if entry.kind == kind]
        if not matches and row['ownerUsr'] in owner_matches and projection is not None:
            for field in projection.members[row['ownerUsr']]:
                field_type = field.type
                while field_type and field_type.kind in ('ConstantArray', 'IncompleteArray'):
                    field_type = field_type.element
                if field.kind != 'field' or not field_type or field_type.declaration_usr != row['usr'] or field.row['offsetBits'] < 0:
                    continue
                for owner in owner_matches[row['ownerUsr']]:
                    for source_field in members(owner):
                        if source_field.kind == 'field' and source_field.row['offsetBits'] == field.row['offsetBits'] and source_field.type:
                            target = source_field.type
                            while target and target.kind in ('ConstantArray', 'IncompleteArray'):
                                target = target.element
                            if target:
                                matches += [entry for entry in sdk.by_usr[target.declaration_usr] if entry.kind == kind]
    if kind in ('enumerator', 'variable'):
        matches += [entry for entry in sdk.by_name[projected.name] if entry.kind in ('enumerator', 'variable') and entry not in matches]
    return choose(matches)


def support_symbol(declaration):
    row = declaration.row
    path = row['site']['file'].replace('\\', '/')
    if row['qualified'].startswith('win32::_impl_::'):
        return 'cppwin32_support'
    if '/impl/System' in path or Path(path).name.startswith('System.'):
        return 'managed_metadata_support'
    if Path(path).name == 'base.h' and row['qualified'].startswith('win32::'):
        return 'cppwin32_support'
    return None


def compare_configuration(config, rules, index_cache=None, runner=None):
    global POINTER_SIZE
    POINTER_SIZE = 4 if config.get('architecture', 'x86') == 'x86' else 8
    types = Types()
    projection = Index()
    projected_snapshot = load(config['projection'])
    projection.add_file(config['projection'], types, runner.complete_string_values(config['projection'], snapshot=projected_snapshot) if runner else None, projected_snapshot)
    value_names = {entry.name for entry in projection.unique.values() if entry.kind in ('variable', 'enumerator')}
    if config.get('templates'):
        projection.add_file(config['templates'], types)
    if len(set(config['sdk'].values())) != len(config['sdk']):
        raise ValueError('多个 SDK 分区错误地共用了同一个快照路径')
    inventory_hash = hashlib.sha256((runner.args.output / 'bin/Extractor.Inventory.dll').read_bytes()).hexdigest() if runner else ''
    signature = hashlib.sha256(('sdk-index-v3' + inventory_hash + json.dumps([(path, Path(path[:-len('.json.gz')] + '.signature').read_text() if Path(path[:-len('.json.gz')] + '.signature').exists() else str(Path(path).stat().st_mtime_ns)) for path in sorted(config['sdk'].values())])).encode()).hexdigest()
    marker = Path(str(index_cache) + '.signature') if index_cache else None
    if index_cache and index_cache.exists() and marker.exists() and marker.read_text() == signature:
        with gzip.open(index_cache, 'rb') as stream:
            sdk = pickle.load(stream)
        print('已复用 SDK 符号索引', flush=True)
    else:
        sdk = Index()
        for number, path in enumerate(sorted(config['sdk'].values()), 1):
            snapshot = load(path)
            sdk.add_file(path, types, runner.complete_string_values(path, value_names, snapshot) if runner else None, snapshot)
            if number % 32 == 0:
                print(f'SDK 符号索引：{number}/{len(config["sdk"])}，去重后 {len(sdk.unique)} 条', flush=True)
        if index_cache:
            index_cache.parent.mkdir(parents=True, exist_ok=True)
            with gzip.open(index_cache, 'wb', compresslevel=3) as stream:
                pickle.dump(sdk, stream, protocol=5)
            marker.write_text(signature)
    rules.owner_aliases = collections.defaultdict(set)
    for entry in sdk.unique.values():
        if entry.kind == 'typedef' and entry.type and entry.type.declaration_usr:
            rules.owner_aliases[entry.type.declaration_usr].add(entry.name)
    macro_types = MacroTypes(sdk)
    primary = choose(projection.unique.values())
    primary.sort(key=lambda entry: ({'record': 0, 'enum': 1, 'typedef': 2, 'field': 3, 'method': 4}.get(entry.kind, 5), entry.row['qualified']))
    owner_matches, rows, pending_macros = {}, [], []
    for projected in primary:
        row = {'kind': projected.kind, 'name': projected.row['qualified'], 'projection': projected.brief(), 'status': '', 'differences': [], 'unverified': [], 'exemptions': []}
        support = support_symbol(projected)
        if support:
            row['status'] = support if projected.clean else 'parse_tainted'
            rows.append(row)
            continue
        if projected.kind.startswith('template_'):
            row['status'] = 'template_not_instantiated'
            row['unverified'] = ['模板未实例化，不能确定通用布局；需为具体类型实参另行实例化']
            rows.append(row)
            continue
        if projected.kind == 'enum' and projected.name in rules.names:
            row['status'] = 'configured_enum_projection' if projected.clean else 'parse_tainted'
            rows.append(row)
            continue
        matches = candidates(projected, sdk, owner_matches, projection)
        if projected.kind in ('record', 'enum', 'typedef'):
            for original in rules.renames.get(projected.name, ()):
                matches += choose(entry for entry in sdk.by_name[original] if entry.kind == projected.kind)
            matches = choose(matches)
        if projected.kind == 'record' and projected.row['uuid']:
            matching_uuid = [entry for entry in matches if entry.row['uuid'] == projected.row['uuid']]
            if matching_uuid:
                matches = matching_uuid
        if projected.kind in ('function', 'method') and len(matches) > 1:
            arity = [entry for entry in matches if len(entry.parameters) == len(projected.parameters)]
            if arity:
                matches = arity
            compatible = [entry for entry in matches if abi_shape(entry.result) == abi_shape(projected.result) and all(abi_shape(a[1]) == abi_shape(b[1]) for a, b in zip(entry.parameters, projected.parameters))]
            if len(compatible) == 1:
                matches = compatible
        if projected.kind == 'variable' and projected.name.startswith('IID_'):
            uuid = guid_value(projected.row['value'])
            source_names = {projected.name[4:], *rules.renames.get(projected.name[4:], ())}
            uuid_sources = choose(entry for name in source_names for entry in sdk.by_name[name] if entry.kind == 'record' and entry.row['uuid'])
            uuid_sources = [entry for entry in uuid_sources if entry.clean]
            if uuid_sources:
                same_uuid = [entry for entry in uuid_sources if entry.row['uuid'] == uuid]
                if same_uuid:
                    uuid_sources = same_uuid
                expected = {entry.row['uuid'] for entry in uuid_sources}
                row['sdk_candidates'] = [entry.brief() for entry in uuid_sources]
                row['status'] = 'ambiguous' if len(expected) != 1 else 'matched_iid_uuid' if uuid in expected else 'different'
                if row['status'] == 'different':
                    row['differences'] = [difference('iid_uuid_value', '', sorted(expected), uuid)]
                if projected.type is None or projected.type.size != 16:
                    row['differences'].append(difference('iid_size', '', 16, projected.type.size if projected.type else None))
                    row['status'] = 'different'
                if not projected.clean:
                    row['status'] = 'parse_tainted'
                rows.append(row)
                continue
        if not matches and projected.kind in ('variable', 'enumerator'):
            macros = choose(entry for entry in sdk.macros[projected.name] if not entry.row['functionLike'])
            if macros:
                row['sdk_candidates'] = [entry.brief() for entry in macros]
                row['status'] = 'macro_pending'
                pending_macros.append((projected, macros, row))
                rows.append(row)
                continue
        if not matches:
            row['status'] = 'unmatched'
            rows.append(row)
            continue
        clean = [entry for entry in matches if entry.clean]
        if clean:
            matches = clean
        row['sdk_candidates'] = [entry.brief() for entry in matches]
        if len(matches) > 1:
            signatures = {json.dumps((entry.type.brief() if entry.type else None, [(p[0], p[1].brief() if p[1] else None) for p in entry.parameters], entry.row['offsetBits'], entry.row['bitWidth'], entry.row['value'], entry.row['uuid']), sort_keys=True) for entry in matches}
            if len(signatures) > 1:
                row['status'] = 'ambiguous'
                rows.append(row)
                continue
        compared = [(entry, *compare_pair(projected, entry, rules, macro_types)) for entry in matches]
        exact = [item for item in compared if not item[1] and not item[2]]
        chosen = exact[0] if exact else compared[0]
        reference, row['differences'], row['unverified'], row['exemptions'] = chosen
        if projected.kind in ('record', 'enum'):
            owner_matches[projected.row['usr']] = matches
        row['status'] = 'different' if row['differences'] else 'unverified' if row['unverified'] else 'matched_with_enum_rule' if row['exemptions'] else 'matched'
        if not projected.clean or not reference.clean:
            row['status'] = 'parse_tainted'
        rows.append(row)
    return rows, pending_macros, sdk


def evaluate_macros(args, runner, config, pending):
    groups = collections.defaultdict(dict)
    values, failures = {}, {}
    for projected, macros, row in pending:
        for macro in macros:
            if not macro.clean:
                continue
            if any(word in ('__FILE__', '__LINE__', '__COUNTER__', '__DATE__', '__TIME__') for word in (macro.row['tokens'] or [])):
                failures[id(macro)] = ['宏依赖展开位置或时间']
                continue
            groups[macro.job][macro.name] = macro
    for number, (snapshot, macros) in enumerate(sorted(groups.items())):
        path = Path(snapshot)
        original_job = load(Path(str(path)[:-len('.json.gz')] + '.job.json'))
        cache_prefix = config.get('cachePrefix', config['architecture'] + '-' + config['characterSet'])
        folder = args.output / 'inputs' / cache_prefix
        folder.mkdir(parents=True, exist_ok=True)
        active = dict(macros)
        for attempt in range(4):
            if not active:
                break
            source = folder / f'macros-{number}-{attempt}.cpp'
            lines = ['#include "' + original_job['source'].replace('\\', '/') + '"']
            by_line = {}
            for name in sorted(active):
                lines.append('#ifdef ' + name)
                lines.append('const auto __sdk_audit_' + name + ' = (' + name + ');')
                by_line[len(lines)] = name
                lines.append('#endif')
            source.write_text('\n'.join(lines) + '\n', encoding='utf-8')
            key = cache_prefix + f'/probe-{number}-{attempt}'
            result_path = runner.job(key, source, original_job['arguments'], [folder], mode='probe')
            result = load(result_path)
            errors = [item for item in result['diagnostics'] if item['severity'] >= 3]
            if valid_snapshot(result):
                types = Types()
                for declaration in declarations(result_path, types, runner.complete_string_values(result_path)):
                    name = declaration.name.removeprefix('__sdk_audit_')
                    if name in active:
                        values[id(active[name])] = declaration
                break
            bad = {}
            for error in errors:
                if Path(error['site']['file']).resolve() == source.resolve():
                    name = by_line.get(error['site']['line'])
                    if name:
                        bad.setdefault(name, []).append(error['message'])
            if not bad:
                for macro in active.values():
                    failures[id(macro)] = [item['message'] for item in errors] or [result.get('failure') or '宏求值翻译单元解析失败']
                break
            for name, errors in bad.items():
                failures[id(active[name])] = errors
                del active[name]
        for macro in active.values():
            if id(macro) not in values and id(macro) not in failures:
                failures[id(macro)] = ['未生成可求值声明或已达到宏求值重试上限']

    for projected, macros, row in pending:
        evaluated = [(macro, values.get(id(macro))) for macro in macros]
        good = [(macro, value) for macro, value in evaluated if value and value_key(value.row['value']) is not None]
        if not good:
            row['status'] = 'macro_value_unverified'
            row['unverified'] = ['SDK 宏存在，但无法对值和宽度作有效验证']
            row['macro_diagnostics'] = [message for macro in macros for message in failures.get(id(macro), [])]
            continue
        candidates_values = {(json.dumps(value_key(value.row['value']), sort_keys=True), value.type.size if value.type else -1) for macro, value in good}
        row['macro_evaluations'] = [value.brief() for macro, value in good]
        bodies = {tuple(macro.row['tokens'] or []) for macro in macros}
        if len(candidates_values) > 1 or len(good) != len(macros) or len(bodies) > 1:
            row['status'] = 'ambiguous_macro' if len(candidates_values) > 1 else 'macro_partially_verified'
            row['unverified'] = ['宏存在多个不同值/宽度或部分候选无法求值']
            continue
        reference = good[0][1]
        actual_value, expected_value = value_key(projected.row['value']), value_key(reference.row['value'])
        if actual_value is None:
            row['status'] = 'macro_value_unverified'
            row['unverified'] = ['导出常量无法求值']
            continue
        changes, unknown = compare_constant_value(projected, reference)
        row['differences'] += changes
        row['unverified'] += unknown
        if projected.type and reference.type and projected.type.size >= 0 and reference.type.size >= 0:
            if projected.type.size != reference.type.size:
                row['differences'].append(difference('constant_size', '', reference.type.size, projected.type.size))
        else:
            row['unverified'].append('宏或导出常量的字节宽度无法测量')
        row['status'] = 'different' if row['differences'] else 'macro_value_unverified' if row['unverified'] else 'matched_macro'
        if not projected.clean:
            row['status'] = 'parse_tainted'


def compare_run(args, runner, run_manifest):
    rules = EnumRules(runner.sdk / 'enums.json')
    totals = []
    for config in run_manifest['configurations']:
        key = config['architecture'] + '-' + config['characterSet']
        print('正在关联和比较：' + key, flush=True)
        rows, pending, sdk = compare_configuration(config, rules, args.output / 'indexes' / (key + '.pickle.gz'), runner)
        evaluate_macros(args, runner, config, pending)
        output = args.output / 'reports' / key
        output.mkdir(parents=True, exist_ok=True)
        with gzip.open(output / 'symbols.jsonl.gz', 'wt', encoding='utf-8') as stream:
            for row in rows:
                stream.write(json.dumps(row, ensure_ascii=False) + '\n')
        findings = [row for row in rows if row['status'] not in ('matched', 'matched_macro', 'matched_iid_uuid', 'matched_with_enum_rule', 'configured_enum_projection', 'cppwin32_support', 'managed_metadata_support')]
        with gzip.open(output / 'findings.jsonl.gz', 'wt', encoding='utf-8') as stream:
            for row in findings:
                stream.write(json.dumps(row, ensure_ascii=False) + '\n')
        with (output / 'findings.csv').open('w', encoding='utf-8-sig', newline='') as stream:
            writer = csv.writer(stream)
            writer.writerow(['状态', '符号类别', '符号名称', '差异类别', '导出文件', '导出行号', 'SDK 文件', 'SDK 行号'])
            for row in findings:
                site = row['projection']['site']
                candidates = row.get('sdk_candidates', [])
                source = candidates[0]['site'] if candidates else {}
                writer.writerow([row['status'], row['kind'], row['name'], ';'.join(sorted({item['kind'] for item in row['differences']})), site['file'], site['line'], source.get('file', ''), source.get('line', '')])
        with (output / 'differences.csv').open('w', encoding='utf-8-sig', newline='') as stream:
            writer = csv.writer(stream)
            writer.writerow(['配置', '符号类别', '符号名称', '检查项', '成员或参数位置', 'SDK 值', '导出值', '单位', 'SDK 文件', 'SDK 行号', '导出文件', '导出行号'])
            for row in findings:
                actual = row['projection']['site']
                references = row.get('sdk_candidates', [])
                reference = references[0]['site'] if references else {}
                for entry in row['differences']:
                    unit = 'bits' if entry['kind'] in ('offsetBits', 'bitWidth') else 'bytes' if 'size' in entry['kind'] or 'alignment' in entry['kind'] else ''
                    writer.writerow([key, row['kind'], row['name'], entry['kind'], entry['site'], json.dumps(entry['sdk'], ensure_ascii=False), json.dumps(entry['projection'], ensure_ascii=False), unit, reference.get('file', ''), reference.get('line', ''), actual['file'], actual['line']])
        diagnostics = {path: [item for item in load(path)['diagnostics'] if item['severity'] >= 3] for path in [config['projection'], *config['sdk'].values()]}
        dump(output / 'parse-errors.json', {key: value for key, value in diagnostics.items() if value})
        parse_failures = sum(bool(value) for value in diagnostics.values())
        summary = {'configuration': key, 'comparison_sha256': COMPARISON_HASH, 'symbols': len(rows), 'parameters': sum(len(entry['projection'].get('parameters', [])) for entry in rows), 'statuses': dict(collections.Counter(row['status'] for row in rows)), 'difference_kinds': dict(collections.Counter(item['kind'] for row in rows for item in row['differences'])), 'sdk_translation_units': len(config['sdk']), 'failed_or_tainted_translation_units': parse_failures, 'all_symbols_verified': not findings and parse_failures == 0}
        projection_marker = Path(config['projection'][:-len('.json.gz')] + '.signature')
        summary['projection_snapshot_signature'] = projection_marker.read_text() if projection_marker.exists() else None
        summary['exported_header_count'] = config.get('exported_header_count', len(list(args.headers.rglob('*.h'))))
        categories = {
            'layout': {'record_size', 'record_alignment', 'type_size', 'type_alignment', 'offsetBits', 'bitWidth', 'iid_size'},
            'signature_or_type_identity': {'type_structure', 'calling_convention', 'parameter_count', 'variadic', 'base_classes', 'method_order', 'method_virtual', 'method_pure'},
            'spelling': {'name', 'parameter_name', 'type_spelling'},
            'constant_value': {'constant_value', 'iid_uuid_value', 'uuid_attribute'},
            'macro_expression_width': {'constant_size'},
            'numeric_representation': {'constant_numeric_representation'},
            'cxx_exception_specification': {'exception'},
        }
        summary['symbols_by_difference_category'] = {name: sum(row['status'] == 'different' and any(item['kind'] in kinds for item in row['differences']) for row in rows) for name, kinds in categories.items()}
        summary['reference_scopes'] = dict(collections.Counter(row['sdk_candidates'][0].get('source_scope', '') for row in rows if row.get('sdk_candidates')))
        dump(output / 'summary.json', summary)
        totals.append(summary)
        print(json.dumps(summary, ensure_ascii=False), flush=True)
    dump(args.output / 'summary.json', {'configurations': totals, 'all_symbols_verified': all(entry['all_symbols_verified'] for entry in totals)})
    if args.fail_on_difference and not all(entry['all_symbols_verified'] for entry in totals):
        raise SystemExit(2)
