#include <Windows.h>
#include <time.h>
#include <strsafe.h> 
#include <io.h>			//_access()头文件
#include <direct.h>		//_rmdir()头文件

#include"resource.h"

#pragma comment(lib,"Winmm.lib")

//用定时器控制下落，期间可以调整方向
//1-7代表下落中的7中方块，8-14代表已落地的方块;

//为了简洁，编译时将资源文件一起编译进去，音频文件是先临时保存在C:\TetrisRes目录下再读取的，正常退出程序会自动清理
//两个bgm来源qq音乐，音效文件来源网络


#define DOWN_SPEED       50				//下落速度，方便测试

#define SWAP(a,b)  (a=a+b,b=a-b,a=a-b)	//交换两个数的值，a+b的存储空间不允许大于a的

#define ID_TIMER	      1				//控制下落的定时器ID
#define ID_TIMER2         2				//用来监听BGM是否播放完毕

#define LEFT		      1				//左
#define RIGHT		      2				//右
#define UP			      3				//上（旋转）

#define CLIENT_X          (360+210)	    //客户区宽
#define CLIENT_Y          630	    	//客户区高
#define BLOCK             30	 		//方块的边长
#define X_BOLCK_NUM       10			//x方向方块数量
#define Y_BOLCK_NUM	      20			//y方向方块数量

#define MAP_START         0				//开始界面  
#define MAP_INGAME		  1				//游戏中界面
#define MAP_GAME_OVER     2				//游戏结束界面
#define MAP_PAUSE		  3				//游戏暂停

#define KD_UP             0				//下方向键松开
#define KD_DOWN           1				//下方向键按下

#define NB_BGM1           1				//BGM1
#define NB_BGM2           2				//BGM2

HDC hdc, mdc, bufdc;					//hdc:显示的设备环境句柄 mdc:缓冲用的设备环境句柄，防屏闪 buf
HBITMAP bmp, voice_bmp;
int num = 0;
int key = 0;							//按键信息
int map = MAP_START;					//目前未制作其他界面，只有游戏中和游戏结束界面
int keyDown = KD_UP;					//下方向键状态
int nextBlock;							//下一个方块
int score = 0;							//分数
BOOL music = TRUE;						//是否开音效
int nowBgm = -1;						//当前正在播放的bgm


//存储全部方块
int a[20][10] = { 0 };

//存储下落中4个方块位置
POINT blockPoint[4] = { 0 };

LRESULT CALLBACK WinProc(HWND, UINT, WPARAM, LPARAM);
void Init(HWND, HINSTANCE);	//初始化
void Paint(HWND);		//绘制画面
void BlockDown(HWND hwnd);		//方块下落
void BlockMove();		//方块左右移动
void Sort();			//将blockPoint[4]的元素重新排序
BOOL RandBlock();		//随机种类方块,返回值判断游戏是否结束，TRUE结束，FALSE未结束
BOOL Remove();			//消除方块
void CleanUp(HWND);		//清理游戏资源

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR szCmdLine, int iCmdShow)
{
	static TCHAR szAppName[] = TEXT("TetrisAPP");
	HWND hwnd;
	WNDCLASS wndclass;

	wndclass.style = CS_HREDRAW | CS_VREDRAW;
	wndclass.lpfnWndProc = WinProc;
	wndclass.cbClsExtra = 0;
	wndclass.cbWndExtra = 0;
	wndclass.hInstance = hInstance;
	wndclass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
	wndclass.hbrBackground = (HBRUSH)CreateSolidBrush(RGB(0xc0, 0xc0, 0xc0));
	wndclass.lpszMenuName = NULL;
	wndclass.lpszClassName = szAppName;

	if (!RegisterClass(&wndclass))
	{
		MessageBox(NULL, TEXT("这个程序需要在 Windows NT 才能运行！"), szAppName, MB_ICONERROR);
		return 0;
	}

	hwnd = CreateWindow(szAppName,
		TEXT("俄罗斯方块"),
		WS_OVERLAPPEDWINDOW ^ WS_MAXIMIZEBOX ^ WS_THICKFRAME,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		640,		//窗口宽
		480,		//窗口高
		NULL,
		NULL,
		hInstance,
		NULL);

	ShowWindow(hwnd, iCmdShow);
	UpdateWindow(hwnd);

	MSG msg = { 0 };
	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return msg.wParam;
}

