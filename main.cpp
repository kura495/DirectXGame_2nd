#include<Windows.h>
#include<cstdint>
#include<string>
#include<format>
#include<d3d12.h>
#include<dxgi1_6.h>
#include<cassert>
#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")

// ウィンドウプロシージャ
LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
	switch (msg) {
		// メッセージに応じてゲーム固有の処理を行う
	case WM_DESTROY:
		// ウィンドウが破棄された
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc(hwnd, msg, wparam, lparam);
}
void Log(const std::string& message) {
	OutputDebugStringA(message.c_str());
}
std::wstring ConvertString(const std::string& str)
{
	if (str.empty())
	{
		return std::wstring();
	}

	auto sizeNeeded = MultiByteToWideChar(CP_UTF8, 0, reinterpret_cast<const char*>(&str[0]), static_cast<int>(str.size()), NULL, 0);
	if (sizeNeeded == 0)
	{
		return std::wstring();
	}
	std::wstring result(sizeNeeded, 0);
	MultiByteToWideChar(CP_UTF8, 0, reinterpret_cast<const char*>(&str[0]), static_cast<int>(str.size()), &result[0], sizeNeeded);
	return result;
}
std::string ConvertString(const std::wstring& str)
{
	if (str.empty())
	{
		return std::string();
	}

	auto sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, str.data(), static_cast<int>(str.size()), NULL, 0, NULL, NULL);
	if (sizeNeeded == 0)
	{
		return std::string();
	}
	std::string result(sizeNeeded, 0);
	WideCharToMultiByte(CP_UTF8, 0, str.data(), static_cast<int>(str.size()), result.data(), sizeNeeded, NULL, NULL);
	return result;
}

// Windowsアプリでのエントリーポイント(main関数)
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {

#pragma region WindowOption
	WNDCLASS wc{};
	//ウィンドウプロシージャ
	wc.lpfnWndProc = WindowProc;
	//ウィンドウクラス名
	wc.lpszClassName = L"CG2WindowClass";
	//インスタンスハンドル
	wc.hInstance = GetModuleHandle(nullptr);
	//カーソル
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
	//ウィンドウクラスを登録
	RegisterClass(&wc);
	const int kClientWidth = 1280;
	const int kClientHeight = 720;
	RECT wrc = { 0,0,kClientWidth ,kClientHeight };
	AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);
#pragma endregion
#pragma region WindowMake
	HWND hwnd = CreateWindow(
		wc.lpszClassName,	//利用するクラス名
		L"CG2",				//タイトルバーの文字
		WS_OVERLAPPEDWINDOW,//ウィンドウスタイル
		CW_USEDEFAULT,		//表示X座標
		CW_USEDEFAULT,		//表示Y座標
		wrc.right - wrc.left,//ウィンドウ幅
		wrc.bottom - wrc.top,//ウィンドウ幅
		nullptr,			 //親ウィンドウハンドル
		nullptr,			 //メニューハンドル
		wc.hInstance,		 //インスタンスハンドル
		nullptr);			 //オプション
#pragma endregion

	ShowWindow(hwnd, SW_SHOW);

#pragma region DXGIFactory
	//DXGIファクトリーを作成
	IDXGIFactory7* dxgiFactory = nullptr;
	//HRESULTはウィンドウズ系のエラーコードで
	//関数が成功したかどうかassertで確認
	HRESULT hr = CreateDXGIFactory(IID_PPV_ARGS(&dxgiFactory));
	assert(SUCCEEDED(hr));

	//使用するアダプタを決定
	IDXGIAdapter4* useAdapter = nullptr;
	//良い順にアダプタを積む
	for (UINT i = 0; dxgiFactory->EnumAdapterByGpuPreference(i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&useAdapter)) != DXGI_ERROR_NOT_FOUND; ++i) {
		//アダプタ情報を取得
		DXGI_ADAPTER_DESC3 adapterDesc{};
		hr = useAdapter->GetDesc3(&adapterDesc);
		assert(SUCCEEDED(hr));//取得できないのは一大事
		//ソフトウェアアダプタ出なければ採用
		if (!(adapterDesc.Flags & DXGI_ADAPTER_FLAG3_SOFTWARE)) {
			//採用したアダプタ情報をログに出力wStringなので注意
			Log(ConvertString(std::format(L"Use Adapter:{}\n", adapterDesc.Description)));
			break;
		}
		useAdapter = nullptr;//ソフトウェアアダプタは見なかったことにする
	}
	assert(useAdapter != nullptr);
	//D3D12Deviceの作成
	ID3D12Device* device = nullptr;
	//機能レベルとログ出力用の文字列
	D3D_FEATURE_LEVEL featureLevels[] = {
		D3D_FEATURE_LEVEL_12_2, D3D_FEATURE_LEVEL_12_1, D3D_FEATURE_LEVEL_12_0, 
	};
	const char* featureLevelStrings[] = { "12.2","12.1","12.0" };
	//高い順に生成できるか試していく
	for (size_t i = 0; i < _countof(featureLevels); ++i) {
		//採用したアダプターでデバイスを生成
		hr = D3D12CreateDevice(useAdapter, featureLevels[i], IID_PPV_ARGS(&device));
		//指定した機能レベルでデバイスを生成できたかを確認
		if (SUCCEEDED(hr)) {
			//作成出来たらログを出力してループを抜ける
			Log(std::format("FeatureLevel : {}\n", featureLevelStrings[i]));
			break;
		}
	}
	//デバイスの生成が上手く行かなかったので起動できない
	assert(device != nullptr);
	Log("Complete create D3D12Device!!!\n");
#pragma endregion

	MSG msg{};
	//ウィンドウのXボタンが押されるまでループ
	while (msg.message != WM_QUIT) {
		//ウィンドウにメッセージが来ていたら最優先で処理させる
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else {
			//ゲームの処理
		}
	}

	//出力ウィンドウに出力
	OutputDebugStringA("HelloDirectX!!!\n");
	return 0;
}