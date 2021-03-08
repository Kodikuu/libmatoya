// Copyright (c) Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "matoya.h"

#include <stdio.h>

#include <windows.h>
#include <shlwapi.h>
#include <shlobj_core.h>

#include "tlocal.h"

static MTY_TLOCAL char FS_CWD[MTY_PATH_MAX];
static MTY_TLOCAL char FS_HOME[MTY_PATH_MAX];
static MTY_TLOCAL char FS_PATH[MTY_PATH_MAX];
static MTY_TLOCAL char FS_EXECUTABLE[MTY_PATH_MAX];
static MTY_TLOCAL char FS_PROGRAMS[MTY_PATH_MAX];
static MTY_TLOCAL char FS_GLOBAL_HOME[MTY_PATH_MAX];
static MTY_TLOCAL char FS_FILE_NAME[MTY_PATH_MAX];

bool MTY_DeleteFile(const char *path)
{
	wchar_t *wpath = MTY_MultiToWideD(path);

	BOOL ok = DeleteFile(wpath);
	MTY_Free(wpath);

	if (!ok) {
		MTY_Log("'DeleteFile' failed with error 0x%X", GetLastError());
		return false;
	}

	return true;
}

bool MTY_FileExists(const char *path)
{
	wchar_t *wpath = MTY_MultiToWideD(path);

	BOOL ok = PathFileExists(wpath);
	MTY_Free(wpath);

	if (!ok) {
		DWORD e = GetLastError();
		if (e != ERROR_PATH_NOT_FOUND && e != ERROR_FILE_NOT_FOUND)
			MTY_Log("'PathFileExists' failed with error 0x%X", e);
	}

	return ok;
}

bool MTY_Mkdir(const char *path)
{
	wchar_t *wpath = MTY_MultiToWideD(path);

	int32_t e = SHCreateDirectory(NULL, wpath);
	MTY_Free(wpath);

	if (e == ERROR_ALREADY_EXISTS) {
		return false;

	} else if (e != ERROR_SUCCESS) {
		MTY_Log("'SHCreateDirectory' failed with error %d", e);
		return false;
	}

	return true;
}

const char *MTY_Path(const char *dir, const char *file)
{
	char *safe_dir = MTY_Strdup(dir);
	char *safe_file = MTY_Strdup(file);

	snprintf(FS_PATH, MTY_PATH_MAX, "%s\\%s", safe_dir, safe_file);

	MTY_Free(safe_dir);
	MTY_Free(safe_file);

	return FS_PATH;
}

bool MTY_CopyFile(const char *src, const char *dst)
{
	bool r = true;
	wchar_t *srcw = MTY_MultiToWideD(src);
	wchar_t *dstw = MTY_MultiToWideD(dst);

	if (!CopyFile(srcw, dstw, FALSE)) {
		MTY_Log("'CopyFile' failed with error 0x%X", GetLastError());
		r = false;
	}

	MTY_Free(srcw);
	MTY_Free(dstw);

	return r;
}

