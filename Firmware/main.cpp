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

class Serial {
    HANDLE h_ = INVALID_HANDLE_VALUE;
public:
    explicit Serial(const std::wstring& port) {
        h_ = CreateFileW((L"\\\\.\\" + port).c_str(), GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);
        if (h_ == INVALID_HANDLE_VALUE) return;

        DCB dcb{ sizeof(dcb) };
        GetCommState(h_, &dcb);
        dcb.BaudRate = BAUD;
        dcb.ByteSize = 8;
        SetCommState(h_, &dcb);
    }

// thanks for gemini for the fix down here ngl you are my goat
    ~Serial() { if (h_ != INVALID_HANDLE_VALUE) CloseHandle(h_); }

    void send(const std::vector<uint32_t>& pixels) {
        if (h_ == INVALID_HANDLE_VALUE) return;

        std::vector<BYTE> buf = { 'A', 'd', 'a', BYTE((LED_COUNT - 1) >> 8), BYTE(LED_COUNT - 1) };
        buf.push_back(buf[3] ^ buf[4] ^ 0x55);

        for (UINT i = 0; i < LED_COUNT; ++i) {
            uint32_t color = pixels[i];
            buf.push_back(BYTE(color));
            buf.push_back(BYTE(color >> 8));
            buf.push_back(BYTE(color >> 16));
        }

        DWORD written;
        WriteFile(h_, buf.data(), static_cast<DWORD>(buf.size()), &written, nullptr);
    }
};