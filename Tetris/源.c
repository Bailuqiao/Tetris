#include <Windows.h>
#include <time.h>
#include <strsafe.h> 
#include <io.h>			//_access()ͷ�ļ�
#include <direct.h>		//_rmdir()ͷ�ļ�

#include"resource.h"

#pragma comment(lib,"Winmm.lib")

//�ö�ʱ���������䣬�ڼ���Ե�������
//1-7���������е�7�з��飬8-14��������صķ���;

//Ϊ�˼�࣬����ʱ����Դ�ļ�һ������ȥ����Ƶ�ļ�������ʱ������C:\TetrisResĿ¼���ٶ�ȡ�ģ������˳�������Զ�����
//����bgm��Դqq���֣���Ч�ļ���Դ����


#define DOWN_SPEED       50				//�����ٶȣ��������

#define SWAP(a,b)  (a=a+b,b=a-b,a=a-b)	//������������ֵ��a+b�Ĵ洢�ռ䲻�������a��

#define ID_TIMER	      1				//��������Ķ�ʱ��ID
#define ID_TIMER2         2				//��������BGM�Ƿ񲥷����

#define LEFT		      1				//��
#define RIGHT		      2				//��
#define UP			      3				//�ϣ���ת��

#define CLIENT_X          (360+210)	    //�ͻ�����
#define CLIENT_Y          630	    	//�ͻ�����
#define BLOCK             30	 		//����ı߳�
#define X_BOLCK_NUM       10			//x���򷽿�����
#define Y_BOLCK_NUM	      20			//y���򷽿�����

#define MAP_START         0				//��ʼ����  
#define MAP_INGAME		  1				//��Ϸ�н���
#define MAP_GAME_OVER     2				//��Ϸ��������
#define MAP_PAUSE		  3				//��Ϸ��ͣ

#define KD_UP             0				//�·�����ɿ�
#define KD_DOWN           1				//�·��������

#define NB_BGM1           1				//BGM1
#define NB_BGM2           2				//BGM2

HDC hdc, mdc, bufdc;					//hdc:��ʾ���豸������� mdc:�����õ��豸��������������� buf
HBITMAP bmp, voice_bmp;
int num = 0;
int key = 0;							//������Ϣ
int map = MAP_START;					//Ŀǰδ�����������棬ֻ����Ϸ�к���Ϸ��������
int keyDown = KD_UP;					//�·����״̬
int nextBlock;							//��һ������
int score = 0;							//����
BOOL music = TRUE;						//�Ƿ���Ч
int nowBgm = -1;						//��ǰ���ڲ��ŵ�bgm


//�洢ȫ������
int a[20][10] = { 0 };

//�洢������4������λ��
POINT blockPoint[4] = { 0 };