LRESULT CALLBACK WinProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{

	HDC pdc;
	PAINTSTRUCT ps;
	RECT rect;
	TCHAR str[128];

	switch (message)
	{
	case WM_CREATE:
		Init(hwnd, ((LPCREATESTRUCT)lParam)->hInstance);		//初始化
		SetTimer(hwnd, ID_TIMER, DOWN_SPEED, NULL);				//控制下落
		SetTimer(hwnd, ID_TIMER2, 20, NULL);					//监听bgm是否播放完毕
		return 0;
	case WM_TIMER:
		
		if (wParam == ID_TIMER)
		{
			//开始界面
			if (map == MAP_START)
			{
				HBRUSH hBrush = CreateSolidBrush(RGB(0, 0, 0));
				GetClientRect(hwnd, &rect);
				FillRect(mdc, &rect, hBrush);
				HFONT hOldFont, hFont = CreateFont(40, 20, 0, 0, 0, 0, 0, 0, GB2312_CHARSET, 0, 0, 0, 0, TEXT("MyFont"));
				hOldFont = SelectObject(mdc, hFont);
				SetTextColor(mdc, RGB(0xFF, 0xFF, 0xFF));
				TextOut(mdc, 210, 250, TEXT("开始游戏"), 4);
				DeleteObject(SelectObject(mdc, hOldFont));

				if (music)
				{
					SelectObject(bufdc, voice_bmp);
					BitBlt(mdc, 270, 320, 50, 50, bufdc, 0, 0, SRCCOPY);
				}
				else
				{
					SelectObject(bufdc, voice_bmp);
					BitBlt(mdc, 270, 320, 50, 50, bufdc, 50, 0, SRCCOPY);
				}


				BitBlt(hdc, 0, 0, CLIENT_X, CLIENT_Y, mdc, 0, 0, SRCCOPY);
				DeleteObject(hBrush);

			}

			//游戏中界面
			if (map == MAP_INGAME)
			{

				//实现按下方向键加速功能
				//按一次下键便使方块下落一格
				//长按时，keyDown一直等于KD_DOWN，方块会加速下落
				//不按下键时，定时器会使num的值在[0,8]之间循坏，当num=0时，方块便会下落一格
				if (num < 1 || keyDown == KD_DOWN)
				{
					BlockDown(hwnd);
					if (map == MAP_INGAME)
					{
						Paint(hwnd);
					}
					num = 1;
				}
				else if (num < 9)
				{
					if (num == 8)
					{
						num = 0;
					}
					else
					{
						num++;
					}
				}
			}
		}
		if (wParam == ID_TIMER2 && map == MAP_INGAME)
		{
			//两首BGM交替播放
			if (nowBgm == NB_BGM1)
			{
				mciSendString(TEXT("status bgm1 mode"), str, sizeof(str), NULL);
				if (_wcsicmp(str, TEXT("stopped")) == 0)
				{
					//如果播放完毕，就播放另一首BGM
					mciSendString(TEXT("close bgm2"), NULL, 0, NULL);
					mciSendString(TEXT("open C:\\TetrisRes\\2462 type MPEGVideo alias bgm2"), NULL, 0, NULL);
					mciSendString(TEXT("play bgm2"), NULL, 0, NULL);
					nowBgm = NB_BGM2;
				}
			}
			else if (nowBgm == NB_BGM2)
			{
				mciSendString(TEXT("status bgm2 mode"), str, sizeof(str), NULL);
				if (_wcsicmp(str, TEXT("stopped")) == 0)
				{
					//如果播放完毕，就播放另一首BGM
					mciSendString(TEXT("close bgm1"), NULL, 0, NULL);
					mciSendString(TEXT("open C:\\TetrisRes\\2461 type MPEGVideo alias bgm1"), NULL, 0, NULL);
					mciSendString(TEXT("play bgm1"), NULL, 0, NULL);
					nowBgm = NB_BGM1;
				}
			}

		}
		return 0;
	case WM_MOUSEMOVE:
		//鼠标在开始界面文字上变成手
		if (map == MAP_START && (LOWORD(lParam) > 210 && LOWORD(lParam) < 380 && HIWORD(lParam) > 250 && HIWORD(lParam) < 290 || LOWORD(lParam) > 270 && LOWORD(lParam) < 320 && HIWORD(lParam) > 320 && HIWORD(lParam) < 370))
		{
			SetCursor(LoadCursor(NULL, IDC_HAND));
		}

		return 0;
	case WM_LBUTTONUP:
		//点击开始游戏
		if (map == MAP_START && LOWORD(lParam) > 210 && LOWORD(lParam) < 380 && HIWORD(lParam) > 250 && HIWORD(lParam) < 290)
		{
			map = MAP_INGAME;
			if (music)
			{
				//初始随机播放bgm
				if (rand() % 2)
				{
					mciSendString(TEXT("close bgm1"), NULL, 0, NULL);
					mciSendString(TEXT("open C:\\TetrisRes\\2461 type MPEGVideo alias bgm1"), NULL, 0, NULL);
					mciSendString(TEXT("play bgm1"), NULL, 0, NULL);
					nowBgm = NB_BGM1;
				}
				else
				{
					mciSendString(TEXT("close bgm2"), NULL, 0, NULL);
					mciSendString(TEXT("open C:\\TetrisRes\\2462 type MPEGVideo alias bgm2"), NULL, 0, NULL);
					mciSendString(TEXT("play bgm2"), NULL, 0, NULL);
					nowBgm = NB_BGM2;
				}

				
			}
		}

		//点击声音图标
		if (map == MAP_START && LOWORD(lParam) > 270 && LOWORD(lParam) < 320 && HIWORD(lParam) > 320 && HIWORD(lParam) < 370)
		{
			//当前开声音，则关闭声音
			if (music)
			{
				music = FALSE;
				mciSendString(TEXT("close all"), NULL, 0, NULL);
			}
			//反之
			else
			{
				music = TRUE;
			}
		}
		//游戏开始中的暂停按钮
		if (map != MAP_START && ((LOWORD(lParam) > (X_BOLCK_NUM + 3.5)*BLOCK && LOWORD(lParam) < (X_BOLCK_NUM + 6.5)*BLOCK && HIWORD(lParam) > 14.5 * BLOCK && HIWORD(lParam) < 15.5 * BLOCK)))
		{
			//当前游戏开始中，则暂停游戏，暂停bgm
			if (map == MAP_INGAME)
			{
				map = MAP_PAUSE;
				if (nowBgm == NB_BGM1)
				{
					mciSendString(TEXT("stop bgm1"), NULL, 0, NULL);
				}
				else if (nowBgm == NB_BGM2)
				{
					mciSendString(TEXT("stop bgm2"), NULL, 0, NULL);
				}
			}
			//反之
			else
			{
				map = MAP_INGAME;
				if (nowBgm == NB_BGM1)
				{
					mciSendString(TEXT("play bgm1"), NULL, 0, NULL);
				}
				else if (nowBgm == NB_BGM2)
				{
					mciSendString(TEXT("play bgm2"), NULL, 0, NULL);
				}
			}
			//刷新一次界面
			Paint(hwnd);
		}
		//游戏开始中或暂停时点击返回
		if ((map == MAP_INGAME || map == MAP_PAUSE) && LOWORD(lParam) > (X_BOLCK_NUM + 3.5)*BLOCK && LOWORD(lParam) < (X_BOLCK_NUM + 6.5)*BLOCK && HIWORD(lParam) > 16.5 * BLOCK && HIWORD(lParam) < 17.5 * BLOCK)
		{
			int temp = map;		//记录下当前游戏状态
			map = MAP_PAUSE;	//用来阻塞方块下落
			if (MessageBox(hwnd, TEXT("当前进度会丢失，确定要返回主菜单？"), TEXT("警告"), MB_YESNO | MB_ICONEXCLAMATION) == IDYES)
			{
				mciSendString(TEXT("close all"), NULL, 0, NULL);
				map = MAP_START;
				memset(a, 0, sizeof(a));
				score = 0;
				num = 0;
				memset(blockPoint, 0, sizeof(blockPoint));
				nextBlock = rand() % 7;
				RandBlock();
				key = 0;
				keyDown = KD_UP;
				map = MAP_START;
				return 0;
			}
			//还原状态
			map = temp;
		}
		return 0;
	case WM_KEYUP:
		if (wParam == VK_DOWN)
		{
			keyDown = KD_UP;
		}
		return 0;
	case WM_KEYDOWN:
		if (map == MAP_INGAME)
		{
			switch (wParam)
			{
			case VK_LEFT:
				key = LEFT;
				break;
			case VK_RIGHT:
				key = RIGHT;
				break;
			case VK_UP:
				key = UP;
				break;
			case VK_DOWN:
				key = 0;
				keyDown = KD_DOWN;
				break;
			}
			BlockMove();
			Paint(hwnd);
		}

		if (wParam == VK_ESCAPE)
		{

			DestroyWindow(hwnd);
		}

		return 0;
		case WM_PAINT:
		//WM_PAINT 消息中的设备环境句柄只能使用BeginPaint()返回的
		//且必须有EndPaint()
		//否则会一直收到WM_PAINT消息，导致程序出问题
		pdc = BeginPaint(hwnd, &ps);

		//响应WM_PAINT消息，将缓冲区的设备环境句柄输出屏幕
		BitBlt(pdc, 0, 0, CLIENT_X, CLIENT_Y, mdc, 0, 0, SRCCOPY);

		EndPaint(hwnd, &ps);
		return 0;
	case WM_DESTROY:
		//程序退出前最后处理
		CleanUp(hwnd);
		PostQuitMessage(0);
		return 0;
	}

	return DefWindowProc(hwnd, message, wParam, lParam);
}

