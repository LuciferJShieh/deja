--检查是否需要刷新缓存的辅助函数
local function CreateSignature()
    local obj = {}
    
    local count = 0
    local result = 0
    
    function obj.SetSignature(n)
        if n~='yes' then
            result = result + 0
        else
            result = result + 2^count
        end
        
        count = count + 1
    end
    
    function obj.GetSignature(n)
        return result
    end
    
    return obj
end

--
function QQSkinModify()
    local obj = {}
    
    --读取配置文件
    local HideSafeEntry = Config.Skins.HideSafeEntry
    local HideALLServices = Config.Skins.HideALLServices
    local HideLevelButton = Config.Skins.HideLevelButton
    local HideUpdateEntry = Config.Skins.HideUpdateEntry
    
    function obj.GetInfo()
        local info = {}
        info.name = "QQ皮肤修改"
        info.author = "耍下 www.shuax.com"
        info.description = "本插件启用后可以自定义QQ的皮肤描述文件"
        info.version = "1.0.0"
        return info
    end
        
    function obj.OnLoadXtmlRdb(buffer, length)
        --直接修改buffer即可
        --2013尚未支持
        
        --修改QQ2012面板宽度【主面板、登录面板、锁定面板】
        local startset = 0
        for i=1,3 do
            local signature = "09 0E 00 9C F1 98 F1 9F F1 A2 F1 98 F1 8B F1 94 F1 08 00 00 00 D0 F6 F7 F7 EB F5 F7 F7"
            local signature_len = (#signature + 1)/3
            local minSize = SearchMemory(buffer + startset,length - startset, signature)
            if minSize~=-1 then
                buffer[minSize+startset+21] = 0x0D
                buffer[minSize+startset+22] = 0xF7

                buffer[minSize+startset+25] = 0x2C
                buffer[minSize+startset+26] = 0xF6

                startset = startset + minSize + signature_len
            else
                break
            end
        end
        
        if HideSafeEntry=='yes' then
            --屏蔽安全中心【主面板】
            local MainToolExtArea = SearchMemory(buffer, length, "08 08 00 99 F7 96 F7 9A F7 92 F7 1E 00 00 00 AC E1 80 E1 88 E1 8F E1 B5 E1 8E E1 8E E1 8D E1 A4 E1 99 E1 95 E1 A0 E1 93 E1 84 E1 80 E1 01 12")
            if MainToolExtArea~=-1 then buffer[MainToolExtArea] = 0x0F end
            
            --屏蔽安全中心【主菜单】
            local Menu_SecrityCenter = SearchMemory(buffer, length, "08 08 00 99 F7 96 F7 9A F7 92 F7 24 00 00 00 96 DB BE DB B5 DB AE DB 84 DB 88 DB BE DB B8 DB A9 DB B2 DB AF DB A2 DB 98 DB BE DB B5 DB AF DB BE DB A9 DB 08 10")
            if Menu_SecrityCenter~=-1 then
                if buffer[Menu_SecrityCenter-156]==0x82 and buffer[Menu_SecrityCenter-155]==0xB7 then
                    buffer[Menu_SecrityCenter-156] = 0x81
                end
            end
        end
        
        if HideALLServices=='yes' then
            --屏蔽所有服务【主菜单】
            local Menu_ALLServices = SearchMemory(buffer, length, "08 08 00 99 F7 96 F7 9A F7 92 F7 20 00 00 00 92 DF BA DF B1 DF AA DF 80 DF 9E DF 93 DF 93 DF 8C DF BA DF AD DF A9 DF B6 DF BC DF BA DF AC DF 08 10")
            if Menu_ALLServices~=-1 then
                if buffer[Menu_ALLServices-152]==0x82 and buffer[Menu_ALLServices-151]==0xB7 then
                    buffer[Menu_ALLServices-152] = 0x81
                end
            end
        end
        
        if HideLevelButton=='yes' then
            --屏蔽等级按钮【主面板】
            local QQLevelFrame = SearchMemory(buffer, length, "08 08 00 99 F7 96 F7 9A F7 92 F7 18 00 00 00 B6 E7 B6 E7 AB E7 82 E7 91 E7 82 E7 8B E7 B7 E7 86 E7 89 E7 82 E7 8B E7 0B 14 00 9B EB 99 EB 84 EB 9B EB 8E EB 99 EB 9F EB 82 EB 8E EB 98 EB")
            if QQLevelFrame~=-1 then
                buffer[QQLevelFrame+87] = 0xF7
                buffer[QQLevelFrame+91] = 0xF7

                if buffer[QQLevelFrame-798]==0x00 then buffer[QQLevelFrame-798] = 0x01 end
            end
        end
        
        if HideUpdateEntry=='yes' then
            --禁用升级【主菜单】
            local Menu_UpdateHint = SearchMemory(buffer, length, "08 08 00 99 F7 96 F7 9A F7 92 F7 1E 00 00 00 AC E1 84 E1 8F E1 94 E1 BE E1 B4 E1 91 E1 85 E1 80 E1 95 E1 84 E1 A9 E1 88 E1 8F E1 95 E1 08 08 00 83 F7 92 F7 8F F7 83 F7 3A 00 00 00 E1 C5 B5 C5 A9 C5 A4 C5 B1 C5 A3 C5 AA C5 B7 C5 A8 C5 FF C5 88 C5 84 C5 8C C5 8B C5 88 C5 80 C5 8B C5 90 C5 9A C5 90 C5 B5 C5 A1 C5 A4 C5 B1 C5 A0 C5 8D C5 AC C5 AB C5 B1 C5 08 0C")
            if Menu_UpdateHint~=-1 then
                if buffer[Menu_UpdateHint-150]==0x82 and buffer[Menu_UpdateHint-149]==0xB7 then
                    buffer[Menu_UpdateHint-150] = 0x81
                end
            end
        end
        
        --未完待续
    end
    
    function obj.Execute()
        --检查是否需要更新缓存文件
        local cfg_signature = CreateSignature()
        cfg_signature.SetSignature(HideSafeEntry)
        cfg_signature.SetSignature(HideALLServices)
        cfg_signature.SetSignature(HideLevelButton)
        cfg_signature.SetSignature(HideUpdateEntry)
        
        local cfg_old_signature = Config.Skins.SkinsSignature
        if tonumber(cfg_old_signature)~=cfg_signature.GetSignature() then
            --没有匹配特征或者特征不匹配，需要刷新缓存
            UpdateXtmlCache()
            
            Config.Skins.SkinsSignature = cfg_signature.GetSignature()
        end
        
        --禁用升级【设置页】
        if HideUpdateEntry=='yes' then
            local PageSafeUpdate = SearchModule("ConfigCenter.dll", "C7 45 FC 21 00 00")
            if PageSafeUpdate~=-1 then
                WriteJMP(PageSafeUpdate - 0x1A, PageSafeUpdate + 0xC0)
            end
        end
        
        --注册皮肤修改处理函数
        RegisterXtmlHandle(obj.OnLoadXtmlRdb, "void (__cdecl*) (unsigned char *buffer, unsigned int length)")
    end
    
    return obj
end

RegisterPlugin("QQSkinModify")