LRESULT CALLBACK WinProc(HWND, UINT, WPARAM, LPARAM);
void Init(HWND, HINSTANCE);	//��ʼ��
void Paint(HWND);		//���ƻ���
void BlockDown(HWND hwnd);		//��������
void BlockMove();		//���������ƶ�
void Sort();			//��blockPoint[4]��Ԫ����������
BOOL RandBlock();		//������෽��,����ֵ�ж���Ϸ�Ƿ������TRUE������FALSEδ����
BOOL Remove();			//��������
void CleanUp(HWND);		//������Ϸ��Դ

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
		MessageBox(NULL, TEXT("���������Ҫ�� Windows NT �������У�"), szAppName, MB_ICONERROR);
		return 0;
	}

	hwnd = CreateWindow(szAppName,
		TEXT("����˹����"),
		WS_OVERLAPPEDWINDOW ^ WS_MAXIMIZEBOX ^ WS_THICKFRAME,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		640,		//���ڿ�
		480,		//���ڸ�
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
		Init(hwnd, ((LPCREATESTRUCT)lParam)->hInstance);		//��ʼ��
		SetTimer(hwnd, ID_TIMER, DOWN_SPEED, NULL);				//��������
		SetTimer(hwnd, ID_TIMER2, 20, NULL);					//����bgm�Ƿ񲥷����
		return 0;
	case WM_TIMER:
		
		if (wParam == ID_TIMER)
		{
			//��ʼ����
			if (map == MAP_START)
			{
				HBRUSH hBrush = CreateSolidBrush(RGB(0, 0, 0));
				GetClientRect(hwnd, &rect);
				FillRect(mdc, &rect, hBrush);
				HFONT hOldFont, hFont = CreateFont(40, 20, 0, 0, 0, 0, 0, 0, GB2312_CHARSET, 0, 0, 0, 0, TEXT("MyFont"));
				hOldFont = SelectObject(mdc, hFont);
				SetTextColor(mdc, RGB(0xFF, 0xFF, 0xFF));
				TextOut(mdc, 210, 250, TEXT("��ʼ��Ϸ"), 4);
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

			//��Ϸ�н���
			if (map == MAP_INGAME)
			{

				//ʵ�ְ��·�������ٹ���
				//��һ���¼���ʹ��������һ��
				//����ʱ��keyDownһֱ����KD_DOWN��������������
				//�����¼�ʱ����ʱ����ʹnum��ֵ��[0,8]֮��ѭ������num=0ʱ������������һ��
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
			//����BGM���沥��
			if (nowBgm == NB_BGM1)
			{
				mciSendString(TEXT("status bgm1 mode"), str, sizeof(str), NULL);
				if (_wcsicmp(str, TEXT("stopped")) == 0)
				{
					//���������ϣ��Ͳ�����һ��BGM
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
					//���������ϣ��Ͳ�����һ��BGM
					mciSendString(TEXT("close bgm1"), NULL, 0, NULL);
					mciSendString(TEXT("open C:\\TetrisRes\\2461 type MPEGVideo alias bgm1"), NULL, 0, NULL);
					mciSendString(TEXT("play bgm1"), NULL, 0, NULL);
					nowBgm = NB_BGM1;
				}
			}

		}
		return 0;
	case WM_MOUSEMOVE:
		//����ڿ�ʼ���������ϱ����
		if (map == MAP_START && (LOWORD(lParam) > 210 && LOWORD(lParam) < 380 && HIWORD(lParam) > 250 && HIWORD(lParam) < 290 || LOWORD(lParam) > 270 && LOWORD(lParam) < 320 && HIWORD(lParam) > 320 && HIWORD(lParam) < 370))
		{
			SetCursor(LoadCursor(NULL, IDC_HAND));
		}

		return 0;
	case WM_LBUTTONUP:
		//�����ʼ��Ϸ
		if (map == MAP_START && LOWORD(lParam) > 210 && LOWORD(lParam) < 380 && HIWORD(lParam) > 250 && HIWORD(lParam) < 290)
		{
			map = MAP_INGAME;
			if (music)
			{
				//��ʼ�������bgm
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

		//�������ͼ��
		if (map == MAP_START && LOWORD(lParam) > 270 && LOWORD(lParam) < 320 && HIWORD(lParam) > 320 && HIWORD(lParam) < 370)
		{
			//��ǰ����������ر�����
			if (music)
			{
				music = FALSE;
				mciSendString(TEXT("close all"), NULL, 0, NULL);
			}
			//��֮
			else
			{
				music = TRUE;
			}
		}
		//��Ϸ��ʼ�е���ͣ��ť
		if (map != MAP_START && ((LOWORD(lParam) > (X_BOLCK_NUM + 3.5)*BLOCK && LOWORD(lParam) < (X_BOLCK_NUM + 6.5)*BLOCK && HIWORD(lParam) > 14.5 * BLOCK && HIWORD(lParam) < 15.5 * BLOCK)))
		{
			//��ǰ��Ϸ��ʼ�У�����ͣ��Ϸ����ͣbgm
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
			//��֮
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
			//ˢ��һ�ν���
			Paint(hwnd);
		}
		//��Ϸ��ʼ�л���ͣʱ�������
		if ((map == MAP_INGAME || map == MAP_PAUSE) && LOWORD(lParam) > (X_BOLCK_NUM + 3.5)*BLOCK && LOWORD(lParam) < (X_BOLCK_NUM + 6.5)*BLOCK && HIWORD(lParam) > 16.5 * BLOCK && HIWORD(lParam) < 17.5 * BLOCK)
		{
			int temp = map;		//��¼�µ�ǰ��Ϸ״̬
			map = MAP_PAUSE;	//����������������
			if (MessageBox(hwnd, TEXT("��ǰ���Ȼᶪʧ��ȷ��Ҫ�������˵���"), TEXT("����"), MB_YESNO | MB_ICONEXCLAMATION) == IDYES)
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
			//��ԭ״̬
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
		//WM_PAINT ��Ϣ�е��豸�������ֻ��ʹ��BeginPaint()���ص�
		//�ұ�����EndPaint()
		//�����һֱ�յ�WM_PAINT��Ϣ�����³��������
		pdc = BeginPaint(hwnd, &ps);

		//��ӦWM_PAINT��Ϣ�������������豸������������Ļ
		BitBlt(pdc, 0, 0, CLIENT_X, CLIENT_Y, mdc, 0, 0, SRCCOPY);

		EndPaint(hwnd, &ps);
		return 0;
	case WM_DESTROY:
		//�����˳�ǰ�����
		CleanUp(hwnd);
		PostQuitMessage(0);
		return 0;
	}

	return DefWindowProc(hwnd, message, wParam, lParam);
}

void Init(HWND hwnd, HINSTANCE hInstance)
{
	//���ھ��С��ı��С
	RECT rcWindow, rcClinet;
	int winX, winY;
	GetWindowRect(hwnd, &rcWindow);
	GetClientRect(hwnd, &rcClinet);
	winX = rcWindow.right - rcWindow.left - (rcClinet.right - rcClinet.left) + CLIENT_X;
	winY = rcWindow.bottom - rcWindow.top - (rcClinet.bottom - rcClinet.top) + CLIENT_Y;
	MoveWindow(hwnd, (GetSystemMetrics(SM_CXSCREEN) - winX) / 2, (GetSystemMetrics(SM_CYSCREEN) - winY) / 2, winX, winY, TRUE);

	//��������ʼ��hdc
	hdc = GetDC(hwnd);
	mdc = CreateCompatibleDC(hdc);
	bufdc = CreateCompatibleDC(hdc);
	bmp = CreateCompatibleBitmap(hdc, CLIENT_X, CLIENT_Y);
	SelectObject(mdc, bmp);

	//͸�����ֱ���
	SetBkMode(mdc, TRANSPARENT);

	//����λͼ
	bmp = LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_BITMAP1));				//����
	voice_bmp = LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_BITMAP2));		//����ͼ��

	//�����ļ���
	char folderName[] = "C:\\TetrisRes";
	//���ļ��в������򴴽�����ֹ�ϴ������������û��ɾ��Ŀ¼��
	if (_access(folderName, 0) == -1)
	{
		_mkdir(folderName);
	}
	
	HRSRC hResource;
	HGLOBAL hg;
	LPVOID pDate;
	DWORD dwSize;
	FILE *fp;

	//��C:\TetrisRes��д����Ƶ�ļ����������˳�������Զ�ɾ��

	//�ҵ���Դ�ļ�
	hResource = FindResource(NULL, MAKEINTRESOURCE(IDR_MP31), TEXT("MP3"));
	//������Դ�ļ�
	hg = LoadResource(NULL, hResource);
	//������Դ�ļ�
	pDate = LockResource(hg);
	//��ȡ��Դ�ļ���С
	dwSize = SizeofResource(NULL, hResource);
	//����Դ�ļ�д�����
	fp = fopen("C:\\TetrisRes\\2461", "wb");
	fwrite(pDate, sizeof(char), dwSize, fp);
	fclose(fp);

	//�ҵ���Դ�ļ�
	hResource = FindResource(NULL, MAKEINTRESOURCE(IDR_MP32), TEXT("MP3"));
	//������Դ�ļ�
	hg = LoadResource(NULL, hResource);
	//������Դ�ļ�
	pDate = LockResource(hg);
	//��ȡ��Դ�ļ���С
	dwSize = SizeofResource(NULL, hResource);
	//����Դ�ļ�д�����
	fp = fopen("C:\\TetrisRes\\2462", "wb");
	fwrite(pDate, sizeof(char), dwSize, fp);
	fclose(fp);

	//�ҵ���Դ�ļ�
	hResource = FindResource(NULL, MAKEINTRESOURCE(IDR_WAVE1), TEXT("WAVE"));
	//������Դ�ļ�
	hg = LoadResource(NULL, hResource);
	//������Դ�ļ�
	pDate = LockResource(hg);
	//��ȡ��Դ�ļ���С
	dwSize = SizeofResource(NULL, hResource);
	//����Դ�ļ�д�����
	fp = fopen("C:\\TetrisRes\\991", "wb");
	fwrite(pDate, sizeof(char), dwSize, fp);
	fclose(fp);

	//�ҵ���Դ�ļ�
	hResource = FindResource(NULL, MAKEINTRESOURCE(IDR_WAVE2), TEXT("WAVE"));
	//������Դ�ļ�
	hg = LoadResource(NULL, hResource);
	//������Դ�ļ�
	pDate = LockResource(hg);
	//��ȡ��Դ�ļ���С
	dwSize = SizeofResource(NULL, hResource);
	//����Դ�ļ�д�����
	fp = fopen("C:\\TetrisRes\\992", "wb");
	fwrite(pDate, sizeof(char), dwSize, fp);
	fclose(fp);


	//�������
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

	//����ɫ
	GetClientRect(hwnd, &rect);
	FillRect(mdc, &rect, hBrush);
	
	//��߿�
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

	//�����ұ�
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

	//����
	rect.left = (X_BOLCK_NUM + 2)*BLOCK;
	rect.right = rect.left + 6 * BLOCK;
	rect.top = 11 * BLOCK;
	rect.bottom = rect.top + 2 * BLOCK;
	FillRect(mdc, &rect, hBrush);
	rect.bottom = rect.top + BLOCK;
	DrawText(mdc, TEXT("�÷�"), -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
	rect.top = 12 * BLOCK;
	rect.bottom = rect.top + BLOCK;
	StringCchPrintf(buff, 128, TEXT("%d"), score);
	DrawText(mdc, buff, -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

	//��һ������Ļ���
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

	//д��ϻ�
	rect.left = (X_BOLCK_NUM + 2)*BLOCK;
	rect.right = rect.left + 6 * BLOCK;
	rect.top = 19 * BLOCK;
	rect.bottom = rect.top + BLOCK;
	FillRect(mdc, &rect, hBrush);
	DrawText(mdc, TEXT("���ĳ�Ʒ��������Ʒ"), -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

	//��ť�ռ�
	rect.left = (X_BOLCK_NUM + 2)*BLOCK;
	rect.right = rect.left + 6 * BLOCK;
	rect.top = 14 * BLOCK;
	rect.bottom = rect.top + 4 * BLOCK;
	FillRect(mdc, &rect, hBrush);
	//���ư�ť
	hBrush = CreateSolidBrush(RGB(0xA0, 0xA0, 0xA0));
	rect.left = (X_BOLCK_NUM + 3.5)*BLOCK;
	rect.right = rect.left + 3 * BLOCK;
	rect.top = 14.5 * BLOCK;
	rect.bottom = rect.top + BLOCK;
	FillRect(mdc, &rect, hBrush);
	if (map != MAP_PAUSE)
	{
		DrawText(mdc, TEXT("��ͣ"), -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
	}
	else
	{
		DrawText(mdc, TEXT("����"), -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
	}
	rect.left = (X_BOLCK_NUM + 3.5)*BLOCK;
	rect.right = rect.left + 3 * BLOCK;
	rect.top = 16.5 * BLOCK;
	rect.bottom = rect.top + BLOCK;
	FillRect(mdc, &rect, hBrush);
	DrawText(mdc, TEXT("����"), -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

	//���Ʒ���
	int n = 0;
	for (x = Y_BOLCK_NUM - 1; x >= 0; x--)			//xΪ����
	{
		for (y = 0; y < X_BOLCK_NUM; y++)		//yΪ����
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
				blockPoint[n].x = x;			//������
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
	//����Ϊ�ƶ�����ת��׼��
	//��ת����Ϊ��������
	Sort();

	//�Ƿ����ƶ�
	BOOL flag = TRUE;

	int i;
	switch (key)
	{
	case LEFT:
		//�ж��ܷ��ƶ�
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
	case UP:		//��ת
		switch (a[blockPoint[0].x][blockPoint[0].y])			//���ݷ������ͣ�������������������Ч���Ҳ��������÷����ˣ�
		{
		case 1:			//���ַ��飬������ת
			break;
		case 2:
			if (blockPoint[0].x == blockPoint[1].x && blockPoint[1].x == blockPoint[2].x && blockPoint[2].x == blockPoint[3].x)
			{
				//�ж��Ƿ������ת
				if (a[blockPoint[1].x - 1][blockPoint[1].y] == 0 && a[blockPoint[1].x + 1][blockPoint[1].y] == 0 && a[blockPoint[1].x + 2][blockPoint[1].y] == 0 && blockPoint[1].x >= 1 && blockPoint[1].x + 2 < Y_BOLCK_NUM)
				{
					a[blockPoint[1].x - 1][blockPoint[1].y] = a[blockPoint[1].x + 1][blockPoint[1].y] = a[blockPoint[1].x + 2][blockPoint[1].y] = 2;
					a[blockPoint[0].x][blockPoint[0].y] = a[blockPoint[2].x][blockPoint[2].y] = a[blockPoint[3].x][blockPoint[3].y] = 0;
				}
			}
			else if (blockPoint[0].y == blockPoint[1].y && blockPoint[1].y == blockPoint[2].y && blockPoint[2].y == blockPoint[3].y)
			{
				//�ж��Ƿ������ת
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
				//�ж��Ƿ������ת
				if (a[blockPoint[1].x + 1][blockPoint[1].y] == 0 && blockPoint[1].x + 1 < Y_BOLCK_NUM)
				{
					a[blockPoint[1].x + 1][blockPoint[1].y] = 3;
					a[blockPoint[0].x][blockPoint[0].y] = 0;
				}
			}
			else if (blockPoint[0].y == blockPoint[1].y && blockPoint[1].y == blockPoint[2].y && blockPoint[3].x == blockPoint[1].x && blockPoint[3].y > blockPoint[1].y)
			{
				//�ж��Ƿ������ת
				if (a[blockPoint[1].x][blockPoint[1].y - 1] == 0 && blockPoint[1].y - 1 >= 0)
				{
					a[blockPoint[1].x][blockPoint[1].y - 1] = 3;
					a[blockPoint[2].x][blockPoint[2].y] = 0;
				}
			}
			else if (blockPoint[0].x == blockPoint[2].x && blockPoint[2].x == blockPoint[3].x && blockPoint[2].y == blockPoint[1].y && blockPoint[1].x > blockPoint[2].x)
			{
				//�ж��Ƿ������ת
				if (a[blockPoint[2].x - 1][blockPoint[2].y] == 0 && blockPoint[1].x - 1 >= 0)
				{
					a[blockPoint[2].x - 1][blockPoint[2].y] = 3;
					a[blockPoint[3].x][blockPoint[3].y] = 0;
				}
			}
			else
			{
				//�ж��Ƿ������ת
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
				//�ж��Ƿ������ת
				if (a[blockPoint[0].x + 1][blockPoint[0].y] == 0 && a[blockPoint[1].x - 1][blockPoint[1].y] == 0 && a[blockPoint[1].x + 1][blockPoint[1].y] == 0 && blockPoint[0].x + 1 < Y_BOLCK_NUM && blockPoint[1].x - 1 >= 0)
				{
					a[blockPoint[0].x + 1][blockPoint[0].y] = a[blockPoint[1].x - 1][blockPoint[1].y] = a[blockPoint[1].x + 1][blockPoint[1].y] = 4;
					a[blockPoint[0].x][blockPoint[0].y] = a[blockPoint[2].x][blockPoint[2].y] = a[blockPoint[3].x][blockPoint[3].y] = 0;
				}
			}
			else if (blockPoint[1].y == blockPoint[2].y && blockPoint[2].y == blockPoint[3].y && blockPoint[1].y > blockPoint[0].y && blockPoint[1].x == blockPoint[0].x)
			{
				//�ж��Ƿ������ת
				if (a[blockPoint[3].x][blockPoint[3].y - 1] == 0 && a[blockPoint[2].x][blockPoint[2].y - 1] == 0 && a[blockPoint[2].x][blockPoint[2].y + 1] == 0 && blockPoint[2].y + 1 < X_BOLCK_NUM && blockPoint[2].y - 1 >= 0)
				{
					a[blockPoint[3].x][blockPoint[3].y - 1] = a[blockPoint[2].x][blockPoint[2].y - 1] = a[blockPoint[2].x][blockPoint[2].y + 1] = 4;
					a[blockPoint[0].x][blockPoint[0].y] = a[blockPoint[1].x][blockPoint[1].y] = a[blockPoint[3].x][blockPoint[3].y] = 0;
				}
			}
			else if (blockPoint[0].x == blockPoint[2].x && blockPoint[2].x == blockPoint[3].x && blockPoint[0].y == blockPoint[1].y && blockPoint[0].x > blockPoint[1].x)
			{
				//�ж��Ƿ������ת
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
				//�ж��Ƿ������ת
				if (a[blockPoint[1].x - 1][blockPoint[1].y] == 0 && a[blockPoint[2].x - 1][blockPoint[2].y] == 0 && a[blockPoint[2].x + 1][blockPoint[2].y] == 0 && blockPoint[2].x + 1 < Y_BOLCK_NUM && blockPoint[2].x - 1 >= 0)
				{
					a[blockPoint[1].x - 1][blockPoint[1].y] = a[blockPoint[2].x - 1][blockPoint[2].y] = a[blockPoint[2].x + 1][blockPoint[2].y] = 5;
					a[blockPoint[0].x][blockPoint[0].y] = a[blockPoint[1].x][blockPoint[1].y] = a[blockPoint[3].x][blockPoint[3].y] = 0;
				}
			}
			else if (blockPoint[1].y == blockPoint[2].y && blockPoint[2].y == blockPoint[3].y && blockPoint[3].y > blockPoint[0].y && blockPoint[3].x == blockPoint[0].x)
			{
				//�ж��Ƿ������ת
				if (a[blockPoint[3].x][blockPoint[3].y + 1] == 0 && a[blockPoint[2].x][blockPoint[2].y - 1] == 0 && a[blockPoint[2].x][blockPoint[2].y + 1] == 0 && blockPoint[2].y + 1 < X_BOLCK_NUM && blockPoint[2].y - 1 >= 0)
				{
					a[blockPoint[3].x][blockPoint[3].y + 1] = a[blockPoint[2].x][blockPoint[2].y - 1] = a[blockPoint[2].x][blockPoint[2].y + 1] = 5;
					a[blockPoint[0].x][blockPoint[0].y] = a[blockPoint[1].x][blockPoint[1].y] = a[blockPoint[3].x][blockPoint[3].y] = 0;
				}
			}
			else if (blockPoint[0].x == blockPoint[2].x && blockPoint[2].x == blockPoint[1].x && blockPoint[2].y == blockPoint[3].y && blockPoint[2].x > blockPoint[3].x)
			{
				//�ж��Ƿ������ת
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
				//�ж��Ƿ������ת
				if (a[blockPoint[0].x + 1][blockPoint[0].y] == 0 && a[blockPoint[2].x - 1][blockPoint[2].y] == 0 && blockPoint[2].x - 1 >= 0)
				{
					a[blockPoint[0].x + 1][blockPoint[0].y] = a[blockPoint[2].x - 1][blockPoint[2].y] = 6;
					a[blockPoint[3].x][blockPoint[3].y] = a[blockPoint[1].x][blockPoint[1].y] = 0;
				}
			}
			else
			{
				//�ж��Ƿ������ת
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
				//�ж��Ƿ������ת
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

	//����Ƿ��������
	for (i = 0; i < 4; i++)
	{
		if (!(a[blockPoint[i].x + 1][blockPoint[i].y] < 8 && blockPoint[0].x < Y_BOLCK_NUM - 1))
		{
			flag_down = FALSE;
		}
	}

	if (flag_down)			//������Ե���
	{
		for (i = 0; i < 4; i++)
		{
			SWAP(a[blockPoint[i].x + 1][blockPoint[i].y], a[blockPoint[i].x][blockPoint[i].y]);
		}
	}
	else
	{
		//��Ϊ���״̬
		for (i = 0; i < 4; i++)
		{
			a[blockPoint[i].x][blockPoint[i].y] += 7;
		}

		//��������
		Remove();
		if (RandBlock())			//TURE����Ϸ����
		{
			map = MAP_GAME_OVER;
			if (music)
			{
				mciSendString(TEXT("close all"), NULL, 0, NULL);
				mciSendString(TEXT("open C:\\TetrisRes\\991 type MPEGVideo alias sound1"), NULL, 0, NULL);
				mciSendString(TEXT("play sound1"), NULL, 0, NULL);
			}
			MessageBox(hwnd, TEXT("GAME OVER!"), TEXT("���ź�"), MB_OK);
			//ֹͣ��������
			mciSendString(TEXT("stop all"), NULL, 0, NULL);
			mciSendString(TEXT("close all"), NULL, 0, NULL);

			MessageBox(hwnd, TEXT("���� ��������ĵķ����᲻�ᱣ��������\n�⻹�����𣬵�Ȼ�ǲ�������\n�������Ҫ�����̿��־ͺ�(��ͼ�����Լ�������)"), TEXT("����ʲô����"), MB_YESNO);
			

			//��Ϸ���������ݳ�ʼ��
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

//����TRUE��Ϸ����
BOOL RandBlock()
{
	switch (nextBlock)
	{
	case 0:
		if (!(a[0][4] || a[0][5] || a[1][4] || a[1][5]))				//��������λ��Ϊ�գ���Ϸδ����������FALSE
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
	BOOL flag_clear = FALSE;	//����Ƿ�����������
	BOOL flag_X = TRUE;			//ĳ���Ƿ��������
	int clear_x[4];				//����ͬʱ����4��

	//�ҵ���������������clear_x
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

	//�ӷ���
	score += n;

	//����clear_x��
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

		//ʹ���淽�����
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
	//�ͷ���Դ
	ReleaseDC(hwnd, hdc);
	DeleteDC(mdc);
	DeleteDC(bufdc);
	DeleteObject(bmp);
	DeleteObject(voice_bmp);
	//ɾ����ʱ��
	KillTimer(hwnd, ID_TIMER);
	KillTimer(hwnd, ID_TIMER2);
	//ɾ�������ļ�
	remove("C:\\TetrisRes\\2461");
	remove("C:\\TetrisRes\\2462");
	remove("C:\\TetrisRes\\991");
	remove("C:\\TetrisRes\\992");
	//ɾ���ļ���
	_rmdir("C:\\TetrisRes");

}