void Init(HWND hwnd, HINSTANCE hInstance)
{
	//窗口居中、改变大小
	RECT rcWindow, rcClinet;
	int winX, winY;
	GetWindowRect(hwnd, &rcWindow);
	GetClientRect(hwnd, &rcClinet);
	winX = rcWindow.right - rcWindow.left - (rcClinet.right - rcClinet.left) + CLIENT_X;
	winY = rcWindow.bottom - rcWindow.top - (rcClinet.bottom - rcClinet.top) + CLIENT_Y;
	MoveWindow(hwnd, (GetSystemMetrics(SM_CXSCREEN) - winX) / 2, (GetSystemMetrics(SM_CYSCREEN) - winY) / 2, winX, winY, TRUE);

	//创建并初始化hdc
	hdc = GetDC(hwnd);
	mdc = CreateCompatibleDC(hdc);
	bufdc = CreateCompatibleDC(hdc);
	bmp = CreateCompatibleBitmap(hdc, CLIENT_X, CLIENT_Y);
	SelectObject(mdc, bmp);

	//透明文字背景
	SetBkMode(mdc, TRANSPARENT);

	//加载位图
	bmp = LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_BITMAP1));				//方块
	voice_bmp = LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_BITMAP2));		//声音图标

	//创建文件夹
	char folderName[] = "C:\\TetrisRes";
	//若文件夹不存在则创建（防止上次软件意外意外没有删除目录）
	if (_access(folderName, 0) == -1)
	{
		_mkdir(folderName);
	}
	
	HRSRC hResource;
	HGLOBAL hg;
	LPVOID pDate;
	DWORD dwSize;
	FILE *fp;

	//往C:\TetrisRes下写入音频文件，若正常退出软件会自动删除

	//找到资源文件
	hResource = FindResource(NULL, MAKEINTRESOURCE(IDR_MP31), TEXT("MP3"));
	//加载资源文件
	hg = LoadResource(NULL, hResource);
	//锁定资源文件
	pDate = LockResource(hg);
	//获取资源文件大小
	dwSize = SizeofResource(NULL, hResource);
	//将资源文件写入磁盘
	fp = fopen("C:\\TetrisRes\\2461", "wb");
	fwrite(pDate, sizeof(char), dwSize, fp);
	fclose(fp);

	//找到资源文件
	hResource = FindResource(NULL, MAKEINTRESOURCE(IDR_MP32), TEXT("MP3"));
	//加载资源文件
	hg = LoadResource(NULL, hResource);
	//锁定资源文件
	pDate = LockResource(hg);
	//获取资源文件大小
	dwSize = SizeofResource(NULL, hResource);
	//将资源文件写入磁盘
	fp = fopen("C:\\TetrisRes\\2462", "wb");
	fwrite(pDate, sizeof(char), dwSize, fp);
	fclose(fp);

	//找到资源文件
	hResource = FindResource(NULL, MAKEINTRESOURCE(IDR_WAVE1), TEXT("WAVE"));
	//加载资源文件
	hg = LoadResource(NULL, hResource);
	//锁定资源文件
	pDate = LockResource(hg);
	//获取资源文件大小
	dwSize = SizeofResource(NULL, hResource);
	//将资源文件写入磁盘
	fp = fopen("C:\\TetrisRes\\991", "wb");
	fwrite(pDate, sizeof(char), dwSize, fp);
	fclose(fp);

	//找到资源文件
	hResource = FindResource(NULL, MAKEINTRESOURCE(IDR_WAVE2), TEXT("WAVE"));
	//加载资源文件
	hg = LoadResource(NULL, hResource);
	//锁定资源文件
	pDate = LockResource(hg);
	//获取资源文件大小
	dwSize = SizeofResource(NULL, hResource);
	//将资源文件写入磁盘
	fp = fopen("C:\\TetrisRes\\992", "wb");
	fwrite(pDate, sizeof(char), dwSize, fp);
	fclose(fp);


	//随机种子
	srand((UINT)time(NULL));

	nextBlock = rand() % 7;
	RandBlock();
}

