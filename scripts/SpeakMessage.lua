local ffi = require("ffi")

require("Common.String")

function SpeakMessage()
    local obj = {}
    
    function obj.GetInfo()
        local info = {}
        info.name = "语音朗读"
        info.author = "耍下 www.shuax.com"
        info.description = "本插件启用后可以使用系统TTS功能朗读聊天消息"
        info.version = "1.0.0"
        return info
    end
    
    local RealSaveMsg1 = nil
    local RealSaveMsg2 = nil
    
    local GetMsgAbstract = ffi.cast("void (__stdcall*) (unsigned long *,unsigned long)", GetFunc("KernelUtil.dll","?GetMsgAbstract@Msg@Util@@YA?AVCTXStringW@@PAUITXMsgPack@@@Z"))
    local GetNickname = ffi.cast("void (__stdcall*) (unsigned long *,unsigned long)", GetFunc("KernelUtil.dll","?GetNickname@Contact@Util@@YA?AVCTXStringW@@K@Z"))
    local GetPublicName = ffi.cast("void (__stdcall*) (unsigned long *,unsigned long)", GetFunc("KernelUtil.dll","?GetPublicName@Contact@Util@@YA?AVCTXStringW@@K@Z"))

    local function IsNotNULL(cData)
        return ffi.cast("void*", cData) > nil
    end
    
    local function GetString(f, p)
        local str = ffi.new("unsigned long [1]")
        f(str, p)
        str = ffi.cast("wchar_t *", str[0])
        return utf16_to_utf8(str)
    end
    
    function obj.QQMsgToText(ITXMsgPack, QQUIN)
        local msg = GetString(GetMsgAbstract, ITXMsgPack)   --聊天消息
        local name = GetString(GetNickname, QQUIN)          --对方昵称
        local nick = GetString(GetPublicName, QQUIN)        --备注名称（取不到自己对自己的备注）
        
        msg = string.replace(msg, "[表情]", "")
        msg = string.replace(msg, "[图片]", "")
        msg = string.replace(msg, "[Emoticon]", "")
        msg = string.replace(msg, "[Image]", "")
        msg = string.replace(msg, "(本消息由您的好友通过手机QQ发送，体验手机QQ请登录： http://mobile.qq.com/c )", "")
        msg = string.replace(msg, "【提示：此用户正在使用Q+ Web：http://web.qq.com/】", "")
        msg = string.replace(msg, "(通过iPhone QQ发送，详情请访问： http://apple.qq.com/ )", "")
        
        if #msg>0 and #msg<64 then
            TextToSpeech( utf8_to_utf16( nick .. "说：" .. msg ) )
        end
    end
    
    function obj.MySaveMsg1(str, src, dst, QQUIN, ITXMsgPack, unused)
        local utf8_str = utf16_to_utf8(str)
        if utf8_str~="group" then
            if dst==QQUIN then
                if dst==src then
                    --自己跟自己聊天
                    obj.QQMsgToText(ITXMsgPack, QQUIN)
                else
                    --别人对你说
                    obj.QQMsgToText(ITXMsgPack, QQUIN)
                end
            else
                --自己对别人说
                obj.QQMsgToText(ITXMsgPack, dst)
            end
        end
        return RealSaveMsg1(str, src, dst, QQUIN, ITXMsgPack, unused)
    end
    
    function obj.MySaveMsg2(str, unused1, unused2, unused3, unused4, QQUIN, ITXMsgPack, unused5)
        local utf8_str = utf16_to_utf8(str)
        if utf8_str=="group" or utf8_str=="discuss" then
            obj.QQMsgToText(ITXMsgPack, QQUIN)
        end
        
        return RealSaveMsg2(str, unused1, unused2, unused3, unused4, QQUIN, ITXMsgPack, unused5)
    end
    
    function obj.Execute()
        if IsNotNULL(GetMsgAbstract) and IsNotNULL(GetNickname) and IsNotNULL(GetPublicName) then
            --QQ聊天
            RealSaveMsg1 = InstallHook(GetFunc("KernelUtil.dll","?SaveMsg@Msg@Util@@YAHPB_WKKKPAUITXMsgPack@@PAUITXData@@@Z"), obj.MySaveMsg1, "int (__cdecl*) (wchar_t *str, uint32_t src, uint32_t dst, uint32_t QQUIN, uint32_t ITXMsgPack, uint32_t)")
            
            --QQ群聊天
            RealSaveMsg2 = InstallHook(GetFunc("KernelUtil.dll","?SaveMsg@Msg@Util@@YAHPB_W000KKPAUITXMsgPack@@PAUITXData@@@Z"), obj.MySaveMsg2, "int (__cdecl*) (wchar_t *str, wchar_t *, wchar_t *, wchar_t *, uint32_t UIN, uint32_t QQUIN, uint32_t ITXMsgPack, uint32_t)")
        end
    end

    return obj
end

RegisterPlugin("SpeakMessage")
