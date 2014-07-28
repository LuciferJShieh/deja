function QQPluginFilter()
    local obj = {}
    
    function obj.GetInfo()
        local info = {}
        info.name = "QQ插件过滤器"
        info.author = "耍下 www.shuax.com"
        info.description = "本插件启用后阻止不需要的插件启动"
        info.version = "1.0.0"
        return info
    end
    
    local RealLoadPlugin = nil
    function obj.MyLoadPlugin(path)
        
        local utf8_path = utf16_to_utf8(path)
        
        if Config.Plugins[utf8_path]=='disabled' then
            --禁用插件
            return 1
        else
            --启用插件
            if not Config.Plugins[utf8_path] then
                Config.Plugins[utf8_path] = 'enabled'
            end
            
            return RealLoadPlugin(path)
        end
    end

    function obj.Execute()
        --安装钩子，拦截初始化插件操作
        RealLoadPlugin = InstallHook(GetFunc("Common.dll", "?InitPluginCoreConfig@Boot@Util@@YAHPA_W@Z"), obj.MyLoadPlugin, "int (__cdecl*) (wchar_t *path)")
    end

    return obj
end

RegisterPlugin("QQPluginFilter")
