#ifndef QQPATCHTOOL_H_INCLUDED
#define QQPATCHTOOL_H_INCLUDED

TCHAR GetCharHex(TCHAR hex)
{
    if (hex >= '0' && hex <= '9') return hex - '0';
    if (hex >= 'A' && hex <= 'F') return hex - 'A' + 10;
    if (hex >= 'a' && hex <= 'f') return hex - 'a' + 10;
    return (TCHAR)-1;
}

int Str2HEX(const TCHAR *string,BYTE *&src)
{
    src = (BYTE*)malloc(1024);

    int i,j,flag;
    for (i = 0;string[i] != 0; i++)
    {
        src[i] = string[i];
        src[i+1] = 0;
    }

    for (i = 0, j = 0, flag = 1; src[i] != 0; i++)
    {
        TCHAR ch = GetCharHex(src[i]);
        if (ch != (TCHAR)-1)
        {
            if (flag == 1) src[j] = ch << 4;
            else src[j++] += ch;
            flag *= -1;
        }
    }

    //修正不是偶数的情况
    if(flag==-1)
    {
        src[j] >>= 4;
        j++;
    }

    return j;
}

int memstr(BYTE* p, int m,BYTE* s, int n)
{
    long mask;
    int skip;
    int i, j, mlast, w;

    w = n - m;

    if (w < 0)
        return -1;

    /* look for special cases */
    if (m <= 1)
    {
        if (m <= 0)//如果模式串为空
            return -1;
        /* use special case for 1-character strings */
        for (i = 0; i < n; i++)
            if (s[i] == p[0])
                return i;

        return -1;
    }

    mlast = m - 1;

    /* create compressed boyer-moore delta 1 table */
    skip = mlast - 1;
    /* process pattern[:-1] */
    for (mask = i = 0; i < mlast; i++)
    {
        mask |= (1 << (p[i] & 0x1F));
        if (p[i] == p[mlast])
            skip = mlast - i - 1;
    }

    /* process pattern[-1] outside the loop */
    mask |= (1 << (p[mlast] & 0x1F));

    for (i = 0; i <= w; i++)   // w == n - m;
    {
        /* note: using mlast in the skip path slows things down on x86 */
        if (s[i+m-1] == p[m-1])    //(Boyer-Moore式的后缀搜索)
        {
            /* candidate match */
            for (j = 0; j < mlast; j++)
                if (s[i+j] != p[j])
                    break;
            if (j == mlast) /* got a match! */
                return i;
            /* miss: check if next character is part of pattern */
            if (!(mask & (1 << (s[i+m] & 0x1F))))  //(Sunday式的基于末字符的下一字符)
                i = i + m;
            else
                i = i + skip; //(Horspool式的基于末字符)
        }
        else
        {
            /* skip: check if next character is part of pattern */
            if (!(mask & (1 << (s[i+m] & 0x1F))))
                i = i + m;
        }
    }
    return -1;
}


bool CheckPE(void *image)
{
    PIMAGE_DOS_HEADER pDosHeader = (PIMAGE_DOS_HEADER)image;
    if(pDosHeader->e_magic!=IMAGE_DOS_SIGNATURE)
    {
        //不是有效的DOS文件
        return false;
    }
    PIMAGE_NT_HEADERS pNtHeader = (PIMAGE_NT_HEADERS)((DWORD)pDosHeader+pDosHeader->e_lfanew);//指向pNtHeader的指针
    if ( pNtHeader->Signature != IMAGE_NT_SIGNATURE )
    {
        //不是有效的NT文件
        return false;
    }
    //检查完成
    //OutputDebugStringA("CheckPE");
    return true;
}

long MemorySearch(const TCHAR *path,BYTE* key, int length)
{
    HMODULE m_hModule = GetModuleHandle(path);
    if(!m_hModule) m_hModule = LoadLibrary(path);
    if(m_hModule)
    {
        if(CheckPE(m_hModule))
        {
            PIMAGE_DOS_HEADER pDosHeader = (PIMAGE_DOS_HEADER)m_hModule;
            PIMAGE_NT_HEADERS pNtHeader = (PIMAGE_NT_HEADERS)((DWORD)pDosHeader+pDosHeader->e_lfanew);
            PIMAGE_FILE_HEADER pPEHead=(PIMAGE_FILE_HEADER)&pNtHeader->FileHeader;
            PIMAGE_SECTION_HEADER section = (PIMAGE_SECTION_HEADER)((DWORD)pPEHead + sizeof(IMAGE_FILE_HEADER) + pPEHead->SizeOfOptionalHeader);
            for(int i=0; i<pPEHead->NumberOfSections; i++)
            {
                if(strcmp((const char*)section[i].Name,".text")==0)
                {
                    long res = memstr(key,length,(BYTE*)((DWORD)m_hModule + section[i].PointerToRawData),section[i].SizeOfRawData);
                    if(res!=-1)
                    {
                        //OutputDebugStringA("memstr");
                        //DbgPrint(L"%s %X",path,res+(DWORD)m_hModule + section[i].PointerToRawData);
                        return res+(DWORD)m_hModule + section[i].PointerToRawData;
                    }
                    break;
                }
            }
        }
    }
    return -1;
}

long SearchMemorySignature(BYTE* s, int n, const TCHAR *signatures)
{
    BYTE *buffer;
    int len = Str2HEX(signatures, buffer);
    long offset = memstr(buffer, len, s, n);
    free(buffer);

    return offset;
}

long SearchModuleSignature(const TCHAR *module, const TCHAR *signatures)
{
    BYTE *buffer;
    int len = Str2HEX(signatures, buffer);
    long offset = MemorySearch(module, buffer, len);
    free(buffer);

    return offset;
}

void WritePatch(unsigned long offset, const TCHAR *signatures)
{
    BYTE *buffer;
    int len = Str2HEX(signatures, buffer);

    WriteProcessMemory((void*)-1,(void*)offset, buffer, len, NULL);

    free(buffer);
}
#endif // QQPATCHTOOL_H_INCLUDED
