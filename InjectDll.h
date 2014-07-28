void InjectDll(HANDLE hProcess,HANDLE hThread,wchar_t *tzPath)
{
    // 获取函数入口点
    CONTEXT cn;
    cn.ContextFlags = CONTEXT_INTEGER;
    ::GetThreadContext(hThread,&cn);

    //检查入口点是否是E8
    BYTE read[5];
    ReadProcessMemory(hProcess,(LPVOID)cn.Eax,read,sizeof(read),0);
    if(read[0]==0xE8)
    {
        LPVOID pAddr = VirtualAllocEx(hProcess,NULL,1024,MEM_COMMIT,PAGE_EXECUTE_READWRITE);

        DWORD des = (DWORD)cn.Eax + *(DWORD *)&read[1] + 5;

        int len_str = (wcslen(tzPath) + 1)*sizeof(wchar_t);

        //写入dll路径
        ::WriteProcessMemory(hProcess, pAddr, tzPath, len_str, NULL);

        //写入GetKernelBase函数
        const BYTE GetKernelBase[] ={0x56, 0x31, 0xC9, 0x64, 0x8B, 0x71, 0x30, 0x8B, 0x76, 0x0C, 0x8B, 0x76, 0x1C, 0x8B, 0x46, 0x08, 0x8B, 0x7E, 0x20, 0x8B, 0x36, 0x66, 0x39, 0x4F, 0x18, 0x75, 0xF2, 0x5E, 0xC3};
        ::WriteProcessMemory(hProcess, (LPVOID)((DWORD)pAddr + len_str), GetKernelBase, sizeof(GetKernelBase), NULL);

        BYTE ShellCode[] =
        {
            0x68,0x90,0x90,0x90,0x90,    //push patch

            0x68,0x90,0x90,0x90,0x90,    //push Return

            0xE8,0xD4,0xFF,0xFF,0xFF,    //call GetKernelBase
            0x05,0x90,0x90,0x90,0x90,    //add eax,xxxx

            0xFF,0xE0,                    //JMP eax,LoadLibrary
        };

        HMODULE kernel32 = GetModuleHandle(_T("Kernel32.dll"));
        *(DWORD *)&ShellCode[1] = (DWORD)pAddr;
        *(DWORD *)&ShellCode[6] = des;
        *(DWORD *)&ShellCode[16] = (DWORD)GetProcAddress(kernel32, "LoadLibraryW") - (DWORD)kernel32; //(DWORD)LoadLibraryW - (DWORD)pAddr - len_str - 5 - 7;

        //写入LoadLibrary
        ::WriteProcessMemory(hProcess,(LPVOID)((DWORD)pAddr + len_str + sizeof(GetKernelBase)),ShellCode,sizeof(ShellCode),NULL);

        BYTE JmpCode[] =
        {
            0xE8,0x90,0x90,0x90,0x90,    //call xxxx
        };

        *(DWORD *)&JmpCode[1] = (DWORD)pAddr + len_str + sizeof(GetKernelBase) - (DWORD)cn.Eax - 5;

        //写入主函数跳转
        ::WriteProcessMemory(hProcess,(LPVOID)cn.Eax,&JmpCode,sizeof(JmpCode),NULL);
    }
}

void WriteOffset(BYTE type,DWORD TargetProc, DWORD NewProc)
{
    //*(BYTE*)TargetProc = type;
	WriteProcessMemory((HANDLE)-1, (LPVOID)TargetProc, &type, sizeof(type), NULL);
    DWORD offset = NewProc - TargetProc - 5;
	//*(DWORD*)(TargetProc+1) = offset;
    WriteProcessMemory((HANDLE)-1, (LPVOID)(TargetProc + 1), &offset, sizeof(offset), NULL);
}

