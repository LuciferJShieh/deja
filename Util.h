
bool isEndWith(const TCHAR *path,const TCHAR* ext)
{
	if(!path || !ext) return false;
	int len1 = _tcslen(path);
	int len2 = _tcslen(ext);
	if(len2>len1) return false;
	return !_memicmp(path + len1 - len2,ext,len2*sizeof(TCHAR));
}

bool isEndWith(const wchar_t *path,const wchar_t* ext)
{
	if(!path || !ext) return false;
    int len1 = wcslen(path);
    int len2 = wcslen(ext);
    if(len2>len1) return false;
    return !_memicmp(path + len1 - len2,ext,len2*sizeof(wchar_t));
}

void DbgPrint(const TCHAR *s,...)
{
	va_list vl;
	va_start(vl, s);
	TCHAR buffer[10240];
	_vsnprintf(buffer, 10240, s, vl);
	OutputDebugString(buffer);
	va_end(vl);
}

template <class T>
void PushFunction(lua_State *L, int &count, T t)
{
    union
    {
        DWORD  _addr;
        T  _t;
    } tmp;
    tmp._t = t;

    lua_pushnumber(L, count);
    lua_pushnumber(L, tmp._addr);
    lua_settable(L, -3);
    count++;
}
