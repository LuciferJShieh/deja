function QQUtilPatch()
    local obj = {}
    
    function obj.GetInfo()
        local info = {}
        info.name = "QQ实用补丁"
        info.author = "耍下 www.shuax.com"
        info.description = "本插件启用后可以保护msimg32.dll不被重命名、去校验等"
        info.version = "1.0.0"
        return info
    end

    function obj.Execute()
        --搜索特征码【删除msimg32.dll】
        --为了稳定，此操作由msimg32.dll自己完成
        
        --搜索特征码【插件校验 适用于QQ2012正式版】2013尚不支持
        
        local MsgMgr = SearchModule("MsgMgr.dll", "83 C4 28 3B C3 74 75")
        SafePatch(MsgMgr, 5, "EB")
        
        local AppUtil = SearchModule("AppUtil.dll", "3B C6 74 7A")
        SafePatch(AppUtil, 2, "EB")
        AppUtil = SearchModule("AppUtil.dll", "3B C6 74 7A")
        SafePatch(AppUtil, 2, "EB")
        
        local ChatFrameApp1 = SearchModule("ChatFrameApp.dll", "E9 F2 FE FF FF")
        SafePatch(ChatFrameApp1, 5, "EB F9")
        local ChatFrameApp2 = SearchModule("ChatFrameApp.dll", "8B 75 08 56 8D 55 D4")
        SafePatch(ChatFrameApp2, 12, "90 90 90 90 90")
        
        local AppMisc = SearchModule("AppMisc.dll", "52 8D 45 BC 50")
        SafePatch(AppMisc, 5, "90 90 90 90 90")
    end

    return obj
end

RegisterPlugin("QQUtilPatch")
