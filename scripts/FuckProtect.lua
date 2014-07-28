function FuckProtect()
    local obj = {}
    
    function obj.GetInfo()
        local info = {}
        info.name = "去除QQProtect"
        info.author = "童话（原创），耍下（翻译为lua）"
        info.description = "本插件启用后可以去除QQProtect.exe的依赖，这样才可以正常启用msimg32.dll"
        info.version = "1.0.0"
        return info
    end

    function obj.Execute()
        --未完成
    end

    return obj
end

RegisterPlugin("FuckProtect")
