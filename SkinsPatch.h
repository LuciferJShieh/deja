
#include <shlobj.h>

bool isFindXtml = 0;
unsigned long Skin_length = 0;

bool need_update_rdb = false;

void update_rdb()
{
    need_update_rdb = true;
}

typedef void (__cdecl* PatchHandle) (unsigned char *buffer, unsigned int length);
PatchHandle MyPatchHandle[100];
int registered = 0;
void SkinsPatchHandle(unsigned long handle)
{
    if(registered<100)
    {
        MyPatchHandle[registered] = (PatchHandle)handle;
        registered++;
    }
}
void SkinsPatch(BYTE* rdb_buffer)
{
    for(int i=0;i<registered;i++)
    {
        MyPatchHandle[i](rdb_buffer, Skin_length);
    }
}

typedef HANDLE (WINAPI*  MYCreateFile)(
									   LPCWSTR lpFileName, //指向文件名的指针
									   DWORD dwDesiredAccess, //访问模式（写/读）
									   DWORD dwShareMode, //共享模式
									   LPSECURITY_ATTRIBUTES lpSecurityAttributes, //指向安全属性的指针
									   DWORD dwCreationDisposition, //如何创建
									   DWORD dwFlagsAndAttributes, //文件属性
									   HANDLE hTemplateFile //用于复制文件句柄
									   );

MYCreateFile OldCreateFile= NULL;

HANDLE WINAPI MyCreateFile(
						   LPCWSTR lpFileName, //指向文件名的指针
						   DWORD dwDesiredAccess, //访问模式（写/读）
						   DWORD dwShareMode, //共享模式
						   LPSECURITY_ATTRIBUTES lpSecurityAttributes, //指向安全属性的指针
						   DWORD dwCreationDisposition, //如何创建
						   DWORD dwFlagsAndAttributes, //文件属性
						   HANDLE hTemplateFile //用于复制文件句柄
						   )
{
	HANDLE hFile = INVALID_HANDLE_VALUE;
	if(OldCreateFile) hFile = OldCreateFile(lpFileName,dwDesiredAccess,dwShareMode,lpSecurityAttributes,dwCreationDisposition,dwFlagsAndAttributes,hTemplateFile);
	if(OldCreateFile && !isFindXtml && isEndWith(lpFileName,L".rdb") && wcsstr(lpFileName,L"Xtml") )
	{
		isFindXtml = true;

		Skin_length = GetFileSize(hFile,NULL);

		wchar_t TempRDB[MAX_PATH + 1];
		GetTempPathW(MAX_PATH, TempRDB);
		wcscat(TempRDB, L"deja.r");

		wchar_t *temp_FileName = _wcsdup(lpFileName);
		wchar_t *pos = (wchar_t *)wcsrchr(temp_FileName,'\\');
		if( pos ) *(pos) = 0;
		pos = (wchar_t *)wcsrchr(temp_FileName,'\\');
		if( pos ) wcscat(TempRDB,pos+2);
		free(temp_FileName);
		wcscat(TempRDB,L".xtml");
		wcscat(TempRDB,L".rdb");

		HANDLE rdb_file = OldCreateFile(TempRDB,dwDesiredAccess,dwShareMode,lpSecurityAttributes,dwCreationDisposition,dwFlagsAndAttributes,hTemplateFile);

		if (rdb_file == INVALID_HANDLE_VALUE || need_update_rdb)
		{
			CloseHandle(rdb_file);
			//生成
			if( pos )
			{
				BYTE *rdb_buffer = (BYTE *)malloc(Skin_length);
				DWORD readsize;
				ReadFile(hFile,rdb_buffer,Skin_length,&readsize,NULL);

				SkinsPatch(rdb_buffer);

				FILE *temp_rdb_file = _wfopen(TempRDB, L"wb");
				if(temp_rdb_file)
				{
					fwrite(rdb_buffer,Skin_length,1,temp_rdb_file);
					fclose(temp_rdb_file);

					CloseHandle(hFile);
					hFile = OldCreateFile(TempRDB,dwDesiredAccess,dwShareMode,lpSecurityAttributes,dwCreationDisposition,dwFlagsAndAttributes,hTemplateFile);

					wchar_t AppData[MAX_PATH+1];
					SHGetSpecialFolderPathW(NULL, AppData, CSIDL_APPDATA, FALSE);
					wcscat(AppData,L"\\Tencent\\QQ\\rdo.cache");

					DeleteFileW(AppData);
				}
				free(rdb_buffer);
			}
		}
		else
		{
			//已经存在
			CloseHandle(hFile);
			hFile = rdb_file;
		}
		//he.Uninstallhook(OldCreateFile);
	}

	return hFile;
}
