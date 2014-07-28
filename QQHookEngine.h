/*
  HOOKLIB by shuax
  2012.03.17
*/
#pragma once

#ifndef __HOOKLIB_H__
#define __HOOKLIB_H__

#define WIN32_LEAN_AND_MEAN
#define WIN32_EXTRA_LEAN

#include <windows.h>
#include <malloc.h>
#include "DisassembleProlog.h"


#pragma pack(1) //保证按照1字节对齐
struct FILL_CODE
{
    BYTE push_code;
    DWORD address;
    BYTE retn_code;
}PUSH_CODE;
#pragma pack(4)

class HookEngine
{
public:
    HookEngine()
    {
        //初始化
        PUSH_CODE.push_code = 0x68;
        PUSH_CODE.retn_code = 0xC3;

        nowLength = 0;
        SaveHook = VirtualAlloc(NULL, 1024, MEM_COMMIT, PAGE_EXECUTE_READWRITE);//备份长度、备份指令、JMP指令
    }

    void* InstallHook(DWORD orig, DWORD det)
    {
        //检查传入参数
        if(orig==0 || det==0) return 0;

        //保存指令
        void* ptr;
        ptr = ((BYTE*)SaveHook) + nowLength;
        //获得需要处理的长度
        int ThunkLen = GetPatchLength(orig,ptr,sizeof(PUSH_CODE));

        if(ThunkLen == 0) return 0;


        nowLength += ThunkLen;
        if( nowLength > 1024 ) return 0;


        //调到原始地址接着运行
        WritePUSH_RET((DWORD)ptr + ThunkLen - sizeof(PUSH_CODE),orig + ThunkLen - sizeof(PUSH_CODE) - 1);

        //写入跳转到新程序
        WritePUSH_RET(orig,det);

        //对多余字节填充NOP
        if(ThunkLen>13) WriteNOP(orig+6,ThunkLen-13);

        *(BYTE*)ptr = ThunkLen;
        ptr = (BYTE*)ptr + 1;

        return ptr;
    }
	template <class T>
    void Uninstallhook(T t)
    {
		union
		{
			DWORD  _addr;
			T  _t;
		} tmp;
		tmp._t = t;

		//检查传入参数
		if(tmp._addr==0) return;

		DWORD ptr = tmp._addr - 1;

        int MinLen = *(BYTE*)ptr;

        DWORD base = *(DWORD*)(ptr+MinLen-5) - (MinLen-7);// + 5 + (DWORD)ptr;
        //DbgPrint(L"%X %X",base,(DWORD)ptr);//[776] 7785C43A 6726E1

		//memcpy((LPVOID)base, (LPVOID)(ptr + 1), MinLen - 7);
		WriteProcessMemory((void*)-1,(LPVOID)base, (LPVOID)(ptr + 1), MinLen - 7, NULL);
        //WriteProcessMemory((void*)-1,(LPVOID)base, ptr, MinLen, NULL);
        memset((void*)ptr,0xC3,MinLen);
        //free();
    }
private:
    void* SaveHook;
    int nowLength;

    int VAtoFileOffset(void *pModuleBase, void *pVA)
    {
        return (DWORD)pVA - (DWORD)pModuleBase;
    }
    void WritePUSH_RET(DWORD TargetProc, DWORD NewProc)
    {
        //写入一个 68 XXXXXXXX C3
        PUSH_CODE.address = NewProc;
        //memcpy((LPVOID)TargetProc, &PUSH_CODE, sizeof(PUSH_CODE));
		WriteProcessMemory((void*)-1,(LPVOID)TargetProc, &PUSH_CODE, sizeof(PUSH_CODE), NULL);
    }
    void WriteNOP(DWORD TargetProc,int len)
    {
        //填充NOP，方便反汇编观察
        static BYTE NOP[] = {0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90};
        WriteProcessMemory((void*)-1,(LPVOID)TargetProc, &NOP, len, NULL);
		//memset((void*)TargetProc,0x90,len);
    }


    bool AntiHook(DWORD orig,int length,void* bak)
    {
        TCHAR stack[MAX_PATH + 1];
        MEMORY_BASIC_INFORMATION mbi;
        VirtualQuery((LPVOID)orig, &mbi, sizeof(mbi));

        GetModuleFileName((HMODULE)mbi.AllocationBase, stack, MAX_PATH);

        HMODULE h_module = GetModuleHandle(stack);
        DWORD offset = VAtoFileOffset(h_module,(void*)orig);

        HANDLE hfile = CreateFile(stack,
            GENERIC_READ,
            FILE_SHARE_READ, NULL, OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL, NULL);
        if (hfile != INVALID_HANDLE_VALUE)
        {
            HANDLE hfilemap = CreateFileMapping(hfile, NULL, PAGE_READONLY|SEC_IMAGE, 0, 0, NULL);
            CloseHandle(hfile);

            unsigned char *buf = (unsigned char*) MapViewOfFile(hfilemap, FILE_MAP_READ, 0, 0, 0);
            CloseHandle(hfilemap);

            memcpy(bak,buf+offset,length);
            UnmapViewOfFile(buf);

            return true;
        }
        return false;
    }

    int GetPatchLength(DWORD func_start, void* thunk, int max_need_len=6)
    {
        BYTE temp[100];
        if(!AntiHook(func_start,100,temp)) return 0;

        int actual_oplen = DisassembleProlog((PBYTE)temp,max_need_len);

        if(actual_oplen==0) return 0;

        //*(BYTE*)thunk = actual_oplen;

        thunk = (BYTE*)thunk + 1;
        memcpy(thunk,temp,actual_oplen);

        actual_oplen = actual_oplen + 1 + max_need_len;

        return actual_oplen;
    }
} ;

HookEngine he;
void* InstallHook2(DWORD orig, DWORD det)
{
    return he.InstallHook(orig,det);
}

unsigned long GetFunc(const TCHAR *tzDllPath, PCSTR pszProcName)
{
    HMODULE m_hModule = GetModuleHandle(tzDllPath);
    if(!m_hModule) m_hModule = LoadLibrary(tzDllPath);
    return (unsigned long)GetProcAddress(m_hModule, pszProcName);
}
#endif // __HOOKLIB_H__
