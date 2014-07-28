#include "main.h"

int GetInternalFunction(lua_State *L)
{
    lua_newtable(L);
    int count = 0;

    lua_pushstring(L,"mem");
    lua_newtable(L);
    count = 1;
    PushFunction(L, count, SearchMemorySignature);
    PushFunction(L, count, SearchModuleSignature);
    PushFunction(L, count, WritePatch);
    PushFunction(L, count, WriteJMP);
    PushFunction(L, count, GetFunc);
    PushFunction(L, count, InstallHook2);
    lua_settable(L, -3);

    lua_pushstring(L,"msg");
    lua_newtable(L);
    count = 1;
    PushFunction(L, count, OutputDebugStringW);
    PushFunction(L, count, speak);
    lua_settable(L, -3);

    lua_pushstring(L,"skin");
    lua_newtable(L);
    count = 1;
    PushFunction(L, count, SkinsPatchHandle);
    PushFunction(L, count, update_rdb);
    lua_settable(L, -3);

    return 1;
}

char plugins[100][256];
int plugins_handle[100];
int plugins_count = 0;
int RegisterPlugin(lua_State *L)
{
    if (plugins_count<100)
    {
        strcpy(plugins[plugins_count], lua_tostring(L, 1) );
        plugins_count++;
    }
    return 0;
}

void DoPlugins(lua_State *L)
{
    for(int i=0;i<plugins_count;i++)
    {
        lua_getglobal(L, plugins[i]);
        if (lua_isfunction(L,-1))
        {
            lua_pcall(L, 0, 1, 0);
            if (lua_istable(L,-1))
            {
                plugins_handle[i] = luaL_ref(L, LUA_REGISTRYINDEX);
                lua_rawgeti(L , LUA_REGISTRYINDEX , plugins_handle[i]);
                lua_getfield(L, -1, "Execute");
                if (lua_isfunction(L,-1))
                {
                    lua_pcall(L, 0, 0, 0);
                }
            }
        }
    }
}

