import argparse
import os
from pathlib import Path
from types import SimpleNamespace
import unittest

from compare import EnumRules, compare_configuration, evaluate_macros
from run import Runner, arguments, dump


class DeepAuditTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.args = SimpleNamespace(metadata=Path(os.environ.get('DEEP_AUDIT_METADATA', 'L:/Open_Code/win32metadata')), headers=Path(os.environ.get('DEEP_AUDIT_HEADERS', 'Z:/ms/win32')), output=Path(os.environ.get('DEEP_AUDIT_OUTPUT', 'Z:/sdk-deep-audit-20261105')))
        cls.runner = Runner(cls.args)

    def run_case(self, sdk_text, projected_text, enum_items=()):
        folder = self.args.output / 'selftests' / self._testMethodName
        original, projected = folder / 'sdk', folder / 'projection'
        original.mkdir(parents=True, exist_ok=True)
        projected.mkdir(parents=True, exist_ok=True)
        sdk_source, projected_source = original / 'input.cpp', projected / 'input.cpp'
        sdk_source.write_text(sdk_text, encoding='utf-8')
        projected_source.write_text(projected_text, encoding='utf-8')
        enum_file = folder / 'enums.json'
        dump(enum_file, {'items': list(enum_items)})
        compiler_args = arguments(self.args.metadata, 'x86', 'Ansi', True)
        a = self.runner.job('selftests/' + self._testMethodName + '/sdk', sdk_source, compiler_args, [original])
        b = self.runner.job('selftests/' + self._testMethodName + '/projection', projected_source, compiler_args, [projected])
        config = {'architecture': 'x86', 'characterSet': 'Ansi', 'projection': b, 'sdk': {'fixture': a}, 'cachePrefix': 'selftests/' + self._testMethodName}
        rows, pending, _ = compare_configuration(config, EnumRules(enum_file), runner=self.runner)
        evaluate_macros(self.args, self.runner, config, pending)
        return rows

    @staticmethod
    def kinds(rows):
        return {difference['kind'] for row in rows for difference in row['differences']}

    def test_equal(self):
        code = 'typedef unsigned long WORD; struct Record { WORD field; char bytes[3]; }; extern "C" int __stdcall Function(Record* value);'
        rows = self.run_case(code, code)
        self.assertTrue(all(row['status'] == 'matched' for row in rows), rows)

    def test_pointer_depth(self):
        rows = self.run_case('extern "C" int __stdcall Function(int* value);', 'extern "C" int __stdcall Function(int** value);')
        self.assertIn('type_structure', self.kinds(rows))

    def test_reference_parameter_storage_width(self):
        common = 'struct Large { int fields[4]; }; '
        rows = self.run_case(common + 'int Function(const Large& value);', common + 'int Function(const Large* value);')
        self.assertNotIn('type_size', self.kinds(rows))
        self.assertIn('type_structure', self.kinds(rows))

    def test_reference_member_storage_width(self):
        common = 'struct Large { int fields[4]; }; '
        rows = self.run_case(common + 'struct Holder { const Large& Value; };', common + 'struct Holder { const Large* Value; };')
        self.assertNotIn('type_size', self.kinds(rows))

    def test_pack_offset_and_size(self):
        body = 'struct Record { char first; long second; };'
        rows = self.run_case(body, '#pragma pack(push,1)\n' + body + '\n#pragma pack(pop)')
        self.assertTrue({'record_size', 'offsetBits'} <= self.kinds(rows))

    def test_typedef_spelling(self):
        sdk = 'typedef unsigned long DWORD; struct Record { DWORD field; };'
        rows = self.run_case(sdk, 'typedef unsigned long DWORD; struct Record { unsigned long field; };')
        self.assertIn('type_spelling', self.kinds(rows))

    def test_macro_type_spelling(self):
        common = 'struct NATIVE_RECORD { int Value; };\n#define PUBLIC_RECORD NATIVE_RECORD\n'
        rows = self.run_case(common + 'struct Record { PUBLIC_RECORD* field; };', common + 'struct Record { NATIVE_RECORD* field; };')
        self.assertIn('type_spelling', self.kinds(rows))

    def test_callback_and_array(self):
        rows = self.run_case('typedef int (__stdcall *Callback)(long); struct Record { char bytes[3]; Callback callback; };', 'typedef int (__cdecl *Callback)(short); struct Record { char bytes[4]; Callback callback; };')
        self.assertIn('type_structure', self.kinds(rows))

    def test_calling_convention_and_parameter_name(self):
        rows = self.run_case('int __stdcall Function(int value);', 'int __cdecl Function(int other);')
        self.assertTrue({'calling_convention', 'parameter_name'} <= self.kinds(rows))

    def test_enum_rule(self):
        rows = self.run_case('struct Record { unsigned long Flags; };', 'enum Flags { One=1 }; struct Record { Flags Flags; };', [{'name': 'Flags', 'uses': [{'struct': 'Record', 'field': 'Flags'}]}])
        row = next(row for row in rows if row['kind'] == 'field')
        self.assertEqual(row['status'], 'matched_with_enum_rule', row)

    def test_enum_rule_preserves_width(self):
        rows = self.run_case('struct Record { unsigned long Flags; };', 'enum Flags : unsigned long long { One=1 }; struct Record { Flags Flags; };', [{'name': 'Flags', 'uses': [{'struct': 'Record', 'field': 'Flags'}]}])
        self.assertIn('type_size', self.kinds(rows))

    def test_enum_rule_with_struct_tag(self):
        rows = self.run_case('typedef struct _Record { unsigned long Flags; } Record;', 'enum Flags { One=1 }; typedef struct _Record { Flags Flags; } Record;', [{'name': 'Flags', 'uses': [{'struct': 'Record', 'field': 'Flags'}]}])
        row = next(row for row in rows if row['kind'] == 'field')
        self.assertEqual(row['status'], 'matched_with_enum_rule', row)

    def test_enum_rule_add_uses_to(self):
        rows = self.run_case('struct Record { unsigned long Flags; };', 'enum Flags { One=1 }; struct Record { Flags Flags; };', [{'name': 'Flags', 'uses': []}, {'addUsesTo': 'Flags', 'uses': [{'struct': 'Record', 'field': 'Flags'}]}])
        row = next(row for row in rows if row['kind'] == 'field')
        self.assertEqual(row['status'], 'matched_with_enum_rule', row)

    def test_enum_rule_preserves_pointee_constness(self):
        rows = self.run_case('struct Record { const unsigned long* Flags; };', 'enum Flags { One=1 }; struct Record { Flags* Flags; };', [{'name': 'Flags', 'uses': [{'struct': 'Record', 'field': 'Flags'}]}])
        self.assertIn('type_structure', self.kinds(rows))

    def test_macro_value(self):
        rows = self.run_case('#define CONSTANT 7\n', 'inline constexpr int CONSTANT = 8;')
        self.assertIn('constant_value', self.kinds(rows))

    def test_macro_equal(self):
        rows = self.run_case('#define CONSTANT (2+5)\n', 'inline constexpr int CONSTANT = 7;')
        self.assertEqual(next(row for row in rows if row['kind'] == 'variable')['status'], 'matched_macro')

    def test_wide_string_after_first_character(self):
        rows = self.run_case('#define CONSTANT L"ab"\n', 'inline constexpr const wchar_t* CONSTANT = L"ac";')
        self.assertIn('constant_value', self.kinds(rows))

    def test_string_after_embedded_zero(self):
        rows = self.run_case('#define CONSTANT "a\\0b"\n', 'inline constexpr const char* CONSTANT = "a\\0c";')
        self.assertIn('constant_value', self.kinds(rows))

    def test_cache_names_containing_dots(self):
        folder = self.args.output / 'selftests' / self._testMethodName
        folder.mkdir(parents=True, exist_ok=True)
        source = folder / 'input.cpp'
        source.write_text('struct Value { int Field; };')
        compiler_args = arguments(self.args.metadata, 'x86', 'Ansi', True)
        a = self.runner.job('selftests/Group.One', source, compiler_args, [folder])
        b = self.runner.job('selftests/Group.Two', source, compiler_args, [folder])
        self.assertNotEqual(a, b)

    def test_same_global_interface_in_distinct_headers(self):
        folder = self.args.output / 'selftests' / self._testMethodName
        folder.mkdir(parents=True, exist_ok=True)
        a, b, projected = folder / 'first.cpp', folder / 'second.cpp', folder / 'projection.cpp'
        a.write_text('struct __declspec(uuid("11111111-1111-1111-1111-111111111111")) Interface { virtual int First()=0; };')
        b.write_text('struct __declspec(uuid("22222222-2222-2222-2222-222222222222")) Interface { virtual int Second()=0; };')
        projected.write_text(b.read_text())
        compiler_args = arguments(self.args.metadata, 'x86', 'Ansi', True)
        snapshots = [self.runner.job('selftests/' + self._testMethodName + '/' + label, source, compiler_args, [folder]) for label, source in [('first', a), ('second', b), ('projection', projected)]]
        rules = folder / 'enums.json'
        dump(rules, {'items': []})
        rows, _, _ = compare_configuration({'projection': snapshots[2], 'sdk': {'first': snapshots[0], 'second': snapshots[1]}}, EnumRules(rules), runner=self.runner)
        self.assertTrue(all(row['status'] == 'matched' for row in rows), rows)

    def test_iid_attribute(self):
        common = 'struct GUID { unsigned long Data1; unsigned short Data2, Data3; unsigned char Data4[8]; }; typedef GUID IID; '
        interface = 'struct __declspec(uuid("12345678-1234-5678-90AB-CDEF01234567")) ITest { virtual int Method()=0; };'
        value = 'inline constexpr IID IID_ITest = {0x12345678,0x1234,0x5678,{0x90,0xAB,0xCD,0xEF,1,0x23,0x45,0x67}};'
        rows = self.run_case(common + interface, common + interface + value)
        self.assertEqual(next(row for row in rows if row['name'] == 'IID_ITest')['status'], 'matched_iid_uuid')

    def test_iid_wrong_byte(self):
        common = 'struct GUID { unsigned long Data1; unsigned short Data2, Data3; unsigned char Data4[8]; }; typedef GUID IID; '
        interface = 'struct __declspec(uuid("12345678-1234-5678-90AB-CDEF01234567")) ITest { virtual int Method()=0; };'
        value = 'inline constexpr IID IID_ITest = {0x12345678,0x1234,0x5678,{0x90,0xAB,0xCD,0xEF,1,0x23,0x45,0x68}};'
        self.assertIn('iid_uuid_value', self.kinds(self.run_case(common + interface, common + interface + value)))

    def test_ambiguous_names(self):
        rows = self.run_case('namespace A { struct Record { int field; }; } namespace B { struct Record { long long field; }; }', 'struct Record { int field; };')
        self.assertEqual(next(row for row in rows if row['kind'] == 'record')['status'], 'ambiguous')

    def test_function_overload(self):
        rows = self.run_case('int Function(int value); long Function(long value);', 'int Function(int value);')
        self.assertEqual(next(row for row in rows if row['kind'] == 'function')['status'], 'matched')

    def test_virtual_method_order(self):
        rows = self.run_case('struct Interface { virtual int First()=0; virtual int Second()=0; };', 'struct Interface { virtual int Second()=0; virtual int First()=0; };')
        self.assertIn('method_order', self.kinds(rows))

    def test_parse_error_never_passes(self):
        rows = self.run_case('struct Record { int field; };', '#error injected_failure\nstruct Record { int field; };')
        self.assertTrue(all(row['status'] not in ('matched', 'matched_macro', 'matched_iid_uuid') for row in rows))


if __name__ == '__main__':
    unittest.main()
