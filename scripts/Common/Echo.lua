--用于打印table内容，方便调试

function Echo(t)
    local str = ""
    local function push_str(...)
        str = str .. table.concat({...}, '')
    end
    local function echo(t, n, saved)
        saved = saved or {}
        n = n or 0
        for k in pairs(t) do
            local str = string.rep(" ", 4*n)
            push_str(str,tostring(k), ' = ')
            if type(t[k])=='table' then
                if saved[t[k]] then
                    push_str(saved[t[k]], 'sss\n')
                else
                    saved[t[k]] = k
                    push_str('{\n')
                    echo(t[k], n + 1, saved)
                    push_str(str,'}\n')
                end
            else
                push_str(tostring(t[k]),'\n')
            end
        end
    end
    echo(t)
    print(str)
end

