--一些常用的QQ函数封装

local ffi = require("ffi")
local C = ffi.C
local new = ffi.new
local copy = ffi.copy
ffi.cdef[[
    //系统函数
    int wcslen(const wchar_t *txt);
    
    //结构体定义
    typedef struct TXStr
    {
        int zero;//貌似要一直是0
        int count;//引用计数
        int len1;
        int len2;
        wchar_t str[4100];
    } TXStr;
    
    
    //导出的内部函数
    typedef int (__cdecl* CreateChatFrameType)(unsigned long,unsigned long,unsigned long*,unsigned long);
    typedef void (__cdecl* WriteMsgTipInChatSession)(unsigned long,unsigned long,const wchar_t*,unsigned long);
    
    typedef int (__cdecl* CreateTXData)(unsigned long*);
    typedef int (__cdecl* ChatFrameEvent)(unsigned long, int CFChatType, int CFEventType, unsigned long *ITXData);
    typedef long (__stdcall* QQPut1)(unsigned long *ITXData,wchar_t *,int);
    typedef long (__stdcall* QQPut2)(unsigned long *ITXData,wchar_t *,wchar_t *);
    typedef long (__stdcall* QQBool)(unsigned long *ITXData,wchar_t *,int);
]]

--
local CreateChatFrameType = ffi.cast("CreateChatFrameType", GetFunc("AppUtil.dll", "?CreateChatFrameType@ChatSession@Util@@YAHKHPAPAUIAFChatFrameType@@PAUITXData@@@Z"))
local WriteMsgTipInChatSession = ffi.cast("WriteMsgTipInChatSession", GetFunc("AFUtil.dll", "?WriteMsgTipInChatSession@AFChatSession@Util@@YAXPAUIAFChatFrameType@@W4IconIndex@@VCTXStringW@@H@Z"))

local CreateTXData = ffi.cast("CreateTXData", GetFunc("Common.dll", "?CreateTXData@Data@Util@@YAHPAPAUITXData@@@Z"))
local ChatFrameEvent = ffi.cast("ChatFrameEvent", GetFunc("AppUtil.dll", "?ChatFrameEvent@ChatSession@Util@@YAHKW4CFChatType@ChatFrame@@W4CFEventType@4@PAUITXData@@@Z"))

--
local function IsNotNULL(cData)
    return ffi.cast("void*", cData) > nil
end

--在对话框内显示消息
function QQMsgInFrame(QQUIN, Text)
    local str = new("TXStr", 0, 3)
    str.len1 = C.wcslen(Text)
    str.len2 = str.len1
    copy(str.str, Text, (str.len1 + 1) * 2 )
    
    if str.len1>0 then
        if IsNotNULL(CreateChatFrameType) and IsNotNULL(WriteMsgTipInChatSession) then
            local pointer = new("unsigned long[1]")
            CreateChatFrameType(QQUIN, 0, pointer, 0)
            WriteMsgTipInChatSession(pointer[0], 0, str.str, 0)
        end
    end
end

--在对话框工具条上显示消息
function QQMsgOnBar(QQUIN,Text)
    if IsNotNULL(CreateTXData) and IsNotNULL(ChatFrameEvent) then
        local str = new("TXStr", 0, 3)
        str.len1 = C.wcslen(Text)
        str.len2 = str.len1
        copy(str.str, Text, (str.len1 + 1) * 2 )
        
        if str.len1>0 then
            local pointer = new("unsigned long[1]")
            CreateTXData(pointer)
            
            pointer = ffi.cast("unsigned long*",pointer[0])
            
            local QQPut1 = ffi.cast("QQPut1", ffi.cast("unsigned long*", pointer[0] + 0xE4)[0]);
            local QQPut2 = ffi.cast("QQPut2", ffi.cast("unsigned long*", pointer[0] + 0xE8)[0]);
            local QQBool = ffi.cast("QQBool", ffi.cast("unsigned long*", pointer[0] + 0xCC)[0]);
            
            if IsNotNULL(QQPut1) and IsNotNULL(QQPut2) and IsNotNULL(QQBool) then
                QQPut1(pointer, L"nIconIndex", 20);
                QQPut2(pointer, L"strText", str.str);
                --Bool(pointer, L"bAutoLink", 0);
                QQBool(pointer, L"bCloseButton", 0);
                QQPut1(pointer, L"nLifetime", 60*1000);
                ChatFrameEvent(QQUIN, 0, 101, pointer);
            end
        end
    end
end
