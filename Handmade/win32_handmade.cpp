#include <windows.h>
#include <stdint.h>

static bool Running;

// Bitmap globals
static BITMAPINFO BitmapInfo;
static void* BitmapMemory;
static int BitmapWidth;
static int BitmapHeight;
static int BytesPerPixel = 4;

static void RenderWeirdGradient(int XOffset, int YOffset) {
	int Width = BitmapWidth;
	int Height = BitmapHeight;
	int Pitch = Width * BytesPerPixel;
	
	uint8_t *Row = (uint8_t *)BitmapMemory;
	for (int Y = 0; Y < BitmapHeight; ++Y) {
		uint32_t *Pixel = (uint32_t *)Row;
		for (int X = 0; X < BitmapWidth; ++X) {
			uint8_t Blue = (X + YOffset);
			uint8_t Green = (Y + XOffset);
			uint8_t Red = Y;

			*Pixel++ = ((Red << 16) | (Green << 8) | Blue);
		}
		Row += Pitch;
	}
}

static void ResizeDIBSection(int Width, int Height) {

	if (BitmapMemory) {
		VirtualFree(BitmapMemory, 0, MEM_RELEASE);
	}

	BitmapWidth = Width;
	BitmapHeight = Height;

	BitmapInfo.bmiHeader.biSize = sizeof(BitmapInfo.bmiHeader);
	BitmapInfo.bmiHeader.biWidth = BitmapWidth;
	BitmapInfo.bmiHeader.biHeight = -BitmapHeight;
	BitmapInfo.bmiHeader.biPlanes = 1;
	BitmapInfo.bmiHeader.biBitCount = 32;
	BitmapInfo.bmiHeader.biCompression = BI_RGB;

	int BitmapSize = (Width*Height)*BytesPerPixel;
	BitmapMemory = VirtualAlloc(0, BitmapSize, MEM_COMMIT, PAGE_READWRITE);
}

static void HmUpdateWindow(HDC DeviceContext, RECT WindowRect, int X, int Y, int Width, int Height) {
	int WindowWidth = WindowRect.right - WindowRect.left;
	int WindowHeight = WindowRect.bottom - WindowRect.top;

	StretchDIBits(
		DeviceContext,
		/*X, Y, Width, Height,
		X, Y, Width, Height,*/
		0, 0, BitmapWidth, BitmapHeight,
		0, 0, WindowWidth, WindowHeight,
		BitmapMemory,
		&BitmapInfo,
		DIB_RGB_COLORS, SRCCOPY
	);
}


LRESULT CALLBACK MainWindowCallback(
	HWND Window,
	UINT Message,
	WPARAM WParam,
	LPARAM LParam
) {
	LRESULT Result = 0;
	switch (Message) {
		case WM_SIZE:
		{
			RECT ClientRect;
			GetClientRect(Window, &ClientRect);
			int Width = ClientRect.right - ClientRect.left;
			int Height = ClientRect.bottom - ClientRect.top;
			ResizeDIBSection(Width, Height);
		} break;
		case WM_CLOSE:
		{
			Running = false;
		} break;
		case WM_ACTIVATEAPP:
		{
			OutputDebugStringA("WM_ACTIVATEAPP\n");
		} break;
		case WM_DESTROY:
		{
			Running = false;
		} break;
		case WM_PAINT:
		{	
			PAINTSTRUCT Paint;
			HDC DeviceContext = BeginPaint(Window, &Paint);
			int X = Paint.rcPaint.left;
			int Y = Paint.rcPaint.top;
			int Width = Paint.rcPaint.right - Paint.rcPaint.left;
			int Height = Paint.rcPaint.bottom - Paint.rcPaint.top;
			
			RECT ClientRect;
			GetClientRect(Window, &ClientRect);

			HmUpdateWindow(DeviceContext, ClientRect, X, Y, Width, Height);
			
			EndPaint(Window, &Paint);
		} break;
		default:
		{
			Result = DefWindowProc(Window, Message, WParam, LParam);
		} break;
	}

	return(Result);
}

int CALLBACK WinMain(
	HINSTANCE Instance,
	HINSTANCE PrevInstance,
	LPSTR CmdLine,
	int CmdShow
) {
	WNDCLASS WindowClass = {};
	WindowClass.style = CS_HREDRAW|CS_VREDRAW;
	WindowClass.lpfnWndProc = MainWindowCallback;
	WindowClass.hInstance = Instance;
	WindowClass.lpszClassName = "HandmadeWindowClass";

	if (RegisterClass(&WindowClass)) {
		HWND WindowHandle = CreateWindowEx(
			0,
			WindowClass.lpszClassName,
			"Handmade",
			WS_OVERLAPPEDWINDOW | WS_VISIBLE,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			0,
			0,
			Instance,
			0);

		if (WindowHandle) {
			MSG Message;

			int XOffset = 0;
			int YOffset = 0;
			Running = true;
			while (Running) {
				while (PeekMessage(&Message, 0, 0, 0, PM_REMOVE)) {
					if (Message.message == WM_QUIT) {
						Running = false;
					}
					TranslateMessage(&Message);
					DispatchMessage(&Message);
				}

				RenderWeirdGradient(XOffset, YOffset);

				HDC DeviceContext = GetDC(WindowHandle);
				RECT ClientRect;
				GetClientRect(WindowHandle, &ClientRect);
				int WindowWidth = ClientRect.right - ClientRect.left;
				int WindowHeight = ClientRect.bottom - ClientRect.top;
				HmUpdateWindow(DeviceContext, ClientRect, 0, 0, WindowWidth, WindowHeight);
				ReleaseDC(WindowHandle, DeviceContext);

				++XOffset;
				++YOffset;
			}
		}
		else {

		}
	}
	else {

	};

	return(0);
}