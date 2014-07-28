local ffi = require("ffi")
local C = ffi.C
local new = ffi.new
ffi.cdef[[
    bool __stdcall WritePrivateProfileStringA(const char* section, const char* key, const char* value, const char* path);
    unsigned int __stdcall GetPrivateProfileStringA(const char* section, const char* key, const char* default, char* dst, unsigned int dstlen, const char*);
]]

function CreateConfigMgr(IniPath)
    local cfg = {}
    
    local key_meta = {}
    
    --读取配置
    key_meta.__index = function (table, key)
        local result = new("char[260]")
        local length = C.GetPrivateProfileStringA(table.name, key, '', result, 256, IniPath)
        if length==0 then return nil end
        return ffi.string(result, length)
    end
    
    --写入配置
    key_meta.__newindex = function(table, key, value)
        
        if value==nil then
            --删除
            C.WritePrivateProfileStringA(table.name, key, value, IniPath)
        else
            if type(value)=="string" or type(value)=="number" or type(value)=="boolean"  then
                C.WritePrivateProfileStringA(table.name, key, tostring(value), IniPath)
            end
        end
        
    end
    
    --
    local meta = {}  
    meta.__index = function(_, key)
        local key_mgr = {name = key}
        setmetatable(key_mgr, key_meta)
        return key_mgr
    end
    
    setmetatable(cfg, meta)  
    
    return cfg
end
