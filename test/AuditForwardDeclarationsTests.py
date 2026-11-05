from pathlib import Path
import tempfile
import unittest

from AuditForwardDeclarations import audit


class ForwardDeclarationAuditTests(unittest.TestCase):
    def check_headers(self, source):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / 'impl').mkdir()
            (root / 'impl' / 'Test.0.h').write_text('#ifndef TEST_H\n#define TEST_H\n' + source + '\n#endif\n', encoding='utf-8')
            return audit(root)

    def test_overlapping_architectures(self):
        result = self.check_headers('''#if defined(_M_AMD64) || defined(_M_ARM64)
typedef struct _RECORD RECORD;
#endif
#if defined(_M_AMD64)
typedef struct _RECORD RECORD;
#endif''')
        self.assertEqual(result['duplicate_pairs'], 1)
        self.assertEqual(result['duplicates'][0]['architectures'], ['_M_AMD64'])

    def test_separate_architectures_conditions_and_namespaces(self):
        result = self.check_headers('''#if defined(_M_AMD64) || defined(_M_ARM64)
typedef struct _RECORD RECORD;
#else
typedef struct _RECORD RECORD;
#endif
#ifdef UNICODE
typedef struct _TEXT TEXT;
#else
typedef struct _TEXT TEXT;
#endif
WIN32_EXPORT namespace win32::First
{
struct Shared;
}
WIN32_EXPORT namespace win32::Second
{
struct Shared;
}''')
        self.assertEqual(result['duplicate_pairs'], 0)
        self.assertEqual(result['declarations'], 6)

    def test_repeated_pointer_alias_and_callback(self):
        result = self.check_headers('''typedef struct _RECORD RECORD;
typedef RECORD* PRECORD;
typedef int (__stdcall * Callback) (PRECORD value);
typedef RECORD* PRECORD;
typedef int (__stdcall * Callback) (PRECORD value);''')
        self.assertEqual(result['duplicate_pairs'], 2)

    def test_uuid_forward_declarations(self):
        result = self.check_headers('''struct __declspec(uuid("00000000-0000-0000-C000-000000000046")) IUnknown;
struct __declspec(uuid("00000000-0000-0000-C000-000000000046")) IUnknown;''')
        self.assertEqual(result['duplicate_declarations'], 1)


if __name__ == '__main__':
    unittest.main()
