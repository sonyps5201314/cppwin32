#include "win32/Windows.Win32.Test.ImportFirst.h"
#include "win32/Windows.Win32.Test.ImportSecond.h"

auto first_import_symbol = &win32::Windows::Win32::Test::ImportFirst::LibraryProbe;
auto second_import_symbol = &win32::Windows::Win32::Test::ImportSecond::user32_LibraryProbe;
