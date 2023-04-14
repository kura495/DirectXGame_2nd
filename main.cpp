#include<Windows.h>

// Windowsアプリでのエントリーポイント(main関数)
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
	//出力ウィンドウに出力
	OutputDebugStringA("HelloDirectX!!!");
	return 0;
}