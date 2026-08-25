#include <windows.h>
#include <d3d11.h>
#include <dxgi1_2.h>
#include <d3dcompiler.h>
#include <wrl/client.h>
#include <array>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

using Microsoft::WRL::ComPtr;

constexpr UINT LED_COUNT = 90;
constexpr UINT BAUD = 115200;

static void Check(HRESULT hr, const char* what) {
    if (FAILED(hr)) { std::cerr << what << " failed: 0x" << std::hex << (unsigned)hr << '\n'; std::exit(1); }
}

class Serial {
public:
    explicit Serial(const std::wstring& port) {
        std::wstring path = L"\\\\.\\" + port;
        handle_ = CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);
        if (handle_ == INVALID_HANDLE_VALUE) { std::wcerr << L"Cannot open " << path << L" (is the port correct?)\n"; std::exit(1); }
        DCB dcb{}; dcb.DCBlength = sizeof dcb;
        Check(GetCommState(handle_, &dcb) ? S_OK : HRESULT_FROM_WIN32(GetLastError()), "GetCommState");
        dcb.BaudRate = BAUD; dcb.ByteSize = 8; dcb.Parity = NOPARITY; dcb.StopBits = ONESTOPBIT;
        dcb.fBinary = TRUE; dcb.fDtrControl = DTR_CONTROL_ENABLE; dcb.fRtsControl = RTS_CONTROL_ENABLE;
        Check(SetCommState(handle_, &dcb) ? S_OK : HRESULT_FROM_WIN32(GetLastError()), "SetCommState");
        COMMTIMEOUTS t{}; t.WriteTotalTimeoutConstant = 25; SetCommTimeouts(handle_, &t);
    }

// thanks for gemini for the fix down here ngl you are my goat (again)
    ~Serial() { black(); if (handle_ != INVALID_HANDLE_VALUE) CloseHandle(handle_); }

    void send(const UINT* pixels) {

        std::array<BYTE, 6 + LED_COUNT * 3> p{};
        p[0] = 'A'; p[1] = 'd'; p[2] = 'a';
        const UINT n = LED_COUNT - 1; p[3] = BYTE(n >> 8); p[4] = BYTE(n); p[5] = p[3] ^ p[4] ^ 0x55;
        for (UINT i = 0; i < LED_COUNT; ++i) {
            p[6 + 3*i] = BYTE(pixels[i]); p[7 + 3*i] = BYTE(pixels[i] >> 8); p[8 + 3*i] = BYTE(pixels[i] >> 16);
        }
        DWORD written{};
        const BOOL ok = WriteFile(handle_, p.data(), (DWORD)p.size(), &written, nullptr);
        Check(ok && written == p.size() ? S_OK : (ok ? E_FAIL : HRESULT_FROM_WIN32(GetLastError())), "WriteFile");
        
    }

    void black() { UINT p[LED_COUNT]{}; if (handle_ != INVALID_HANDLE_VALUE) send(p); }
private: HANDLE handle_ = INVALID_HANDLE_VALUE;
};

int wmain(int argc, wchar_t** argv) {
    const std::wstring port = argc > 1 ? argv[1] : L"COM3";
    Serial serial(port);

    ComPtr<IDXGIFactory1> factory; Check(CreateDXGIFactory1(IID_PPV_ARGS(&factory)), "CreateDXGIFactory1");
    ComPtr<IDXGIAdapter1> adapter; ComPtr<IDXGIOutput> output;

    for (UINT ai = 0; !output; ++ai) { ComPtr<IDXGIAdapter1> a; if (factory->EnumAdapters1(ai, &a) == DXGI_ERROR_NOT_FOUND) break;
        for (UINT oi = 0;; ++oi) { ComPtr<IDXGIOutput> o; if (a->EnumOutputs(oi, &o) == DXGI_ERROR_NOT_FOUND) break;
            DXGI_OUTPUT_DESC d{}; o->GetDesc(&d); if (d.AttachedToDesktop && d.DesktopCoordinates.left <= 0 && d.DesktopCoordinates.right > 0 && d.DesktopCoordinates.top <= 0 && d.DesktopCoordinates.bottom > 0) { adapter = a; output = o; break; }
        }
    }

    if (!output) { std::cerr << "No active monitor found.\n"; return 1; }

    // gemini helped me with that list
    ComPtr<ID3D11Device> device; ComPtr<ID3D11DeviceContext> context; D3D_FEATURE_LEVEL fl;
    Check(D3D11CreateDevice(adapter.Get(), D3D_DRIVER_TYPE_UNKNOWN, nullptr, D3D11_CREATE_DEVICE_BGRA_SUPPORT, nullptr, 0, D3D11_SDK_VERSION, &device, &fl, &context), "D3D11CreateDevice");
    ComPtr<IDXGIOutput1> output1; Check(output.As(&output1), "IDXGIOutput1");
    ComPtr<IDXGIOutputDuplication> duplication; Check(output1->DuplicateOutput(device.Get(), &duplication), "DuplicateOutput");

}