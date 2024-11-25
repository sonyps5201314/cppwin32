#include "pch.h"
#include "D3D12HelloWindow.h"

using namespace win32;
using namespace win32::Windows::Win32;
using namespace win32::Windows::Win32::Foundation;
using namespace win32::Windows::Win32::System::Services;
using namespace win32::Windows::Win32::System::LibraryLoader;
using namespace win32::Windows::Win32::UI::WindowsAndMessaging;

LRESULT __stdcall WindowProc(HWND hwnd, uint32_t message, WPARAM wParam, LPARAM lParam);

int run(DXSample* sample);

int main()
{
    D3D12HelloWindow sample(1280, 720, L"D3D12 Hello Window");
    run(&sample);
}

int run(DXSample* sample)
{
    HINSTANCE hInstance{ GetModuleHandleW(nullptr) };

    WNDCLASSEXW windowClass{};
    windowClass.cbSize = sizeof(WNDCLASSEXW);
    windowClass.style = WNDCLASS_STYLES::CS_HREDRAW | WNDCLASS_STYLES::CS_VREDRAW;
    windowClass.lpfnWndProc = WindowProc;
    windowClass.hInstance = hInstance;
    windowClass.hCursor = LoadCursorW(HINSTANCE{}, (uint16_t*)(IDC_ARROW)); // Ugly cast tracked by metadata bug https://github.com/microsoft/win32metadata/issues/69
    windowClass.lpszClassName = (PWSTR)(L"DXSampleClass");

    RegisterClassExW(&windowClass);

    RECT rect{ 0, 0, sample->Width(), sample->Height()};
    AdjustWindowRect(&rect, WINDOW_STYLE::WS_OVERLAPPEDWINDOW, BOOL{0});

    HWND hwnd = CreateWindowExW(
        (WINDOW_EX_STYLE)0,
        windowClass.lpszClassName,
        (uint16_t*)(sample->Title().c_str()),
        WINDOW_STYLE::WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        640,
        480,
        HWND{},
        HMENU{},
        hInstance,
        sample);

    sample->OnInit();

    ShowWindow(hwnd, SHOW_WINDOW_CMD::SW_SHOWDEFAULT);

    MSG msg;
    do
    {
        if (PeekMessageW(&msg, HWND{}, 0, 0, PEEK_MESSAGE_REMOVE_TYPE::PM_REMOVE).Value != 0)
        {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    } while (msg.message != WM_QUIT);

    sample->OnDestroy();

    return 0;
}

LRESULT __stdcall WindowProc(HWND hwnd, uint32_t message, WPARAM wParam, LPARAM lParam)
{
    auto sample = reinterpret_cast<DXSample*>(GetWindowLongW(hwnd, WINDOW_LONG_PTR_INDEX::GWLP_USERDATA));

    switch (message)
    {
    case WM_CREATE:
    {
        auto create_struct = reinterpret_cast<CREATESTRUCTW*>(lParam.Value);
        SetWindowLongW(
            hwnd,
            WINDOW_LONG_PTR_INDEX::GWLP_USERDATA,
            reinterpret_cast<intptr_t>(create_struct->lpCreateParams));
        return LRESULT{ 0 };
    }

    case WM_PAINT:
        if (sample)
        {
            sample->OnUpdate();
            sample->OnRender();
        }
        return LRESULT{ 0 };

    case WM_DESTROY:
        PostQuitMessage(0);
        return LRESULT{ 0 };
    }
    return DefWindowProcW(hwnd, message, wParam, lParam);
}