static int GetHTTP(lua_State* L)
{
    //int ref = luaL_ref(L, LUA_REGISTRYINDEX);

    const char *Url = lua_tostring(L, 2);
    const char *Request = lua_tostring(L, 3);
    int method = lua_tonumber(L, 4);

    BYTE *m_data = 0;

    //解析url
    char host[MAX_PATH+1];
    char path[MAX_PATH+1];

    URL_COMPONENTSA uc = { 0};
    uc.dwStructSize = sizeof(uc);

    uc.lpszHostName = host;
    uc.dwHostNameLength = MAX_PATH;

    uc.lpszUrlPath = path;
    uc.dwUrlPathLength = MAX_PATH;

    ::InternetCrackUrlA(Url, 0, ICU_ESCAPE, &uc);

    if(method==0)
    {
        strcat(path, Request);
    }

    HINTERNET hInet = ::InternetOpenA("Mozilla/5.0 (compatible; MSIE 9.0; Windows NT 6.1; Trident/5.0)", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
    if (hInet)
    {
        HINTERNET hConn = ::InternetConnectA(hInet, host, uc.nPort, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 1);
        if (hConn)
        {
            HINTERNET hRes = ::HttpOpenRequestA(hConn, (method==0)?"GET":"POST", path, 0, NULL, NULL, INTERNET_FLAG_DONT_CACHE | INTERNET_FLAG_KEEP_CONNECTION, 1);
            if (hRes)
            {
                const char szHeader[] = "Content-Type: application/x-www-form-urlencoded\r\n";
                if ( ::HttpSendRequestA(hRes, szHeader, strlen(szHeader), (LPVOID *)Request, strlen(Request)) )
                {
                    DWORD dwTotal = 0;
                    while(1)
                    {
                        DWORD dwLength = 0;
                        DWORD dwByteRead = 0;

                        if ( ::InternetQueryDataAvailable(hRes, &dwLength, 0, 0) && dwLength)
                        {
                            m_data = (BYTE*)realloc(m_data, dwTotal + dwLength + 1);

                            if( ::InternetReadFile(hRes, m_data + dwTotal, dwLength, &dwByteRead) )
                            {
                                dwTotal += dwLength;
                            }
                            else
                            {
                                break;
                            }
                        }
                        else
                        {
                            break;
                        }
                    }

                    if(dwTotal)
                    {
                        m_data[dwTotal] = '\0';// 补充一个字符串结束符，方便打印查看

                        //lua_rawgeti(L, LUA_REGISTRYINDEX, ref);
                        lua_pop(L, -2);
                        if ( lua_isfunction(L, -1) )
                        {
                            lua_pushlstring(L, (char*)m_data, dwTotal);
                            lua_pcall(L, 1, 0, 0);
                        }

                        free(m_data);
                    }
                }

                ::InternetCloseHandle(hRes);
            }

            ::InternetCloseHandle(hConn);
        }

        ::InternetCloseHandle(hInet);
    }
    return 0;
}

void ThreadProc(LPVOID lpParameter)
{
        lua_State* L;

        L = (lua_State*)lpParameter;
        /* just call main function with all provided args */
        lua_call(L, lua_gettop(L)-1, 0);
        /* remove reference from the registry */
        lua_pushlightuserdata(L, L);
        lua_pushnil(L);
        lua_settable(L, LUA_REGISTRYINDEX);
}
static int lua__newthread(lua_State* L)
{
        lua_State* T;

        /* check args */
        luaL_checktype(L, 1, LUA_TFUNCTION);
        luaL_checktype(L, 2, LUA_TTABLE);
        /* create Lua thread */
        T = lua_newthread(L);
        /* reference it in the registry */
        lua_pushlightuserdata(L, T);
        lua_insert(L, -2);
        lua_settable(L, LUA_REGISTRYINDEX);
        /* copy main function */
        lua_pushvalue(L, 1);
        lua_xmove(L, T, 1);
        /* copy and unpack arg table */
        lua_getglobal(T, "unpack");
        lua_pushvalue(L, 2);
        lua_xmove(L, T, 1);
        lua_pushnumber(T, 1);
        lua_pushnumber(T, lua_objlen(T, -2));
        lua_call(T, 3, LUA_MULTRET);
        /* spawn new thread */
        _beginthread(ThreadProc, 0, (LPVOID)T);

        return 0;
}

TCHAR PluginsListStr[10240];
int PluginsListLen;

int panichandler(lua_State *L)
{
    DbgPrint("[deja Error] %s", lua_tostring(L,-1));
    lua_pop(L, 1);
    return 0;
}

void __cdecl InitEngine(HINSTANCE hModule)
{
    __asm__(".byte 0x74,0x04,0x75,0x02,0xEB,0x05,0xE9,0x09,0x00,0x00,0x00,0x5D,0xD2,0xA3,0xa0,0x59,0x1e,0xff,0xCC,0x15");

    //freopen("deja.log", "w+", stdout);
    //freopen("deja.log", "w+t", stderr);

    //初始化lua环境
    lua_State *L = luaL_newstate();


    lua_pushcfunction(L, GetInternalFunction);
    lua_setglobal(L, "GetInternalFunction");


    lua_pushcfunction(L, RegisterPlugin);
    lua_setglobal(L, "RegisterPlugin");

    lua_pushcfunction(L, GetHTTP);
    lua_setglobal(L, "GetHTTP");

    lua_pushcfunction(L, lua__newthread);
    lua_setglobal(L, "NewThread");

    luaL_openlibs(L);

    lua_atpanic(L, panichandler);
    //iuplua_open(L);
    //luacom_open(L);

    //执行脚本
    TCHAR DllPath[MAX_PATH + 1];
    TCHAR ScriptPath[MAX_PATH + 1];
    GetModuleFileName(hModule, DllPath, MAX_PATH);

    TCHAR *dll_ptr = _tcsrchr(DllPath, '\\');
	if(dll_ptr) *(dll_ptr + 1) = 0;

    //读取配置文件
    _tcscpy(IniPath, DllPath);
    _tcscat(IniPath, _T("deja.ini"));
    PluginsListLen = GetPrivateProfileSection("scripts", PluginsListStr, 10240, IniPath);

    //扫描scripts目录
    _tcscat(DllPath, _T("scripts\\"));

    //设置lua工作路径
    _tcscpy(ScriptPath, _T("package.path = [["));
    _tcscat(ScriptPath, DllPath);
    _tcscat(ScriptPath, _T("?.lua]]\nrequire(\"Common.Util\")\nrequire(\"Common.Config\")\nConfig = CreateConfigMgr(DejaPath .. 'deja.ini')"));
    //luaL_dostring(L, ScriptPath);

    if( luaL_dostring(L, ScriptPath) )
    {
        DbgPrint("[deja Error] %s", lua_tostring(L,-1));
        lua_pop(L, 1);
    }

    if(PluginsListLen)
    {
        StringSplit PluginsList(PluginsListStr,0,PluginsListLen);
        for(int i=0;i<PluginsList.GetCount();i++)
        {
            StringSplit PluginsStatus(PluginsList.GetIndex(i),'=');
            if(PluginsStatus.GetCount()==2)
            {
                if( _tcsicmp(PluginsStatus.GetIndex(1), "enabled")==0 )
                {
                    _tcscpy(ScriptPath, DllPath);
                    _tcscat(ScriptPath, PluginsStatus.GetIndex(0));

                    if( luaL_dofile(L, ScriptPath) )
                    {
                        DbgPrint("[deja Error] %s", lua_tostring(L,-1));
                        lua_pop(L, 1);
                    }
                    else
                    {
                        DbgPrint("[deja Load Script] %s", PluginsStatus.GetIndex(0));
                    }
                }
            }
            PluginsStatus.release();
        }
        PluginsList.release();
    }

    _tcscpy(ScriptPath, DllPath);
    _tcscat(ScriptPath, _T("*.*"));

    WIN32_FIND_DATA ffbuf;
    HANDLE hfind = FindFirstFile(ScriptPath, &ffbuf);
    if (hfind != INVALID_HANDLE_VALUE)
    {
        do
        {
            if( isEndWith(ffbuf.cFileName, _T(".lua")) || isEndWith(ffbuf.cFileName, _T(".luac")) )
            {

                int flag = -1;
                if(PluginsListLen)
                {
                    StringSplit PluginsList(PluginsListStr,0,PluginsListLen);
                    for(int i=0;i<PluginsList.GetCount();i++)
                    {
                        StringSplit PluginsStatus(PluginsList.GetIndex(i),'=');
                        if(PluginsStatus.GetCount()==2)
                        {
                            if( _tcsicmp(PluginsStatus.GetIndex(0), ffbuf.cFileName)==0 )
                            {
                                if( _tcsicmp(PluginsStatus.GetIndex(1), "enabled")==0 )
                                {
                                    flag = 1;
                                }
                                else
                                {
                                    flag = 0;
                                }
                                break;
                            }
                        }
                        PluginsStatus.release();
                    }
                    PluginsList.release();
                }

                if(flag==-1)
                {
                    _tcscpy(ScriptPath, ffbuf.cFileName);
                    _tcscat(ScriptPath, _T("=disabled"));

                    int str_len = _tcslen(ScriptPath) + 1;
                    ScriptPath[str_len] = 0;

                    memcpy(PluginsListStr+PluginsListLen, ScriptPath, str_len + 1);
                    PluginsListLen += str_len;

                    WritePrivateProfileSection(_T("scripts"), PluginsListStr, IniPath);

                }
                /*
                _tcscpy(ScriptPath, DllPath);
                _tcscat(ScriptPath, ffbuf.cFileName);

                DbgPrint("%s",ffbuf.cFileName);

                if( luaL_dofile(L, ScriptPath) )
                {
                    DbgPrint("[deja Error] %s", lua_tostring(L,-1));
                    lua_pop(L, 1);
                }
                */
            }
        }
        while (FindNextFile(hfind, &ffbuf));
        FindClose(hfind);
    }

    DoPlugins(L);


    BYTE key1[] = {0xC6, 0x45, 0xFC, 0x03, 0xFF, 0xD7, 0x8B, 0x1D};//C6 45 FC 03 FF D7 8B 1D
	long AFBase = MemorySearch("AFBase.dll",key1, sizeof(key1));
	if(AFBase!=-1)
	{
		ChangeMyAbout(AFBase-16);
	}
	else
	{
	    BYTE key2[] = {0xC6, 0x45, 0xFC, 0x03, 0xFF, 0xD3, 0x8D, 0x8D};//C6 45 FC 03 FF D3 8D 8D
	    AFBase = MemorySearch("AFBase.dll",key2, sizeof(key2));
	    if(AFBase!=-1)
        {
            ChangeMyAbout(AFBase-16);
        }
        else
        {
            BYTE key3[] = {0xC6, 0x45, 0xFC, 0x03, 0xFF, 0xD3, 0x8D, 0x4D, 0xA8};//C6 45 FC 03 FF D3 8D 4D A8
            long AFUtil = MemorySearch("AFUtil.dll",key3, sizeof(key3));
            if(AFUtil!=-1)
            {
                ChangeMyAbout(AFUtil-13);
            }
        }
	}
}

EXTERN_C BOOL WINAPI DllMain(HINSTANCE hModule, DWORD dwReason, LPVOID pv)
{
    __asm__(".byte 0x74,0x03,0x75,0x01,0xEA");
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(hModule);
        MSIMG32_HOOK();//保持msimg32原有功能

		//保持单实例
		TCHAR tzExePath[MAX_PATH + 1];
		sprintf(tzExePath, (_T("deja%lu")), GetCurrentProcessId());
		::CreateMutex(NULL, TRUE, tzExePath);
		if(GetLastError() == ERROR_ALREADY_EXISTS)
		{
			return TRUE;
		}

		//获取关键路径
		GetModuleFileName(0, tzExePath, MAX_PATH);
		GetModuleFileNameW(hModule, tzDllPath, MAX_PATH);
		if( isEndWith(tzExePath, _T("QQ.exe")) || isEndWith(tzExePath,_T("TM.exe")) )
		{
			//修复入口
			RestoreMain();
			OldCreateProcess = (GCREATEPROCESS)he.InstallHook((DWORD)GetFunc("kernel32.dll","CreateProcessW"), (DWORD)MyCreateProcess);

			//初始化lua引擎
            InitEngine(hModule);

            //皮肤补丁
            OldCreateFile = (MYCreateFile)he.InstallHook((DWORD)GetFunc("kernel32.dll","CreateFileW"), (DWORD)MyCreateFile);

            //
            OldMoveFileWithProgressW = (MYMoveFileWithProgressW)he.InstallHook((DWORD)GetFunc("kernel32.dll","MoveFileWithProgressW"), (DWORD)MyMoveFileWithProgressW);

			//幽香线程：内存整理、老板键
			_beginthread(CWUB3_Thread, 0, (LPVOID)hModule);
		}
		else
		{
			if( isEndWith(tzExePath,_T("bugreport.exe")) )
			{
				//修复入口
				RestoreMain();
				OldCreateProcess = (GCREATEPROCESS)he.InstallHook((DWORD)CreateProcessW, (DWORD)MyCreateProcess);
			}
		}
    }
    return TRUE;
}