void RestoreMain()
{
    HANDLE MainExe = GetModuleHandle(NULL);

    PIMAGE_DOS_HEADER pDosHeader = (PIMAGE_DOS_HEADER)MainExe;
    PIMAGE_NT_HEADERS pNtHeader = (PIMAGE_NT_HEADERS)((DWORD)pDosHeader+pDosHeader->e_lfanew);
    DWORD MainEntry = (DWORD)MainExe + pNtHeader->OptionalHeader.AddressOfEntryPoint;

    if( *(BYTE*)MainEntry == 0xE8 )
    {
        DWORD JmpAddr = MainEntry + *(DWORD*)(MainEntry+1) + 5;

        if( max(JmpAddr,MainEntry) - min(JmpAddr,MainEntry) > 0xFFFF )
        {
            //DbgPrint(_T("主入口：%X %X"),MainEntry,JmpAddr);
            //DbgPrint(_T("偏移：%X %X"),*(DWORD*)(JmpAddr+1),*(DWORD*)(JmpAddr+6));
            WriteOffset(0xE8,MainEntry,*(DWORD*)(JmpAddr+6));
            VirtualFreeEx((HANDLE)-1,(LPVOID)*(DWORD*)(JmpAddr+1),0,MEM_RELEASE);//不能在dll载入完成之前释放
        }
    }
}




typedef BOOL (WINAPI* GCREATEPROCESS)(
					LPCWSTR lpApplicationName,
					LPWSTR lpCommandLine,
					LPSECURITY_ATTRIBUTES lpProcessAttributes,
					LPSECURITY_ATTRIBUTES lpThreadAttributes,
					BOOL bInheritHandles,
					DWORD dwCreationFlags,
					LPVOID lpEnvironment,
					LPCWSTR lpCurrentDirectory,
					LPSTARTUPINFO lpStartupInfo,
					LPPROCESS_INFORMATION lpProcessInformation
					);
GCREATEPROCESS OldCreateProcess = NULL;

BOOL WINAPI MyCreateProcess(
					LPCWSTR lpApplicationName,
					LPWSTR lpCommandLine,
					LPSECURITY_ATTRIBUTES lpProcessAttributes,
					LPSECURITY_ATTRIBUTES lpThreadAttributes,
					BOOL bInheritHandles,
					DWORD dwCreationFlags,
					LPVOID lpEnvironment,
					LPCWSTR lpCurrentDirectory,
					LPSTARTUPINFO lpStartupInfo,
					LPPROCESS_INFORMATION lpProcessInformation
					)
{
	if( isEndWith(lpApplicationName,L"QQ.exe") || isEndWith(lpApplicationName,L"bugreport.exe") )
	{
		if(OldCreateProcess)
		{
			BOOL ret = OldCreateProcess(lpApplicationName,lpCommandLine,lpProcessAttributes,lpThreadAttributes,bInheritHandles,CREATE_SUSPENDED,lpEnvironment,lpCurrentDirectory,lpStartupInfo,lpProcessInformation);
			if(ret)
			{
				//注入dll
				InjectDll(lpProcessInformation->hProcess,lpProcessInformation->hThread,tzDllPath);

				//恢复程序执行
				ResumeThread(lpProcessInformation->hThread);
				CloseHandle(lpProcessInformation->hProcess);
			}
			return ret;
		}
		return 0;
	}

	if(wcsstr(lpCommandLine,L"QQExternal.exe")!=0)
	{
		return 0;
	}

	if(wcsstr(lpCommandLine,L"auclt.exe")!=0)
	{
		if(wcsstr(lpCommandLine,L"/MU")==0) return 0;
	}
	if(wcsstr(lpCommandLine,L"txupd.exe")!=0)
	{
		if(wcsstr(lpCommandLine,L"/manual")==0) return 0;
	}
	if(wcsstr(lpCommandLine,L"Tencentdl.exe")!=0)
	{
		return 0;
	}

	//OutputDebugStringW(lpCommandLine);
    if(OldCreateProcess) return OldCreateProcess(lpApplicationName,lpCommandLine,lpProcessAttributes,lpThreadAttributes,bInheritHandles,dwCreationFlags,lpEnvironment,lpCurrentDirectory,lpStartupInfo,lpProcessInformation);
    return 0;
}
