local ffi = require("ffi")
ffi.cdef[[
    int __stdcall ShellExecuteW(void *w, const wchar_t *op, const wchar_t *file, const wchar_t *pa, const wchar_t *dic, int type);
]]
local Shell32 = ffi.load("Shell32.dll")

require("Common.String")

function QQMiscPatch()
    local obj = {}
    
    function obj.GetInfo()
        local info = {}
        info.name = "QQ杂项补丁"
        info.author = "耍下 www.shuax.com"
        info.description = "本脚本启用后可以屏蔽QQ内置图片打开功能等"
        info.version = "1.0.0"
        return info
    end
    
    local RealImageViewer = nil
    function obj.MyImageViewer(ignore, path)
        local ret = Shell32.ShellExecuteW(nil, L"open", path, nil, nil, 1)
        if ret<32 then        
            return RealImageViewer(ignore, path)
        end

        return 0
    end
    
    local RealConvertXMLStrToTXData = nil
    function obj.MyConvertXMLStrToTXData(xml, unused1, unused2, unused3)
        
        local utf8_xml = utf16_to_utf8(xml)
        
        --过滤群图标
        if string.find(utf8_xml,"default_gtb") then
            
            local filter = {2,12,15,16,17,18,19}
            for _,v in ipairs(filter) do
                local str = string.format('<b id="%d" visible="1" privilege="1" />',v)
                utf8_xml = string.replace(utf8_xml, str, "")
            end
            
            return RealConvertXMLStrToTXData(utf8_to_utf16(utf8_xml), unused1, unused2, unused3)
        end
        
        --正在登录界面下方功能推荐
        if string.find(utf8_xml,"QQLoginingPanelBottom") then
            return 0
        end
        
        --屏蔽定期更换Banber
        if Config.MiscPatch.DisableQQBanber=='yes' and string.find(utf8_xml,"LoginLogoConfig") then
            return 0
        end
        
        --更换主菜单中的【我的QQ中心】链接
        if Config.MiscPatch.AddSoftUrl=='yes' and string.find(utf8_xml,"MenuSeverControl") then
            utf8_xml = string.replace(utf8_xml, 'Text="我的QQ中心" LinkUrl="http://ptlogin2.qq.com/qqid?sid=5&amp;ptlang=$LANG$&amp;clientuin=$UIN$&amp;clientkey=$KEY$"', 'Text="deja官网" LinkUrl="http://www.shuax.com/"')
            utf8_xml = string.replace(utf8_xml, 'DownloadUrl="http://cdn.id.qq.com/img/id_20_20.png"', 'DownloadUrl="http://shuax.aliapp.com/deja.png"')
            utf8_xml = string.replace(utf8_xml, 'HashCode="416BC4205C700D6CCA4AAB9C6852786A"', 'HashCode="4BC298906D4871A333A320739C3002F0"')
            return RealConvertXMLStrToTXData(utf8_to_utf16(utf8_xml), unused1, unused2, unused3)
        end
        
        return RealConvertXMLStrToTXData(xml, unused1, unused2, unused3)
    end
    
    function obj.Execute()
        --动态加载XML资源处理
        RealConvertXMLStrToTXData = InstallHook(GetFunc("Common.dll", "?ConvertXMLStrToTXData@Convert@Util@@YAHPA_WPAPA_WPAUITXData@@0@Z"), obj.MyConvertXMLStrToTXData, "int (__cdecl*) (wchar_t *xml, void *, void *, void *)")
    
        if Config.MiscPatch.DisableQQViewer=='yes' then
            --搜索特征码【屏蔽QQ内置图片查看器】
            local ImageView = SearchModule("AppMisc.dll", "55 8B EC 56 57 FF 75 0C")
            if ImageView~=-1 then
                RealImageViewer = InstallHook(ImageView, obj.MyImageViewer, "int (__stdcall*) (unsigned long ignore,const wchar_t *path)")
            end
        end
        --搜索特征码【聊天窗安全提示】
        local SafeTips = SearchModule("AppFramework.dll", "0F 84 F0 02 00 00 89")
        SafePatch(SafeTips, 0, "90 E9")
        
        --搜索特征码【聊天窗顶部提示】
        local BarTips = SearchModule("AppFramework.dll", "83 3B 00 75 16")
        SafePatch(BarTips, 3, "90 90")
        
        --去除接收到视频文件时，推荐下载QQ影音的提示
        local IsVideoFile = GetFunc("AFUtil.dll", "?IsVideoFile@Misc@Util@@YAHVCTXStringW@@@Z")
        if IsVideoFile==0 then
            IsVideoFile = GetFunc("AppUtil.dll", "?IsVideoFile@Misc@Util@@YAHVCTXStringW@@@Z")
        end
        
        if IsVideoFile~=0 then
            SafePatch(IsVideoFile, 0, "31 C0 C3")
        end
        
        --搜索特征码【微博界面广告，感谢Starfish】
        local WeiboAD = SearchModule("..\\Plugin\\Com.Tencent.WBlog\\Bin\\WBlog.dll", "8B 4D 10 53 89 06")
        --SafePatch(WeiboAD, -6, "90 E9")
        
        if Config.MiscPatch.EnabledOutlineShake=='yes' then
            --搜索特征码【离线抖动】
            local SNSApp1 = SearchModule("..\\Plugin\\Com.Tencent.SNSApp\\Bin\\SNSApp.dll", "66 3D 14 00 0F 85")
            local SNSApp2 = SearchModule("..\\Plugin\\Com.Tencent.SNSApp\\Bin\\SNSApp.dll", "C2 0C 00 8D 4D D4")
            if SNSApp1~=-1 and SNSApp2~=-1 and SNSApp2>SNSApp1 and SNSApp2-SNSApp1<0x800 then
                --WriteJMP(SNSApp1 + 4,SNSApp2 + 3)
            end
        end
        
        if Config.MiscPatch.DisableEmptyDir=='yes' then
            --搜索特征码【创建空的插件目录 适用于QQ2012正式版】
            local Common = SearchModule("Common.dll", "75 14 57 8D 4D F0")
            SafePatch(Common, 2, "EB 49")
        end
    end

    return obj
end

RegisterPlugin("QQMiscPatch")