void Paint(HWND hwnd)
{
	int x, y, bmp_x;
	RECT rect;
	TCHAR buff[128];
	HBRUSH hBrush = CreateSolidBrush(RGB(0xc0, 0xc0, 0xc0));

	SelectObject(bufdc, bmp);
	SetTextColor(mdc, RGB(0, 0, 0));

	//背景色
	GetClientRect(hwnd, &rect);
	FillRect(mdc, &rect, hBrush);
	
	//绘边框
	for (x = 0; x < 2; x++)
	{
		for (y = 0; y < Y_BOLCK_NUM + 1; y++)
		{
			BitBlt(mdc, x * BLOCK * (X_BOLCK_NUM + 1), y * BLOCK, BLOCK, BLOCK, bufdc, 0, 0, SRCCOPY);
		}
	}
	for (x = 1; x < X_BOLCK_NUM + 2; x++)
	{
		BitBlt(mdc, x * BLOCK, (CLIENT_Y - BLOCK), BLOCK, BLOCK, bufdc, 0, 0, SRCCOPY);
	}

	//绘制右边
	for (x = (X_BOLCK_NUM + 2) * BLOCK; x < CLIENT_X; x += BLOCK)
	{
		for (y = 0; y < CLIENT_Y; y += BLOCK)
		{
			BitBlt(mdc, x, y, BLOCK, BLOCK, bufdc, 0, 0, SRCCOPY);
		}
	}
	//NEXT BLOCK
	rect.left = (X_BOLCK_NUM + 2)*BLOCK;
	rect.right = rect.left + 6 * BLOCK;
	rect.top = 5 * BLOCK;
	rect.bottom = rect.top + 4 * BLOCK;
	FillRect(mdc, &rect, hBrush);

	rect.left = (X_BOLCK_NUM + 4)*BLOCK;
	rect.right = rect.left + 2 * BLOCK;
	rect.top = 4 * BLOCK;
	rect.bottom = rect.top + BLOCK;
	FillRect(mdc, &rect, hBrush);
	DrawText(mdc, TEXT("NEXT"), -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

	//分数
	rect.left = (X_BOLCK_NUM + 2)*BLOCK;
	rect.right = rect.left + 6 * BLOCK;
	rect.top = 11 * BLOCK;
	rect.bottom = rect.top + 2 * BLOCK;
	FillRect(mdc, &rect, hBrush);
	rect.bottom = rect.top + BLOCK;
	DrawText(mdc, TEXT("得分"), -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
	rect.top = 12 * BLOCK;
	rect.bottom = rect.top + BLOCK;
	StringCchPrintf(buff, 128, TEXT("%d"), score);
	DrawText(mdc, buff, -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

	//下一个方块的绘制
	switch (nextBlock)
	{
	case 0:
		for (x = X_BOLCK_NUM + 4; x < X_BOLCK_NUM + 6; x++)
		{
			BitBlt(mdc, x*BLOCK, 7 * BLOCK, BLOCK, BLOCK, bufdc, (nextBlock + 1) * BLOCK, 0, SRCCOPY);
		}
		for (x = X_BOLCK_NUM + 4; x < X_BOLCK_NUM + 6; x++)
		{
			BitBlt(mdc, x*BLOCK, 6 * BLOCK, BLOCK, BLOCK, bufdc, (nextBlock + 1) * BLOCK, 0, SRCCOPY);
		}
		break;
	case 1:
		for (x = X_BOLCK_NUM + 3; x < X_BOLCK_NUM + 7; x++)
		{
			BitBlt(mdc, x*BLOCK, 6 * BLOCK, BLOCK, BLOCK, bufdc, (nextBlock + 1) * BLOCK, 0, SRCCOPY);
		}
		break;
	case 2:
		for (x = X_BOLCK_NUM + 4; x < X_BOLCK_NUM + 7; x++)
		{
			BitBlt(mdc, x*BLOCK, 7 * BLOCK, BLOCK, BLOCK, bufdc, (nextBlock + 1) * BLOCK, 0, SRCCOPY);
		}
		BitBlt(mdc, (X_BOLCK_NUM + 5)*BLOCK, 6 * BLOCK, BLOCK, BLOCK, bufdc, (nextBlock + 1) * BLOCK, 0, SRCCOPY);
		break;
	case 3:
		for (x = X_BOLCK_NUM + 4; x < X_BOLCK_NUM + 7; x++)
		{
			BitBlt(mdc, x*BLOCK, 6 * BLOCK, BLOCK, BLOCK, bufdc, (nextBlock + 1) * BLOCK, 0, SRCCOPY);
		}
		BitBlt(mdc, (X_BOLCK_NUM + 6)*BLOCK, 7 * BLOCK, BLOCK, BLOCK, bufdc, (nextBlock + 1) * BLOCK, 0, SRCCOPY);
		break;
	case 4:
		for (x = X_BOLCK_NUM + 4; x < X_BOLCK_NUM + 7; x++)
		{
			BitBlt(mdc, x*BLOCK, 6 * BLOCK, BLOCK, BLOCK, bufdc, (nextBlock + 1) * BLOCK, 0, SRCCOPY);
		}
		BitBlt(mdc, (X_BOLCK_NUM + 4)*BLOCK, 7 * BLOCK, BLOCK, BLOCK, bufdc, (nextBlock + 1) * BLOCK, 0, SRCCOPY);
		break;
	case 5:
		for (x = X_BOLCK_NUM + 4; x < X_BOLCK_NUM + 6; x++)
		{
			BitBlt(mdc, x*BLOCK, 6 * BLOCK, BLOCK, BLOCK, bufdc, (nextBlock + 1) * BLOCK, 0, SRCCOPY);
		}
		for (x = X_BOLCK_NUM + 5; x < X_BOLCK_NUM + 7; x++)
		{
			BitBlt(mdc, x*BLOCK, 7 * BLOCK, BLOCK, BLOCK, bufdc, (nextBlock + 1) * BLOCK, 0, SRCCOPY);
		}
		break;
	case 6:
		for (x = X_BOLCK_NUM + 4; x < X_BOLCK_NUM + 6; x++)
		{
			BitBlt(mdc, x*BLOCK, 7 * BLOCK, BLOCK, BLOCK, bufdc, (nextBlock + 1) * BLOCK, 0, SRCCOPY);
		}
		for (x = X_BOLCK_NUM + 5; x < X_BOLCK_NUM + 7; x++)
		{
			BitBlt(mdc, x*BLOCK, 6 * BLOCK, BLOCK, BLOCK, bufdc, (nextBlock + 1) * BLOCK, 0, SRCCOPY);
		}
		break;
	}

	//写点废话
	rect.left = (X_BOLCK_NUM + 2)*BLOCK;
	rect.right = rect.left + 6 * BLOCK;
	rect.top = 19 * BLOCK;
	rect.bottom = rect.top + BLOCK;
	FillRect(mdc, &rect, hBrush);
	DrawText(mdc, TEXT("无聊出品，必属废品"), -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

	//按钮空间
	rect.left = (X_BOLCK_NUM + 2)*BLOCK;
	rect.right = rect.left + 6 * BLOCK;
	rect.top = 14 * BLOCK;
	rect.bottom = rect.top + 4 * BLOCK;
	FillRect(mdc, &rect, hBrush);
	//绘制按钮
	hBrush = CreateSolidBrush(RGB(0xA0, 0xA0, 0xA0));
	rect.left = (X_BOLCK_NUM + 3.5)*BLOCK;
	rect.right = rect.left + 3 * BLOCK;
	rect.top = 14.5 * BLOCK;
	rect.bottom = rect.top + BLOCK;
	FillRect(mdc, &rect, hBrush);
	if (map != MAP_PAUSE)
	{
		DrawText(mdc, TEXT("暂停"), -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
	}
	else
	{
		DrawText(mdc, TEXT("继续"), -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
	}
	rect.left = (X_BOLCK_NUM + 3.5)*BLOCK;
	rect.right = rect.left + 3 * BLOCK;
	rect.top = 16.5 * BLOCK;
	rect.bottom = rect.top + BLOCK;
	FillRect(mdc, &rect, hBrush);
	DrawText(mdc, TEXT("返回"), -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

	//绘制方块
	int n = 0;
	for (x = Y_BOLCK_NUM - 1; x >= 0; x--)			//x为行数
	{
		for (y = 0; y < X_BOLCK_NUM; y++)		//y为列数
		{
			if (a[x][y] != 0)
			{
				if (a[x][y] < 8)
				{
					bmp_x = a[x][y] * BLOCK;
				}
				else
				{
					bmp_x = (a[x][y] - 7) * BLOCK;
				}
				BitBlt(mdc, y * BLOCK + BLOCK, x * BLOCK, BLOCK, BLOCK, bufdc, bmp_x, 0, SRCCOPY);
			}
			if (a[x][y] > 0 && a[x][y] < 8)
			{
				blockPoint[n].x = x;			//下排序
				blockPoint[n].y = y;
				n++;
			}

		}
	}
	BitBlt(hdc, 0, 0, CLIENT_X, CLIENT_Y, mdc, 0, 0, SRCCOPY);
	DeleteObject(hBrush);
}

void BlockMove()
{
	//排序，为移动和旋转做准备
	//旋转排序为先左再下
	Sort();

	//是否能移动
	BOOL flag = TRUE;

	int i;
	switch (key)
	{
	case LEFT:
		//判断能否移动
		for (i = 0; i < 4; i++)
		{
			if (!(blockPoint[i].y > 0 && a[blockPoint[i].x][blockPoint[i].y - 1] < 8))
			{
				flag = FALSE;
				break;
			}
		}
		if (flag)
		{
			for (i = 0; i < 4; i++)
			{
				SWAP(a[blockPoint[i].x][blockPoint[i].y - 1], a[blockPoint[i].x][blockPoint[i].y]);
			}
		}
		break;
	case RIGHT:
		for (i = 0; i < 4; i++)
		{
			if (!(blockPoint[i].y < X_BOLCK_NUM - 1 && a[blockPoint[i].x][blockPoint[i].y + 1] < 8))
			{
				flag = FALSE;
				break;
			}
		}
		if (flag)
		{
			for (i = 0; i < 4; i++)
			{
				SWAP(a[blockPoint[i].x][blockPoint[i].y + 1], a[blockPoint[i].x][blockPoint[i].y]);
			}
		}
		break;
	case UP:		//旋转
		switch (a[blockPoint[0].x][blockPoint[0].y])			//根据方块类型，穷举所有情况，蠢而有效（找不到其他好方法了）
		{
		case 1:			//田字方块，无需旋转
			break;
		case 2:
			if (blockPoint[0].x == blockPoint[1].x && blockPoint[1].x == blockPoint[2].x && blockPoint[2].x == blockPoint[3].x)
			{
				//判断是否可以旋转
				if (a[blockPoint[1].x - 1][blockPoint[1].y] == 0 && a[blockPoint[1].x + 1][blockPoint[1].y] == 0 && a[blockPoint[1].x + 2][blockPoint[1].y] == 0 && blockPoint[1].x >= 1 && blockPoint[1].x + 2 < Y_BOLCK_NUM)
				{
					a[blockPoint[1].x - 1][blockPoint[1].y] = a[blockPoint[1].x + 1][blockPoint[1].y] = a[blockPoint[1].x + 2][blockPoint[1].y] = 2;
					a[blockPoint[0].x][blockPoint[0].y] = a[blockPoint[2].x][blockPoint[2].y] = a[blockPoint[3].x][blockPoint[3].y] = 0;
				}
			}
			else if (blockPoint[0].y == blockPoint[1].y && blockPoint[1].y == blockPoint[2].y && blockPoint[2].y == blockPoint[3].y)
			{
				//判断是否可以旋转
				if (a[blockPoint[2].x][blockPoint[2].y - 1] == 0 && a[blockPoint[2].x][blockPoint[2].y + 1] == 0 && a[blockPoint[2].x][blockPoint[2].y + 2] == 0 && blockPoint[2].y - 1 >= 0 && blockPoint[2].y + 2 < X_BOLCK_NUM)
				{
					a[blockPoint[2].x][blockPoint[2].y - 1] = a[blockPoint[2].x][blockPoint[2].y + 1] = a[blockPoint[2].x][blockPoint[2].y + 2] = 2;
					a[blockPoint[0].x][blockPoint[0].y] = a[blockPoint[1].x][blockPoint[2].y] = a[blockPoint[3].x][blockPoint[3].y] = 0;
				}
				else if (a[blockPoint[2].x][blockPoint[2].y + 1] == 0 && a[blockPoint[2].x][blockPoint[2].y - 1] == 0 && a[blockPoint[2].x][blockPoint[2].y - 2] == 0 && blockPoint[2].y - 2 >= 0 && blockPoint[2].y + 1 < X_BOLCK_NUM)
				{
					a[blockPoint[2].x][blockPoint[2].y + 1] = a[blockPoint[2].x][blockPoint[2].y - 1] = a[blockPoint[2].x][blockPoint[2].y - 2] = 2;
					a[blockPoint[0].x][blockPoint[0].y] = a[blockPoint[1].x][blockPoint[1].y] = a[blockPoint[3].x][blockPoint[3].y] = 0;
				}
			}
			break;

		case 3:
			if (blockPoint[0].x == blockPoint[1].x && blockPoint[1].x == blockPoint[3].x && blockPoint[2].y == blockPoint[1].y && blockPoint[1].x > blockPoint[2].x)
			{
				//判断是否可以旋转
				if (a[blockPoint[1].x + 1][blockPoint[1].y] == 0 && blockPoint[1].x + 1 < Y_BOLCK_NUM)
				{
					a[blockPoint[1].x + 1][blockPoint[1].y] = 3;
					a[blockPoint[0].x][blockPoint[0].y] = 0;
				}
			}
			else if (blockPoint[0].y == blockPoint[1].y && blockPoint[1].y == blockPoint[2].y && blockPoint[3].x == blockPoint[1].x && blockPoint[3].y > blockPoint[1].y)
			{
				//判断是否可以旋转
				if (a[blockPoint[1].x][blockPoint[1].y - 1] == 0 && blockPoint[1].y - 1 >= 0)
				{
					a[blockPoint[1].x][blockPoint[1].y - 1] = 3;
					a[blockPoint[2].x][blockPoint[2].y] = 0;
				}
			}
			else if (blockPoint[0].x == blockPoint[2].x && blockPoint[2].x == blockPoint[3].x && blockPoint[2].y == blockPoint[1].y && blockPoint[1].x > blockPoint[2].x)
			{
				//判断是否可以旋转
				if (a[blockPoint[2].x - 1][blockPoint[2].y] == 0 && blockPoint[1].x - 1 >= 0)
				{
					a[blockPoint[2].x - 1][blockPoint[2].y] = 3;
					a[blockPoint[3].x][blockPoint[3].y] = 0;
				}
			}
			else
			{
				//判断是否可以旋转
				if (a[blockPoint[2].x][blockPoint[2].y + 1] == 0 && blockPoint[2].y + 1 < X_BOLCK_NUM)
				{
					a[blockPoint[2].x][blockPoint[2].y + 1] = 3;
					a[blockPoint[1].x][blockPoint[1].y] = 0;
				}
			}
			break;

		case 4:
			if (blockPoint[0].x == blockPoint[1].x && blockPoint[1].x == blockPoint[3].x && blockPoint[3].y == blockPoint[2].y && blockPoint[3].x < blockPoint[2].x)
			{
				//判断是否可以旋转
				if (a[blockPoint[0].x + 1][blockPoint[0].y] == 0 && a[blockPoint[1].x - 1][blockPoint[1].y] == 0 && a[blockPoint[1].x + 1][blockPoint[1].y] == 0 && blockPoint[0].x + 1 < Y_BOLCK_NUM && blockPoint[1].x - 1 >= 0)
				{
					a[blockPoint[0].x + 1][blockPoint[0].y] = a[blockPoint[1].x - 1][blockPoint[1].y] = a[blockPoint[1].x + 1][blockPoint[1].y] = 4;
					a[blockPoint[0].x][blockPoint[0].y] = a[blockPoint[2].x][blockPoint[2].y] = a[blockPoint[3].x][blockPoint[3].y] = 0;
				}
			}
			else if (blockPoint[1].y == blockPoint[2].y && blockPoint[2].y == blockPoint[3].y && blockPoint[1].y > blockPoint[0].y && blockPoint[1].x == blockPoint[0].x)
			{
				//判断是否可以旋转
				if (a[blockPoint[3].x][blockPoint[3].y - 1] == 0 && a[blockPoint[2].x][blockPoint[2].y - 1] == 0 && a[blockPoint[2].x][blockPoint[2].y + 1] == 0 && blockPoint[2].y + 1 < X_BOLCK_NUM && blockPoint[2].y - 1 >= 0)
				{
					a[blockPoint[3].x][blockPoint[3].y - 1] = a[blockPoint[2].x][blockPoint[2].y - 1] = a[blockPoint[2].x][blockPoint[2].y + 1] = 4;
					a[blockPoint[0].x][blockPoint[0].y] = a[blockPoint[1].x][blockPoint[1].y] = a[blockPoint[3].x][blockPoint[3].y] = 0;
				}
			}
			else if (blockPoint[0].x == blockPoint[2].x && blockPoint[2].x == blockPoint[3].x && blockPoint[0].y == blockPoint[1].y && blockPoint[0].x > blockPoint[1].x)
			{
				//判断是否可以旋转
				if (a[blockPoint[3].x - 1][blockPoint[3].y] == 0 && a[blockPoint[2].x - 1][blockPoint[2].y] == 0 && a[blockPoint[2].x + 1][blockPoint[2].y] == 0 && blockPoint[2].x + 1 < Y_BOLCK_NUM && blockPoint[2].x - 1 >= 0)
				{
					a[blockPoint[3].x - 1][blockPoint[3].y] = a[blockPoint[2].x - 1][blockPoint[2].y] = a[blockPoint[2].x + 1][blockPoint[2].y] = 4;
					a[blockPoint[0].x][blockPoint[0].y] = a[blockPoint[1].x][blockPoint[1].y] = a[blockPoint[3].x][blockPoint[3].y] = 0;
				}
			}
			else
			{
				if (a[blockPoint[0].x][blockPoint[0].y + 1] == 0 && a[blockPoint[1].x][blockPoint[1].y - 1] == 0 && a[blockPoint[1].x][blockPoint[1].y + 1] == 0 && blockPoint[1].y + 1 < X_BOLCK_NUM && blockPoint[1].y - 1 >= 0)
				{
					a[blockPoint[0].x][blockPoint[0].y + 1] = a[blockPoint[1].x][blockPoint[1].y - 1] = a[blockPoint[1].x][blockPoint[1].y + 1] = 4;
					a[blockPoint[0].x][blockPoint[0].y] = a[blockPoint[2].x][blockPoint[2].y] = a[blockPoint[3].x][blockPoint[3].y] = 0;
				}
			}
			break;

		case 5:
			if (blockPoint[1].x == blockPoint[2].x && blockPoint[2].x == blockPoint[3].x && blockPoint[1].y == blockPoint[0].y && blockPoint[1].x < blockPoint[0].x)
			{
				//判断是否可以旋转
				if (a[blockPoint[1].x - 1][blockPoint[1].y] == 0 && a[blockPoint[2].x - 1][blockPoint[2].y] == 0 && a[blockPoint[2].x + 1][blockPoint[2].y] == 0 && blockPoint[2].x + 1 < Y_BOLCK_NUM && blockPoint[2].x - 1 >= 0)
				{
					a[blockPoint[1].x - 1][blockPoint[1].y] = a[blockPoint[2].x - 1][blockPoint[2].y] = a[blockPoint[2].x + 1][blockPoint[2].y] = 5;
					a[blockPoint[0].x][blockPoint[0].y] = a[blockPoint[1].x][blockPoint[1].y] = a[blockPoint[3].x][blockPoint[3].y] = 0;
				}
			}
			else if (blockPoint[1].y == blockPoint[2].y && blockPoint[2].y == blockPoint[3].y && blockPoint[3].y > blockPoint[0].y && blockPoint[3].x == blockPoint[0].x)
			{
				//判断是否可以旋转
				if (a[blockPoint[3].x][blockPoint[3].y + 1] == 0 && a[blockPoint[2].x][blockPoint[2].y - 1] == 0 && a[blockPoint[2].x][blockPoint[2].y + 1] == 0 && blockPoint[2].y + 1 < X_BOLCK_NUM && blockPoint[2].y - 1 >= 0)
				{
					a[blockPoint[3].x][blockPoint[3].y + 1] = a[blockPoint[2].x][blockPoint[2].y - 1] = a[blockPoint[2].x][blockPoint[2].y + 1] = 5;
					a[blockPoint[0].x][blockPoint[0].y] = a[blockPoint[1].x][blockPoint[1].y] = a[blockPoint[3].x][blockPoint[3].y] = 0;
				}
			}
			else if (blockPoint[0].x == blockPoint[2].x && blockPoint[2].x == blockPoint[1].x && blockPoint[2].y == blockPoint[3].y && blockPoint[2].x > blockPoint[3].x)
			{
				//判断是否可以旋转
				if (a[blockPoint[2].x + 1][blockPoint[2].y] == 0 && a[blockPoint[1].x - 1][blockPoint[1].y] == 0 && a[blockPoint[1].x + 1][blockPoint[1].y] == 0 && blockPoint[1].x + 1 < Y_BOLCK_NUM && blockPoint[1].x - 1 >= 0)
				{
					a[blockPoint[2].x + 1][blockPoint[2].y] = a[blockPoint[1].x - 1][blockPoint[1].y] = a[blockPoint[1].x + 1][blockPoint[1].y] = 5;
					a[blockPoint[0].x][blockPoint[0].y] = a[blockPoint[2].x][blockPoint[2].y] = a[blockPoint[3].x][blockPoint[3].y] = 0;
				}
			}
			else
			{
				if (a[blockPoint[0].x][blockPoint[0].y - 1] == 0 && a[blockPoint[1].x][blockPoint[1].y - 1] == 0 && a[blockPoint[1].x][blockPoint[1].y + 1] == 0 && blockPoint[1].y + 1 < X_BOLCK_NUM && blockPoint[1].y - 1 >= 0)
				{
					a[blockPoint[0].x][blockPoint[0].y - 1] = a[blockPoint[1].x][blockPoint[1].y - 1] = a[blockPoint[1].x][blockPoint[1].y + 1] = 5;
					a[blockPoint[0].x][blockPoint[0].y] = a[blockPoint[2].x][blockPoint[2].y] = a[blockPoint[3].x][blockPoint[3].y] = 0;
				}
			}
			break;
		case 6:
			if (blockPoint[0].x == blockPoint[2].x && blockPoint[1].x == blockPoint[3].x)
			{
				//判断是否可以旋转
				if (a[blockPoint[0].x + 1][blockPoint[0].y] == 0 && a[blockPoint[2].x - 1][blockPoint[2].y] == 0 && blockPoint[2].x - 1 >= 0)
				{
					a[blockPoint[0].x + 1][blockPoint[0].y] = a[blockPoint[2].x - 1][blockPoint[2].y] = 6;
					a[blockPoint[3].x][blockPoint[3].y] = a[blockPoint[1].x][blockPoint[1].y] = 0;
				}
			}
			else
			{
				//判断是否可以旋转
				if (a[blockPoint[0].x][blockPoint[0].y + 1] == 0 && a[blockPoint[0].x][blockPoint[0].y + 2] == 0 && blockPoint[0].y + 2 < X_BOLCK_NUM)
				{
					a[blockPoint[0].x][blockPoint[0].y + 1] = a[blockPoint[0].x][blockPoint[0].y + 2] = 6;
					a[blockPoint[3].x][blockPoint[3].y] = a[blockPoint[0].x][blockPoint[0].y] = 0;
				}
			}

			break;
		case 7:
			if (blockPoint[0].x == blockPoint[1].x && blockPoint[2].x == blockPoint[3].x)
			{
				//判断是否可以旋转
				if (a[blockPoint[0].x - 1][blockPoint[0].y] == 0 && a[blockPoint[0].x - 2][blockPoint[0].y] == 0 && blockPoint[0].x - 2 >= 0)
				{
					a[blockPoint[0].x - 1][blockPoint[0].y] = a[blockPoint[0].x - 2][blockPoint[0].y] = 7;
					a[blockPoint[3].x][blockPoint[3].y] = a[blockPoint[0].x][blockPoint[0].y] = 0;
				}
			}
			else
			{
				if (a[blockPoint[2].x][blockPoint[2].y - 1] == 0 && a[blockPoint[3].x][blockPoint[3].y + 1] == 0 && blockPoint[3].y + 1 < X_BOLCK_NUM)
				{
					a[blockPoint[2].x][blockPoint[2].y - 1] = a[blockPoint[3].x][blockPoint[3].y + 1] = 7;
					a[blockPoint[0].x][blockPoint[0].y] = a[blockPoint[1].x][blockPoint[1].y] = 0;
				}
			}

			break;
		}


		break;
	}

	key = 0;
}

void BlockDown(HWND hwnd)
{
	int i;
	BOOL flag_down = TRUE;;

	//检测是否可以下落
	for (i = 0; i < 4; i++)
	{
		if (!(a[blockPoint[i].x + 1][blockPoint[i].y] < 8 && blockPoint[0].x < Y_BOLCK_NUM - 1))
		{
			flag_down = FALSE;
		}
	}

	if (flag_down)			//如果可以掉落
	{
		for (i = 0; i < 4; i++)
		{
			SWAP(a[blockPoint[i].x + 1][blockPoint[i].y], a[blockPoint[i].x][blockPoint[i].y]);
		}
	}
	else
	{
		//改为落地状态
		for (i = 0; i < 4; i++)
		{
			a[blockPoint[i].x][blockPoint[i].y] += 7;
		}

		//消除方块
		Remove();
		if (RandBlock())			//TURE则游戏结束
		{
			map = MAP_GAME_OVER;
			if (music)
			{
				mciSendString(TEXT("close all"), NULL, 0, NULL);
				mciSendString(TEXT("open C:\\TetrisRes\\991 type MPEGVideo alias sound1"), NULL, 0, NULL);
				mciSendString(TEXT("play sound1"), NULL, 0, NULL);
			}
			MessageBox(hwnd, TEXT("GAME OVER!"), TEXT("真遗憾"), MB_OK);
			//停止播放音乐
			mciSendString(TEXT("stop all"), NULL, 0, NULL);
			mciSendString(TEXT("close all"), NULL, 0, NULL);

			MessageBox(hwnd, TEXT("哈？ 你在想你的的分数会不会保存下来？\n这还用问吗，当然是不会啦。\n结果不重要，过程快乐就好(试图掩饰自己的懒惰)"), TEXT("梦里什么都有"), MB_YESNO);
			

			//游戏结束，数据初始化
			map = MAP_START;
			memset(a, 0, sizeof(a));
			score = 0;
			num = 0;
			memset(blockPoint, 0, sizeof(blockPoint));
			nextBlock = rand() % 7;
			RandBlock();
			key = 0;
			keyDown = KD_UP;
		}

	}

}

//返回TRUE游戏结束
BOOL RandBlock()
{
	switch (nextBlock)
	{
	case 0:
		if (!(a[0][4] || a[0][5] || a[1][4] || a[1][5]))				//方块生成位置为空，游戏未结束，返回FALSE
		{
			a[0][4] = a[0][5] = a[1][4] = a[1][5] = 1;
			nextBlock = rand() % 7;
			return FALSE;
		}
		return TRUE;
	case 1:
		if (!(a[0][3] || a[0][4] || a[0][5] || a[0][6]))
		{
			a[0][3] = a[0][4] = a[0][5] = a[0][6] = 2;
			nextBlock = rand() % 7;
			return FALSE;
		}
		return TRUE;
	case 2:
		if (!(a[0][5] || a[1][4] || a[1][5] || a[1][6]))
		{
			a[0][5] = a[1][4] = a[1][5] = a[1][6] = 3;
			nextBlock = rand() % 7;
			return FALSE;
		}
		return TRUE;
	case 3:
		if (!(a[0][4] || a[0][5] || a[0][6] || a[1][6]))
		{
			a[0][4] = a[0][5] = a[0][6] = a[1][6] = 4;
			nextBlock = rand() % 7;
			return FALSE;
		}
		return TRUE;
	case 4:
		if (!(a[0][4] || a[0][5] || a[0][6] || a[1][4]))
		{
			a[0][4] = a[0][5] = a[0][6] = a[1][4] = 5;
			nextBlock = rand() % 7;
			return FALSE;
		}
		return TRUE;
	case 5:
		if (!(a[0][4] || a[0][5] || a[1][5] || a[1][6]))
		{
			a[0][4] = a[0][5] = a[1][5] = a[1][6] = 6;
			nextBlock = rand() % 7;
			return FALSE;
		}
		return TRUE;
	case 6:
		if (!(a[0][5] || a[0][6] || a[1][4] || a[1][5]))
		{
			a[0][5] = a[0][6] = a[1][4] = a[1][5] = 7;
			nextBlock = rand() % 7;
			return FALSE;
		}
		return TRUE;
	}
}

BOOL Remove()
{
	int x, y, n = 0;
	BOOL flag_clear = FALSE;	//最后是否有消除的行
	BOOL flag_X = TRUE;			//某行是否可以消除
	int clear_x[4];				//最多可同时消除4行

	//找到可以消除的行数clear_x
	for (x = Y_BOLCK_NUM - 1; x >= 0; x--)
	{
		flag_X = TRUE;
		for (y = 0; y < X_BOLCK_NUM; y++)
		{
			if (a[x][y] < 8)
			{
				flag_X = FALSE;
				break;
			}
		}
		if (flag_X)
		{
			clear_x[n++] = x;
			flag_clear = TRUE;
		}
	}

	//加分数
	score += n;

	//消除clear_x行
	if (flag_clear)
	{
		if (music)
		{
			mciSendString(TEXT("close sound2"), NULL, 0, NULL);
			mciSendString(TEXT("open C:\\TetrisRes\\992 type MPEGVideo alias sound2"), NULL, 0, NULL);
			mciSendString(TEXT("play sound2"), NULL, 0, NULL);
		}
		for (x = 0; x < n; x++)
		{
			for (y = 0; y < X_BOLCK_NUM; y++)
			{
				a[clear_x[x]][y] = 0;
			}
		}

		//使上面方块掉落
		while (n >= 0)
		{
			for (x = clear_x[n - 1]; x > 0; x--)
			{
				for (y = 0; y < X_BOLCK_NUM; y++)
				{
					SWAP(a[x][y], a[x - 1][y]);
				}

			}
			n--;
		}
	}
	return flag_clear;
}

void Sort()
{
	int i, j;

	if (key == LEFT || key == UP)
	{

		for (i = 0; i < 3; i++)
		{
			for (j = 0; j < 3 - i; j++)
			{
				if (blockPoint[j + 1].y < blockPoint[j].y)
				{
					SWAP(blockPoint[j + 1].y, blockPoint[j].y);
					SWAP(blockPoint[j + 1].x, blockPoint[j].x);
				}
			}
		}
	}
	else if (key == RIGHT)
	{
		for (i = 0; i < 3; i++)
		{
			for (j = 0; j < 3 - i; j++)
			{
				if (blockPoint[j + 1].y > blockPoint[j].y)
				{
					SWAP(blockPoint[j + 1].y, blockPoint[j].y);
					SWAP(blockPoint[j + 1].x, blockPoint[j].x);
				}
			}
		}
	}

}

void CleanUp(HWND hwnd)
{
	//释放资源
	ReleaseDC(hwnd, hdc);
	DeleteDC(mdc);
	DeleteDC(bufdc);
	DeleteObject(bmp);
	DeleteObject(voice_bmp);
	//删除定时器
	KillTimer(hwnd, ID_TIMER);
	KillTimer(hwnd, ID_TIMER2);
	//删除音乐文件
	remove("C:\\TetrisRes\\2461");
	remove("C:\\TetrisRes\\2462");
	remove("C:\\TetrisRes\\991");
	remove("C:\\TetrisRes\\992");
	//删除文件夹
	_rmdir("C:\\TetrisRes");

}