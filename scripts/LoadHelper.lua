local ffi = require("ffi")

function LoadHelper()
    local obj = {}
    
    function obj.GetInfo()
        local info = {}
        info.name = "协助加载"
        info.author = "耍下 www.shuax.com"
        info.description = "本插件启用后可以加载其它QQ显IP软件"
        info.version = "1.0.0"
        return info
    end

    function obj.Execute()
        --可以自行添加其它插件
        GetFunc(DejaPath .. "Fineip.dll", "ignore")
        GetFunc(DejaPath .. "HookQQ.dll", "ignore")
        GetFunc(DejaPath .. "NtrQQ.dll", "ignore")
        
        --加载qqext.dll，并且运行其中的OnLoad函数
        local Onload = GetFunc(DejaPath .. "qqext.dll", "Onload")
        if Onload==0 then
            Onload = GetFunc(DejaPath .. "qqext\\qqext.dll", "Onload")
        end
        if Onload~=0 then ffi.cast("unsigned long (__stdcall*) ()",Onload)() end
    end

    return obj
end

RegisterPlugin("LoadHelper")
