#include <ctype.h>
#include <process.h>


TCHAR IniPath[MAX_PATH + 1];
char tcsHotkey[MAX_PATH];
char szClassName[] = "deja_boss_key_window";
int isBossKey = 0;

#define MY_DATA_SIG 0x1354C5B7
#define WM_MYHOTKEY (WM_APP+32)

bool IsSystemWin7()
{
	static int result = -1;

	if(result==-1)
	{
		OSVERSIONINFO osvi;
		osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
		::GetVersionEx(&osvi);
		if (osvi.dwMajorVersion == 6 &&
			osvi.dwMinorVersion >= 1 )
		{
			result = true;
		}
		else result = false;

	}

	return result!=0;
}

void HotKeyRegister(LPVOID pvoid)
{
	//DbgPrint("检查锁");

	HANDLE hMutex = CreateMutex(NULL , TRUE, szClassName);
	if (GetLastError() == ERROR_ALREADY_EXISTS)
	{
		//DbgPrint("等待锁");
		WaitForSingleObject(hMutex,INFINITE);
	}

	//DbgPrint("开始注册热键");
	//OutputDebugStringA("开始注册热键");
	UINT mo = 0;
	UINT vk = 0;

	StringSplit split(tcsHotkey, '+');
	for(int i=0;i<split.GetCount();i++)
	{
		char* str2 = split.GetIndex(i);

		if(_tcsicmp(str2,"Shift")==0) mo |= MOD_SHIFT;
		else if(_tcsicmp(str2,"Ctrl")==0) mo |= MOD_CONTROL;
		else if(_tcsicmp(str2,"Alt")==0) mo |= MOD_ALT;
		else if(_tcsicmp(str2,"Win")==0) mo |= MOD_WIN;

		char wch = str2[0];
		if (_tcslen(str2)==1)
		{
			if(isalnum(wch)) vk = toupper(wch);
			else vk = LOWORD(VkKeyScan(wch));
		}
		else if (wch=='F'||wch=='f')
		{
			if(isdigit(str2[1]))  vk = VK_F1 + _ttoi(&str2[1]) - 1;
		}
		else
		{
			if(_tcsicmp(str2,"Left")==0) vk = VK_LEFT;
			else if(_tcsicmp(str2,"Right")==0) vk = VK_RIGHT;
			else if(_tcsicmp(str2,"Up")==0) vk = VK_UP;
			else if(_tcsicmp(str2,"Down")==0) vk = VK_DOWN;

			else if(_tcsicmp(str2,"End")==0) vk = VK_END;
			else if(_tcsicmp(str2,"Home")==0) vk = VK_HOME;

			else if(_tcsicmp(str2,"Tab")==0) vk = VK_TAB;
			else if(_tcsicmp(str2,"Space")==0) vk = VK_SPACE;

			else if(_tcsicmp(str2,"Esc")==0) vk = VK_ESCAPE;
			else if(_tcsicmp(str2,"Delete")==0) vk = VK_DELETE;

			else if(_tcsicmp(str2,"PageUp")==0) vk = VK_PRIOR;
			else if(_tcsicmp(str2,"PageDown")==0) vk = VK_NEXT;
		}

	}

	if( IsSystemWin7() ) mo |= 0x4000;

	//DbgPrint("%X %X",mo,vk);

	//DbgPrint("消息循环");
	SendMessage((HWND)pvoid,WM_MYHOTKEY,1000,MAKELPARAM(mo,vk));


	MSG msg;
	while (GetMessage(&msg, 0, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	//OutputDebugStringA("退出热键线程");
}


BOOL CALLBACK broadcast(HWND hWnd, LPARAM lParam)
{
	TCHAR buff[256];
	DWORD dwData =GetWindowLong(hWnd,GWL_USERDATA);
	if(dwData==MY_DATA_SIG)
	{
		GetWindowText(hWnd, buff, 255);
		if (_tcscmp(buff, szClassName) == 0) //比较窗口名
		{
			SendMessage(hWnd,WM_MYHOTKEY+1,0,lParam);
		}
	}
	return true;
}


NOTIFYICONDATAW nid;

typedef BOOL (WINAPI* TShell_NotifyIconW)(
									  DWORD dwMessage,
									  PNOTIFYICONDATAW lpData
									  );
TShell_NotifyIconW OldShell_NotifyIconW = NULL;
void HideTray(bool show)
{
	nid.uFlags |= NIF_STATE;
	nid.dwState = show;
	nid.dwStateMask = NIS_HIDDEN;
	if(OldShell_NotifyIconW)  OldShell_NotifyIconW(NIM_MODIFY,&nid);
}
BOOL WINAPI MyShell_NotifyIconW(DWORD dwMessage, PNOTIFYICONDATAW lpData)
{
	////DbgPrint("dwMessage:%d",dwMessage);

	if(dwMessage==NIM_ADD||dwMessage==NIM_MODIFY)
	{
		memcpy(&nid,lpData,lpData->cbSize);
	}

	if(isBossKey)
	{
		HideTray(NIS_HIDDEN);
		return 0;
	}


	if(OldShell_NotifyIconW)  return OldShell_NotifyIconW(dwMessage,lpData);

	return 0;
}

HWND WndList[100];
int now = 0;
BOOL CALLBACK SearchQQ(HWND hWnd, LPARAM lParam)
{
	if(IsWindowVisible(hWnd))
	{
		TCHAR buff[256];
		GetClassName(hWnd, buff, 255);
		if ( _tcscmp(buff, "TXGuiFoundation") == 0) //比较类名
		{
			DWORD pid;
			GetWindowThreadProcessId(hWnd,&pid);
			if(GetCurrentProcessId()==pid)
			{
				if(now<100)
				{
					ShowWindow(hWnd,SW_HIDE);
					WndList[now] = hWnd;
					now++;
				}
			}
		}
	}
	return true;
}

void RestoreQQ()
{
	for(int i = now - 1;i>=0;i--)
	{
		HWND hWnd = WndList[i];
		ShowWindow(hWnd,SW_SHOW);
		//SetForegroundWindow(hWnd);
		//if(IsIconic(hWnd)) ShowWindow(hWnd,SW_RESTORE);
	}
	now = 0;
}

void HideAllWindow()
{
	if(isBossKey)
	{
		//隐藏
		HideTray(NIS_HIDDEN);
		EnumWindows(SearchQQ,0);
	}
	else
	{
		//还原
		HideTray(0);
		RestoreQQ();
	}
}
BOOL CALLBACK BossKey(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_INITDIALOG:
        ShowWindow(hWnd,SW_HIDE);
		SetWindowText(hWnd,szClassName);
		SetWindowLong(hWnd,GWL_USERDATA,MY_DATA_SIG);
        _beginthread(HotKeyRegister,0,(LPVOID)hWnd);
        OldShell_NotifyIconW = (TShell_NotifyIconW)he.InstallHook((DWORD)GetFunc("SHELL32.dll","Shell_NotifyIconW"), (DWORD)MyShell_NotifyIconW);
        break;
	case WM_MYHOTKEY:
        //DbgPrint("WM_MYHOTKEY");
		if(!RegisterHotKey(hWnd,wParam,LOWORD(lParam), HIWORD(lParam)))
		{
			Sleep(300);
			char Tips[512];
			wsprintf(Tips,"快捷键 \"%s\" 注册失败！请更改你的老板键设置。",tcsHotkey);
			MessageBox(hWnd,Tips,"deja",MB_OK | MB_ICONWARNING | MB_TOPMOST);
		}
		break;
	case WM_MYHOTKEY + 1:
		isBossKey = lParam;
		HideAllWindow();
		break;
	case WM_HOTKEY:
		isBossKey = !isBossKey;
		EnumWindows(broadcast,isBossKey);
		break;
    }
    return 0;
}

typedef int (__cdecl * GetRegSubKeyBoolField) (wchar_t *,wchar_t *,wchar_t *,int *,DWORD ,DWORD);
GetRegSubKeyBoolField GetRegBool = NULL;
int __cdecl MyGetRegBool(wchar_t *p1,wchar_t *p2,wchar_t *p3,int *p4,DWORD p5,DWORD p6)
{
    if(isBossKey)
    {
        if(wcscmp(p3,L"bEnableAllSound")==0
        || wcscmp(p3,L"bAutoPopupChatWnd")==0
        || wcscmp(p3,L"bEnableShakeWindowTip")==0
        )
        {
            int res = GetRegBool?GetRegBool(p1,p2,p3,p4,p5,p6):0;
            *p4 = 0;
            return res;
        }
    }

    if(GetRegBool) return GetRegBool(p1,p2,p3,p4,p5,p6);
    return 0;
}

//
bool RAMCollection = false;
bool LocalVIP = false;
bool DisablePopupTip = false;
//

typedef long (__cdecl * MYRawCreateGFElementByXtml) (wchar_t *,DWORD,DWORD,DWORD);
MYRawCreateGFElementByXtml RawCreateGFElementByXtml = NULL;
long __cdecl MyRawCreateGFElementByXtml(wchar_t  *p1,DWORD p2,DWORD p3,DWORD p4)
{
    if((isBossKey || DisablePopupTip) && wcsstr(p1,L"PopupTip.xml")!=0) return 0;

    if(RawCreateGFElementByXtml) return RawCreateGFElementByXtml(p1,p2,p3,p4);
    return 0;
}

typedef unsigned long (__cdecl * MYGETSELFUIN) (void);
MYGETSELFUIN GetSelfUin = 0;

TCHAR * GetCallStack(void *param)
{
	static TCHAR stack[MAX_PATH+1];
	MEMORY_BASIC_INFORMATION mbi;
	VirtualQuery((LPVOID)*((DWORD*)param-1), &mbi, sizeof(mbi));
	GetModuleFileName((HMODULE)mbi.AllocationBase, stack, MAX_PATH);

	//DbgPrint(L"%X",((DWORD*)param-1));
	return stack;
}

// 本地会员
typedef int (__cdecl * MYISFLAGVALID) (unsigned long, unsigned long);
MYISFLAGVALID VipOldProc = NULL;
int __cdecl MyIsFlagValid(unsigned long QQUIN, unsigned long Flag)
{
	if ( Flag == 4 && LocalVIP && QQUIN == GetSelfUin() && !isEndWith(GetCallStack(&QQUIN), "MsgMgr.dll"))
	{
		return 1;
	}
	// 调用原来的函数
	if(VipOldProc) return VipOldProc(QQUIN, Flag);

	return 0;
}

// 获取服务器开关
typedef int (__cdecl * QQIsServerControlBitOn) (unsigned long);
QQIsServerControlBitOn OldIsServerControlBitOn = NULL;
int __cdecl IsServerControlBitOn(unsigned long FLAG)
{
	switch(FLAG)
	{
		//case 46:	return 0;
		//case 48:	return 0;
		case 131:	return 0;//QQ秀
		//case 163:	return 1;//升级
		//case 166:	return 1;
	}

	int ret = 0;
	//if(isEndWith(GetCallStack(&FLAG),L"QQShow.dll")) return 0;
	if(OldIsServerControlBitOn) ret = OldIsServerControlBitOn(FLAG);

	//DbgPrint(L"%d:%d %s",FLAG,ret,GetCallStack(&FLAG));
	return ret;
}

void MemoryClear()
{
    if (RAMCollection)
    {
        SetProcessWorkingSetSize(GetCurrentProcess(), (DWORD) - 1, (DWORD) - 1);
    }
}

EXTERN_C void speak_init();

EXTERN_C void speak(wchar_t *text);

void CWUB3_Thread(PVOID pvoid)
{
    speak_init();
    // 读取配置
    TCHAR config[MAX_PATH];

    GetPrivateProfileString(_T("others"), _T("RAMCollection"), _T(""), config, MAX_PATH, IniPath);
    if (_tcsicmp(config,_T("enabled"))==0) RAMCollection = true;

    GetPrivateProfileString(_T("others"), _T("LocalVIP"), _T(""), config, MAX_PATH, IniPath);
    if (_tcsicmp(config,_T("enabled"))==0) LocalVIP = true;

    GetPrivateProfileString(_T("others"), _T("DisablePopupTip"), _T(""), config, MAX_PATH, IniPath);
    if (_tcsicmp(config,_T("enabled"))==0) DisablePopupTip = true;
    //
    SetTimer(NULL, 0, 1000*60, (TIMERPROC)MemoryClear);//60秒一次

    GetSelfUin = (MYGETSELFUIN)GetFunc("KernelUtil.dll", "?GetSelfUin@Contact@Util@@YAKXZ");
    VipOldProc = (MYISFLAGVALID)he.InstallHook((DWORD)GetFunc("KernelUtil.dll", "?IsFlagValid@Contact@Util@@YAHKK@Z"), (DWORD)MyIsFlagValid);
    //OldIsServerControlBitOn = (QQIsServerControlBitOn)he.InstallHook((DWORD)GetFunc("KernelUtil.dll", "?IsServerControlBitOn@Misc@Util@@YAHK@Z"), (DWORD)IsServerControlBitOn);

    GetRegBool = (GetRegSubKeyBoolField)he.InstallHook((DWORD)GetFunc("AppUtil.dll","?GetRegSubKeyBoolField@API@Registry@@YAHPA_W00PAHW4__MIDL___MIDL_itf_IRegistry_0000_0003@@W4__MIDL___MIDL_itf_IRegistry_0000_0004@@@Z"),(DWORD)MyGetRegBool);

    RawCreateGFElementByXtml = (MYRawCreateGFElementByXtml)he.InstallHook((DWORD)GetFunc("GF.dll","?RawCreateGFElementByXtml@GF@Util@@YAJPA_WPAPAUIGFElement@@PAU3@0H@Z"),(DWORD)MyRawCreateGFElementByXtml);
    if(RawCreateGFElementByXtml==0)
    {
        RawCreateGFElementByXtml = (MYRawCreateGFElementByXtml)he.InstallHook((DWORD)GetFunc("GF.dll","?RawCreateGFElementByXtml@GF@Util@@YAJPA_WPAPAUIGFElement@@PAU3@0@Z"),(DWORD)MyRawCreateGFElementByXtml);
    }

    if(GetPrivateProfileString(_T("others"), _T("BossKey"), _T(""), tcsHotkey, MAX_PATH, IniPath))
    {
        CreateDialog((HINSTANCE)pvoid,MAKEINTRESOURCE(101),NULL,BossKey);
    }

    MSG msg;
    while (GetMessage(&msg, 0, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}
