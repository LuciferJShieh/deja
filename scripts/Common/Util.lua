--deja核心文件

local ffi = require("ffi")
local C = ffi.C
local new = ffi.new
ffi.cdef[[
    //系统函数
    int MultiByteToWideChar(int cp, int flag, const char* src, int srclen, wchar_t* dst, int dstlen);
    int WideCharToMultiByte(int cp, int flag, const wchar_t* src, int srclen, char* dst, int dstlen, const char* defchar, int* used);
    int MessageBoxW(void *w, const wchar_t *txt, const wchar_t *cap, int type);
    
    //导出的内部函数
    typedef long (__cdecl* SearchMemorySignature)(const unsigned char *dst, int dstlen, const char *signatures);
    typedef long (__cdecl* SearchModuleSignature)(const char *module, const char *signatures);
    typedef void (__cdecl* WritePatch)(unsigned long offset, const char *signatures);
    typedef void (__cdecl* WriteJMP)(unsigned long TargetProc, unsigned long NewProc);
    typedef unsigned long (__cdecl* GetFunc)(const char *tzDllPath, const char *pszProcName);
]]

function utf8_to_utf16(szUTF8)
    --utf8 to unicode
    szUTF8 = tostring(szUTF8)
    local CP_UTF8 = 65001
    local wcsLen = C.MultiByteToWideChar(CP_UTF8, 0, szUTF8, -1, nil, 0)
    local wszString = new("wchar_t[?]", wcsLen)
    C.MultiByteToWideChar(CP_UTF8, 0, szUTF8, -1, wszString, wcsLen)
    return wszString
end
L = utf8_to_utf16

function utf16_to_utf8(wszString)
    --unicode to utf8
    local CP_UTF8 = 65001
    local szLen = C.WideCharToMultiByte(CP_UTF8, 0, wszString, -1, nil, 0, nil, nil) - 1
    local szUTF8 = new("char[?]", szLen)
    C.WideCharToMultiByte(CP_UTF8, 0, wszString, -1, szUTF8, szLen, nil, nil)
    return ffi.string(szUTF8, szLen)
end

function gb2312_to_utf16(szAnsi)
    --gb2312 to unicode
    local CP_ZH_CN = 936
    local wcsLen = C.MultiByteToWideChar(CP_ZH_CN, 0, szAnsi, -1, nil, 0)
    local wszString = new("wchar_t[?]", wcsLen)
    C.MultiByteToWideChar(CP_ZH_CN, 0, szAnsi, -1, wszString, wcsLen)
    return wszString
end

function MessageBox(txt)
    C.MessageBoxW(nil, L(txt), L"提示", 0)
end

--初始化内部函数
if not GetInternalFunction then MessageBox("内部函数载入失败，程序不能正确运行！") end
local deja = GetInternalFunction()

--导出的内部函数

SearchMemory = ffi.cast("SearchMemorySignature", deja.mem[1])
SearchModule = ffi.cast("SearchModuleSignature", deja.mem[2])
local WritePatch = ffi.cast("WritePatch", deja.mem[3])
WriteJMP = ffi.cast("WriteJMP", deja.mem[4])
GetFunc = ffi.cast("GetFunc", deja.mem[5])
local CreateHook = ffi.cast("void* (__cdecl*) (unsigned long orig, void* det)", deja.mem[6])

local DbgStr = ffi.cast("void (__stdcall*) (const wchar_t* text)", deja.msg[1])
TextToSpeech = ffi.cast("void (__cdecl*) (const wchar_t* text)", deja.msg[2])

local SkinsPatchHandle = ffi.cast("void (__cdecl*) (void* function)", deja.skin[1])
UpdateXtmlCache = ffi.cast("void (__cdecl*) ()", deja.skin[2])

--安全写入补丁
function SafePatch(position, offset, patch)
    if position~=-1 then
        WritePatch(position + offset, patch)
    end
end

--重定向print
local function DbgPrint(...)
    DbgStr(L(table.concat({...}, '\t')))
end
print = DbgPrint

--安装钩子函数
function InstallHook(real_function,lua_function,function_declare)
    return ffi.cast(function_declare, CreateHook(real_function,ffi.cast(function_declare,lua_function)))
end

function RegisterXtmlHandle(lua_function,function_declare)
    SkinsPatchHandle(ffi.cast(function_declare,lua_function))
end

--deja运行目录
DejaPath = string.sub(package.path, 0, -14)

function HttpGet(callback, url, request, isPost)
    NewThread(
    function()
        GetHTTP(callback, url, request, isPost)
    end,{}
    )
end
