--常用字符串函数

string.trim = function(str)
    return str:match'^%s*(.*%S)' or ''
end

string.split = function(str, sep)
    local fields = {}
    str:gsub("[^"..sep.."]+", function(c) fields[#fields+1] = c end)
    return fields
end

string.explode = function(str, sep)
    local function DontAddSpace(t, s)
        if #s>0 then table.insert(t, s) end
    end
    local tble = {}
    local ll = 0
    
    while (true) do
        local l = string.find(str, sep, ll, true)
        if l then
            DontAddSpace(tble, string.sub(str, ll, l - 1)) 
            ll = l + 1
        else
            DontAddSpace(tble, string.sub(str, ll))
            break
        end
        
    end
    
    return tble
end

string.replace = function(str, src, rep)
    local start = 1
    while (true) do
        local pos = string.find(str, src, start, true)
    
        if (pos == nil) then
            break
        end
        
        local left = string.sub(str, 1, pos-1)
        local right = string.sub(str, pos + #src)
        
        str = left .. rep .. right
        start = pos + #rep
    end
    return str
end

string.is_start_with = function(str, substr)
    return str:sub(0, #substr):lower()==substr:lower()
end

string.is_end_with = function(str, substr)
    return str:sub(#str - #substr + 1):lower()==substr:lower()
end

string.to_ip = function(str)
    local match = string.split(str, '.')
    if #match==4 then
        local ret = match[1]
        ret = ret + match[2] * 256
        ret = ret + match[3] * 256 * 256
        ret = ret + match[4] * 256 * 256 * 256
        return ret
    end
end
