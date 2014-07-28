local ffi = require("ffi")
local bit = require('bit')
require('Common.String')
require('Common.QQWrap')

ffi.cdef[[
    typedef uint32_t SOCKET;
    
    typedef struct {
        uint8_t    sin_len;
        uint8_t    sin_family;
        uint16_t   sin_port;
        uint32_t   sin_addr;
        char       sin_zero[8];
    } sockaddr_in;
]]

local function ntohs(num)
  return bit.bor(bit.lshift(bit.band(num, 0xff), 8), bit.rshift(bit.band(num, 0xff00), 8))
end

local function ip2string(ip)
    local str = ""
    for i=1,4 do
        local mod = ip % 0x100
        ip = (ip - mod)/0x100
        str = str .. mod .. "."
    end
    
    return str:sub(0,-2)
end

global_qq_ip = {}
--[[
global_qq_ip[qq_uid] = {wan = xx;}
]]


local function ShowIPInFrame(qq, ip)
    local function GetMatch(str, begin, over)
        local pos = str:find(begin)
        if pos then
            local match = str:sub(pos + #begin)
            pos = match:find(over)
            return match:sub(0, pos - 1)
        end
    end
    local function CallBack(str)
        local str = utf16_to_utf8( gb2312_to_utf16(str) )
        local address = GetMatch(str, "本站主数据：", "</li>")
        if address then
            address = string.replace(address, " ", "")
            QQMsgInFrame(qq, L(ip2string(ip) .. ' - ' .. address))
            --QQMsgOnBar(qq, L(ip2string(ip) .. ' - ' .. address))
        end
    end
    
    
    HttpGet(CallBack, "http://www.ip138.com/ips138.asp?ip=" .. ip2string(ip) .."&action=2", "", 0)
    
end

function UDPAnalyze()
    local obj = {}
    
    function obj.GetInfo()
        local info = {}
        info.name = "IP探测"
        info.author = "耍下 www.shuax.com"
        info.description = "本插件启用后可以探测IP连接"
        info.version = "1.0.0"
        return info
    end
    
    local RealSendto = nil
    function obj.MySendto(s, buffer, bufLen, flags, sendto, tolen)
        local port = ntohs(sendto.sin_port)
        
        if port~=8000 and port~=9001 then
            --排除与服务器的连接
            if bufLen==27 and buffer[0]==0x03 then
                local QQUID = bit.bswap(ffi.cast("uint32_t *", buffer + 23)[0])
                if not global_qq_ip[QQUID] then
                    global_qq_ip[QQUID] = {}
                    global_qq_ip[QQUID].wan = sendto.sin_addr
                    
                    ShowIPInFrame(QQUID, global_qq_ip[QQUID].wan)
                end
            end
        end
        
        --
        return RealSendto(s, buffer, bufLen, flags, sendto, tolen)
    end

    function obj.Execute()
        RealSendto = InstallHook(GetFunc("ws2_32.dll","sendto"), obj.MySendto, "int (__stdcall*) (SOCKET s, const unsigned char* buf, int len, int flags, const sockaddr_in* to, int tolen)")
    end

    return obj
end

RegisterPlugin("UDPAnalyze")
