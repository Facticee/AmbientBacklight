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

struct alignas(16) Settings {
    UINT width, height, borderDepth, samplesAcross, samplesDeep;
    float saturationBoost, brightness;
};

struct Readback {
    ComPtr<ID3D11Buffer> buffer;
    ComPtr<ID3D11Query> query;
    bool pending = false;
};

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
private:
    HANDLE handle_ = INVALID_HANDLE_VALUE;
};

int wmain(int argc, wchar_t** argv) {
    const std::wstring port = argc > 1 ? argv[1] : L"COM3";
    Serial serial(port);

    ComPtr<IDXGIFactory1> factory; Check(CreateDXGIFactory1(IID_PPV_ARGS(&factory)), "CreateDXGIFactory1");
    ComPtr<IDXGIAdapter1> adapter; ComPtr<IDXGIOutput> output;

    for (UINT ai = 0; !output; ++ai) {
        ComPtr<IDXGIAdapter1> a;
        if (factory->EnumAdapters1(ai, &a) == DXGI_ERROR_NOT_FOUND) break;
        for (UINT oi = 0;; ++oi) {
            ComPtr<IDXGIOutput> o;
            if (a->EnumOutputs(oi, &o) == DXGI_ERROR_NOT_FOUND) break;
            DXGI_OUTPUT_DESC d{};
            o->GetDesc(&d);
            if (d.AttachedToDesktop && d.DesktopCoordinates.left <= 0 && d.DesktopCoordinates.right > 0 && d.DesktopCoordinates.top <= 0 && d.DesktopCoordinates.bottom > 0) {
                adapter = a; output = o; break;
            }
        }
    }

    if (!output) { std::cerr << "No active monitor found.\n"; return 1; }

    ComPtr<ID3D11Device> device; ComPtr<ID3D11DeviceContext> context; D3D_FEATURE_LEVEL fl;
    Check(D3D11CreateDevice(adapter.Get(), D3D_DRIVER_TYPE_UNKNOWN, nullptr, D3D11_CREATE_DEVICE_BGRA_SUPPORT, nullptr, 0, D3D11_SDK_VERSION, &device, &fl, &context), "D3D11CreateDevice");
    ComPtr<IDXGIOutput1> output1; Check(output.As(&output1), "IDXGIOutput1");
    ComPtr<IDXGIOutputDuplication> duplication; Check(output1->DuplicateOutput(device.Get(), &duplication), "DuplicateOutput");

    wchar_t pathBuf[MAX_PATH];
    GetModuleFileNameW(NULL, pathBuf, MAX_PATH);
    std::filesystem::path shaderPath = std::filesystem::path(pathBuf).parent_path() / L"shader.hlsl";

    ComPtr<ID3DBlob> code, errors;
    HRESULT hr = D3DCompileFromFile(shaderPath.c_str(), NULL, D3D_COMPILE_STANDARD_FILE_INCLUDE, "CSMain", "cs_5_0", 0, 0, &code, &errors);
    if (FAILED(hr)) {
        if (errors) std::cout << (char*)errors->GetBufferPointer() << "\n";
        Check(hr, "Shader kompilierung fehlgeschlagen");
    }

    ComPtr<ID3D11ComputeShader> shader;
    Check(device->CreateComputeShader(code->GetBufferPointer(), code->GetBufferSize(), NULL, &shader), "CreateComputeShader");

    D3D11_BUFFER_DESC bd = {};
    bd.ByteWidth = LED_COUNT * sizeof(UINT);
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.BindFlags = D3D11_BIND_UNORDERED_ACCESS;
    bd.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    bd.StructureByteStride = sizeof(UINT);

    ComPtr<ID3D11Buffer> resultBuffer;
    Check(device->CreateBuffer(&bd, NULL, &resultBuffer), "Result Buffer");

    D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
    uavDesc.Format = DXGI_FORMAT_UNKNOWN;
    uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
    uavDesc.Buffer.NumElements = LED_COUNT;

    ComPtr<ID3D11UnorderedAccessView> uav;
    Check(device->CreateUnorderedAccessView(resultBuffer.Get(), &uavDesc, &uav), "Create UAV");

    Settings s{ 0, 0, 144, 12, 10, 1.0f, 1.0f };
    D3D11_BUFFER_DESC cb{ sizeof(Settings), D3D11_USAGE_DYNAMIC, D3D11_BIND_CONSTANT_BUFFER, D3D11_CPU_ACCESS_WRITE };
    ComPtr<ID3D11Buffer> constants; Check(device->CreateBuffer(&cb, nullptr, &constants), "Create constants");

    Readback readbacks[3]{};
    for (auto& r : readbacks) {
        D3D11_BUFFER_DESC t{ LED_COUNT * sizeof(UINT), D3D11_USAGE_STAGING, 0, D3D11_CPU_ACCESS_READ };
        Check(device->CreateBuffer(&t, nullptr, &r.buffer), "Staging buffer");
        D3D11_QUERY_DESC q{ D3D11_QUERY_EVENT, 0 }; Check(device->CreateQuery(&q, &r.query), "Query");
    }

    std::cout << "Ambilight running on " << std::string(port.begin(), port.end()) << " Ctrl + C to stop \n";

    auto sendCompleted = [&] {
        for (auto& r : readbacks) {
            if (r.pending && context->GetData(r.query.Get(), nullptr, 0, 0) == S_OK) {
                D3D11_MAPPED_SUBRESOURCE m{};
                if (SUCCEEDED(context->Map(r.buffer.Get(), 0, D3D11_MAP_READ, 0, &m))) {
                    serial.send((const UINT*)m.pData);
                    context->Unmap(r.buffer.Get(), 0);
                    r.pending = false;
                }
            }
        }
    };

    while (true) {
        DXGI_OUTDUPL_FRAME_INFO info {}; ComPtr<IDXGIResource> resource;
        HRESULT hr = duplication->AcquireNextFrame(16, &info, &resource);
        if (hr == DXGI_ERROR_WAIT_TIMEOUT) continue;
        if (hr == DXGI_ERROR_ACCESS_LOST) break;
        Check(hr, "AcquireNextFrame");

        ComPtr<ID3D11Texture2D> desktop; Check(resource.As(&desktop), "Desktop texture");
        D3D11_TEXTURE2D_DESC desc{}; desktop->GetDesc(&desc);
        s.width = desc.Width; s.height = desc.Height;

        ComPtr<ID3D11ShaderResourceView> srv; Check(device->CreateShaderResourceView(desktop.Get(), nullptr, &srv), "Create desktop SRV");

        D3D11_MAPPED_SUBRESOURCE mapped{};
        Check(context->Map(constants.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped), "Map constants");
        *(Settings*)mapped.pData = s;
        context->Unmap(constants.Get(), 0);

        ID3D11ShaderResourceView* srvArray[] = { srv.Get() };
        ID3D11UnorderedAccessView* uavArray[] = { uav.Get() };
        ID3D11Buffer* cbArray[] = { constants.Get() };

        context->CSSetShader(shader.Get(), nullptr, 0);
        context->CSSetShaderResources(0, 1, srvArray);
        context->CSSetUnorderedAccessViews(0, 1, uavArray, nullptr);
        context->CSSetConstantBuffers(0, 1, cbArray);
        context->Dispatch(1, 1, 1);

        ID3D11ShaderResourceView* nullSRV = nullptr;
        ID3D11UnorderedAccessView* nullUAV = nullptr;
        context->CSSetShaderResources(0, 1, &nullSRV);
        context->CSSetUnorderedAccessViews(0, 1, &nullUAV, nullptr);
        duplication->ReleaseFrame();

        sendCompleted();
        for (auto& r : readbacks) {
            if (!r.pending) {
                context->CopyResource(r.buffer.Get(), resultBuffer.Get());
                context->End(r.query.Get());
                r.pending = true;
                break;
            }
        }
        sendCompleted();
    }
    return 0;
}