bool MTY_MoveFile(const char *src, const char *dst)
{
	bool r = true;
	wchar_t *srcw = MTY_MultiToWideD(src);
	wchar_t *dstw = MTY_MultiToWideD(dst);

	if (!MoveFileEx(srcw, dstw, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
		MTY_Log("'MoveFileEx' failed with error 0x%X", GetLastError());
		r = false;
	}

	MTY_Free(srcw);
	MTY_Free(dstw);

	return r;
}

static bool fs_known_folder(const KNOWNFOLDERID *fid, char *dir, size_t size)
{
	WCHAR *dirw = NULL;
	HRESULT e = SHGetKnownFolderPath(fid, 0, NULL, &dirw);

	if (e == S_OK) {
		MTY_WideToMulti(dirw, dir, size);
		CoTaskMemFree(dirw);

		return true;

	} else {
		MTY_Log("'SHGetKnownFolderPath' failed with HRESULT 0x%X", e);
	}

	return false;
}

const char *MTY_GetDir(MTY_Dir dir)
{
	wchar_t tmp[MTY_PATH_MAX];

	switch (dir) {
		case MTY_DIR_CWD: {
			memset(FS_CWD, 0, MTY_PATH_MAX);

			DWORD n = GetCurrentDirectory(MTY_PATH_MAX, tmp);
			if (n > 0) {
				MTY_WideToMulti(tmp, FS_CWD, MTY_PATH_MAX);
				return FS_CWD;

			} else {
				MTY_Log("'GetCurrentDirectory' failed with error 0x%X", GetLastError());
			}

			break;
		}
		case MTY_DIR_HOME: {
			memset(FS_HOME, 0, MTY_PATH_MAX);

			wchar_t *home = NULL;
			errno_t e = _wdupenv_s(&home, NULL, L"APPDATA");

			if (e == 0) {
				MTY_WideToMulti(home, FS_HOME, MTY_PATH_MAX);
				free(home);

				return FS_HOME;

			} else {
				MTY_Log("'_wdupenv_s' failed with errno %d", e);
			}

			break;
		}
		case MTY_DIR_EXECUTABLE: {
			memset(FS_EXECUTABLE, 0, MTY_PATH_MAX);

			DWORD n = GetModuleFileName(NULL, tmp, MTY_PATH_MAX);

			if (n > 0) {
				MTY_WideToMulti(tmp, FS_EXECUTABLE, MTY_PATH_MAX);
				char *name = strrchr(FS_EXECUTABLE, '\\');

				if (name)
					name[0] = '\0';

				return FS_EXECUTABLE;

			} else {
				MTY_Log("'GetModuleFileName' failed with error 0x%X", GetLastError());
			}

			break;
		}
		case MTY_DIR_GLOBAL_HOME:
			if (fs_known_folder(&FOLDERID_ProgramData, FS_GLOBAL_HOME, MTY_PATH_MAX))
				return FS_GLOBAL_HOME;

			break;
		case MTY_DIR_PROGRAMS:
			if (fs_known_folder(&FOLDERID_ProgramFiles, FS_PROGRAMS, MTY_PATH_MAX))
				return FS_PROGRAMS;

			break;
	}

	return ".";
}

MTY_LockFile *MTY_LockFileCreate(const char *path, MTY_FileMode mode)
{
	DWORD share = FILE_SHARE_READ;
	DWORD access = GENERIC_READ;
	DWORD create = OPEN_EXISTING;

	if (mode == MTY_FILE_MODE_WRITE) {
		share = 0;
		access = GENERIC_WRITE;
		create = CREATE_ALWAYS;
	}

	wchar_t *pathw = MTY_MultiToWideD(path);
	HANDLE lock = CreateFile(pathw, access, share, NULL, create, FILE_ATTRIBUTE_NORMAL, NULL);
	MTY_Free(pathw);

	if (lock == INVALID_HANDLE_VALUE) {
		MTY_Log("'CreateFile' failed with error 0x%X", GetLastError());
		return NULL;
	}

	return (MTY_LockFile *) lock;
}

void MTY_LockFileDestroy(MTY_LockFile **lock)
{
	if (!lock || !*lock)
		return;

	MTY_LockFile *ctx = *lock;

	if (!CloseHandle((HANDLE) ctx))
		MTY_Log("'CloseHandle' failed with error 0x%X", GetLastError());

	*lock = NULL;
}

const char *MTY_GetFileName(const char *path, bool extension)
{
	const char *name = strrchr(path, '\\');
	name = name ? name + 1 : path;

	snprintf(FS_FILE_NAME, MTY_PATH_MAX, "%s", name);

	if (!extension) {
		char *ext = strrchr(FS_FILE_NAME, '.');

		if (ext)
			*ext = '\0';
	}

	return FS_FILE_NAME;
}

static int32_t fs_file_compare(const void *p1, const void *p2)
{
	MTY_FileDesc *fi1 = (MTY_FileDesc *) p1;
	MTY_FileDesc *fi2 = (MTY_FileDesc *) p2;

	if (fi1->dir && !fi2->dir) {
		return -1;

	} else if (!fi1->dir && fi2->dir) {
		return 1;

	} else {
		wchar_t *name1 = MTY_MultiToWideD(fi1->name);
		wchar_t *name2 = MTY_MultiToWideD(fi2->name);

		int32_t r = _wcsicmp(name1, name2);

		if (r == 0)
			r = -wcscmp(name1, name2);

		MTY_Free(name1);
		MTY_Free(name2);

		return r;
	}
}

MTY_FileList *MTY_GetFileList(const char *path, const char *filter)
{
	MTY_FileList *fl = MTY_Alloc(1, sizeof(MTY_FileList));
	char *pathd = MTY_Strdup(path);

	WIN32_FIND_DATA ent;
	wchar_t *pathw = MTY_MultiToWideD(MTY_Path(pathd, "*"));

	HANDLE dir = FindFirstFile(pathw, &ent);
	bool ok = dir != INVALID_HANDLE_VALUE;
	MTY_Free(pathw);

	wchar_t *filterw = filter ? MTY_MultiToWideD(filter) : NULL;

	while (ok) {
		wchar_t *namew = ent.cFileName;
		bool is_dir = ent.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY;

		if (is_dir || wcsstr(namew, filterw ? filterw : L"")) {
			fl->files = MTY_Realloc(fl->files, fl->len + 1, sizeof(MTY_FileDesc));

			char *name = MTY_WideToMultiD(namew);
			fl->files[fl->len].name = name;
			fl->files[fl->len].path = MTY_Strdup(MTY_Path(pathd, name));
			fl->files[fl->len].dir = is_dir;
			fl->len++;
		}

		ok = FindNextFile(dir, &ent);

		if (!ok)
			FindClose(dir);
	}

	MTY_Free(filterw);
	MTY_Free(pathd);

	if (fl->len > 0)
		MTY_Sort(fl->files, fl->len, sizeof(MTY_FileDesc), fs_file_compare);

	return fl;
}
