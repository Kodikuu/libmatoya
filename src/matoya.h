// Copyright (c) 2020 Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdarg.h>

#if !defined(MTY_EXPORT)
	#define MTY_EXPORT
#endif

#ifdef __cplusplus
extern "C" {
#endif


/// @module audio

typedef struct MTY_Audio MTY_Audio;

MTY_EXPORT MTY_Audio *
MTY_AudioCreate(uint32_t sampleRate, uint32_t minBuffer, uint32_t maxBuffer);

MTY_EXPORT uint32_t
MTY_AudioGetQueuedMs(MTY_Audio *ctx);

MTY_EXPORT void
MTY_AudioStop(MTY_Audio *ctx);

MTY_EXPORT void
MTY_AudioQueue(MTY_Audio *ctx, const int16_t *frames, uint32_t count);

MTY_EXPORT void
MTY_AudioDestroy(MTY_Audio **audio);


/// @module cminmax

#define MTY_MIN(a, b) \
	((a) > (b) ? (b) : (a))

#define MTY_MAX(a, b) \
	((a) > (b) ? (a) : (b))


/// @module compress

typedef enum {
	MTY_IMAGE_PNG     = 1,
	MTY_IMAGE_JPG     = 2,
	MTY_IMAGE_MAKE_32 = 0x7FFFFFFF,
} MTY_Image;

MTY_EXPORT void *
MTY_CompressImage(MTY_Image type, const void *input, uint32_t width, uint32_t height,
	size_t *outputSize);

MTY_EXPORT void *
MTY_DecompressImage(const void *input, size_t size, uint32_t *width, uint32_t *height);

MTY_EXPORT void *
MTY_CropImage(const void *image, uint32_t cropWidth, uint32_t cropHeight, uint32_t *width,
	uint32_t *height);


/// @module crypto

#define MTY_SHA1_SIZE       20
#define MTY_SHA1_HEX_SIZE   41

#define MTY_SHA256_SIZE     32
#define MTY_SHA256_HEX_SIZE 65

typedef enum {
	MTY_ALGORITHM_SHA1       = 1,
	MTY_ALGORITHM_SHA1_HEX   = 2,
	MTY_ALGORITHM_SHA256     = 3,
	MTY_ALGORITHM_SHA256_HEX = 4,
	MTY_ALGORITHM_MAKE_32    = 0x7FFFFFFF,
} MTY_Algorithm;

typedef struct MTY_AESGCM MTY_AESGCM;

MTY_EXPORT uint32_t
MTY_CRC32(const void *data, size_t size);

MTY_EXPORT uint32_t
MTY_DJB2(const char *str);

MTY_EXPORT void
MTY_BytesToHex(const void *bytes, size_t size, char *hex, size_t hexSize);

MTY_EXPORT void
MTY_HexToBytes(const char *hex, void *bytes, size_t size);

MTY_EXPORT void
MTY_BytesToBase64(const void *bytes, size_t size, char *b64, size_t b64Size);

MTY_EXPORT void
MTY_CryptoHash(MTY_Algorithm algo, const void *input, size_t inputSize, const void *key,
	size_t keySize, void *output, size_t outputSize);

MTY_EXPORT bool
MTY_CryptoHashFile(MTY_Algorithm algo, const char *path, const void *key, size_t keySize,
	void *output, size_t outputSize);

MTY_EXPORT void
MTY_RandomBytes(void *output, size_t size);

MTY_EXPORT uint32_t
MTY_RandomUInt(uint32_t minVal, uint32_t maxVal);

MTY_EXPORT MTY_AESGCM *
MTY_AESGCMCreate(const void *key);

MTY_EXPORT bool
MTY_AESGCMEncrypt(MTY_AESGCM *ctx, const void *nonce, const void *plainText, size_t size,
	void *hash, void *cipherText);

MTY_EXPORT bool
MTY_AESGCMDecrypt(MTY_AESGCM *ctx, const void *nonce, const void *cipherText, size_t size,
	const void *hash, void *plainText);

MTY_EXPORT void
MTY_AESGCMDestroy(MTY_AESGCM **aesgcm);


/// @module fs

#define MTY_URL_MAX  1024
#define MTY_PATH_MAX 1280

typedef enum {
	MTY_DIR_CWD        = 1,
	MTY_DIR_HOME       = 2,
	MTY_DIR_EXECUTABLE = 3,
	MTY_DIR_PROGRAMS   = 4,
	MTY_DIR_MAKE_32    = 0x7FFFFFFF,
} MTY_Dir;

typedef enum {
	MTY_FILE_MODE_WRITE   = 1,
	MTY_FILE_MODE_READ    = 2,
	MTY_FILE_MODE_MAKE_32 = 0x7FFFFFFF,
} MTY_FileMode;

typedef struct {
	char *path;
	char *name;
	bool dir;
} MTY_FileDesc;

typedef struct {
	MTY_FileDesc *files;
	uint32_t len;
} MTY_FileList;

typedef struct MTY_LockFile MTY_LockFile;

MTY_EXPORT void *
MTY_ReadFile(const char *path, size_t *size);

MTY_EXPORT bool
MTY_WriteFile(const char *path, const void *data, size_t size);

MTY_EXPORT bool
MTY_WriteTextFile(const char *path, const char *fmt, ...);

MTY_EXPORT bool
MTY_AppendTextToFile(const char *path, const char *fmt, ...);

MTY_EXPORT bool
MTY_DeleteFile(const char *path);

MTY_EXPORT bool
MTY_FileExists(const char *path);

MTY_EXPORT bool
MTY_Mkdir(const char *path);

MTY_EXPORT const char *
MTY_Path(const char *dir, const char *file);

MTY_EXPORT bool
MTY_CopyFile(const char *src, const char *dst);

MTY_EXPORT bool
MTY_MoveFile(const char *src, const char *dst);

MTY_EXPORT const char *
MTY_GetDir(MTY_Dir dir);

MTY_EXPORT MTY_LockFile *
MTY_LockFileCreate(const char *path, MTY_FileMode mode);

MTY_EXPORT void
MTY_LockFileDestroy(MTY_LockFile **lock);

MTY_EXPORT const char *
MTY_GetFileName(const char *path, bool extension);

MTY_EXPORT MTY_FileList *
MTY_GetFileList(const char *path, const char *filter);

MTY_EXPORT void
MTY_FreeFileList(MTY_FileList **fileList);


/// @module json
/// @details The `MTY_JSON` struct is a special kind of object with different semantics
///     than a typical object. The `JSON` prefix is assigned to all functions that return
///     or operate on JSON, while `ReadFile` and `Parse` act as constructors instead of
///     the typical `Create`. Since a JSON object serializes data rather than holding
///     state, the `const` keyword makes sense.
///
///     The `JSONObjGetType` style macros make dealing with JSON easier in C. Other
///     languages that support generics would not need these.

typedef struct MTY_JSON MTY_JSON;

MTY_EXPORT MTY_JSON *
MTY_JSONParse(const char *input);

MTY_EXPORT char *
MTY_JSONStringify(const MTY_JSON *json);

MTY_EXPORT MTY_JSON *
MTY_JSONReadFile(const char *path);

MTY_EXPORT bool
MTY_JSONWriteFile(const char *path, const MTY_JSON *json);

MTY_EXPORT MTY_JSON *
MTY_JSONDuplicate(const MTY_JSON *json);

MTY_EXPORT uint32_t
MTY_JSONLength(const MTY_JSON *json);

MTY_EXPORT void
MTY_JSONDestroy(MTY_JSON **json);

MTY_EXPORT bool
MTY_JSONToString(const MTY_JSON *json, char *value, size_t size);

MTY_EXPORT bool
MTY_JSONToInt(const MTY_JSON *json, int32_t *value);

MTY_EXPORT bool
MTY_JSONToUInt(const MTY_JSON *json, uint32_t *value);

MTY_EXPORT bool
MTY_JSONToInt16(const MTY_JSON *json, int16_t *value);

MTY_EXPORT bool
MTY_JSONToUInt16(const MTY_JSON *json, uint16_t *value);

MTY_EXPORT bool
MTY_JSONToInt8(const MTY_JSON *json, int8_t *value);

MTY_EXPORT bool
MTY_JSONToUInt8(const MTY_JSON *json, uint8_t *value);

MTY_EXPORT bool
MTY_JSONToFloat(const MTY_JSON *json, float *value);

MTY_EXPORT bool
MTY_JSONToBool(const MTY_JSON *json, bool *value);

MTY_EXPORT bool
MTY_JSONIsNull(const MTY_JSON *json);

MTY_EXPORT MTY_JSON *
MTY_JSONFromString(const char *value);

MTY_EXPORT MTY_JSON *
MTY_JSONFromInt(int32_t value);

MTY_EXPORT MTY_JSON *
MTY_JSONFromUInt(uint32_t value);

MTY_EXPORT MTY_JSON *
MTY_JSONFromFloat(float value);

MTY_EXPORT MTY_JSON *
MTY_JSONFromBool(bool value);

MTY_EXPORT MTY_JSON *
MTY_JSONNull(void);

MTY_EXPORT MTY_JSON *
MTY_JSONObj(void);

MTY_EXPORT MTY_JSON *
MTY_JSONArray(void);

MTY_EXPORT bool
MTY_JSONObjKeyExists(const MTY_JSON *json, const char *key);

MTY_EXPORT const char *
MTY_JSONObjGetKey(const MTY_JSON *json, uint32_t index);

MTY_EXPORT void
MTY_JSONObjDeleteKey(MTY_JSON *json, const char *key);

MTY_EXPORT const MTY_JSON *
MTY_JSONObjGet(const MTY_JSON *json, const char *key);

MTY_EXPORT void
MTY_JSONObjSet(MTY_JSON *json, const char *key, const MTY_JSON *value);

MTY_EXPORT bool
MTY_JSONArrayIndexExists(const MTY_JSON *json, uint32_t index);

MTY_EXPORT void
MTY_JSONArrayDeleteIndex(MTY_JSON *json, uint32_t index);

MTY_EXPORT const MTY_JSON *
MTY_JSONArrayGet(const MTY_JSON *json, uint32_t index);

MTY_EXPORT void
MTY_JSONArraySet(MTY_JSON *json, uint32_t index, const MTY_JSON *value);

MTY_EXPORT void
MTY_JSONArrayAppend(MTY_JSON *json, const MTY_JSON *value);

#define MTY_JSONObjGetString(j, k, v, n) \
	MTY_JSONToString(MTY_JSONObjGet(j, k), v, n)

#define MTY_JSONObjGetInt(j, k, v) \
	MTY_JSONToInt(MTY_JSONObjGet(j, k), v)

#define MTY_JSONObjGetUInt(j, k, v) \
	MTY_JSONToUInt(MTY_JSONObjGet(j, k), v)

#define MTY_JSONObjGetInt16(j, k, v) \
	MTY_JSONToInt16(MTY_JSONObjGet(j, k), v)

#define MTY_JSONObjGetUInt16(j, k, v) \
	MTY_JSONToUInt16(MTY_JSONObjGet(j, k), v)

#define MTY_JSONObjGetInt8(j, k, v) \
	MTY_JSONToInt8(MTY_JSONObjGet(j, k), v)

#define MTY_JSONObjGetUInt8(j, k, v) \
	MTY_JSONToUInt8(MTY_JSONObjGet(j, k), v)

#define MTY_JSONObjGetFloat(j, k, v) \
	MTY_JSONToFloat(MTY_JSONObjGet(j, k), v)

#define MTY_JSONObjGetBool(j, k, v) \
	MTY_JSONToBool(MTY_JSONObjGet(j, k), v)

#define MTY_JSONObjSetString(j, k, v) \
	MTY_JSONObjSet(j, k, MTY_JSONFromString(v))

#define MTY_JSONObjSetInt(j, k, v) \
	MTY_JSONObjSet(j, k, MTY_JSONFromInt(v))

#define MTY_JSONObjSetUInt(j, k, v) \
	MTY_JSONObjSet(j, k, MTY_JSONFromUInt(v))

#define MTY_JSONObjSetFloat(j, k, v) \
	MTY_JSONObjSet(j, k, MTY_JSONFromFloat(v))

#define MTY_JSONObjSetBool(j, k, v) \
	MTY_JSONObjSet(j, k, MTY_JSONFromBool(v))

#define MTY_JSONArrayGetString(j, i, v, n) \
	MTY_JSONToString(MTY_JSONArrayGet(j, i), v, n)

#define MTY_JSONArrayGetInt(j, i, v) \
	MTY_JSONToInt(MTY_JSONArrayGet(j, i), v)

#define MTY_JSONArrayGetUInt(j, i, v) \
	MTY_JSONToUInt(MTY_JSONArrayGet(j, i), v)

#define MTY_JSONArrayGetFloat(j, i, v) \
	MTY_JSONToFloat(MTY_JSONArrayGet(j, i), v)

#define MTY_JSONArrayGetBool(j, i, v) \
	MTY_JSONToBool(MTY_JSONArrayGet(j, i), v)

#define MTY_JSONArraySetString(j, i, v) \
	MTY_JSONArraySet(j, i, MTY_JSONFromString(v))

#define MTY_JSONArraySetInt(j, i, v) \
	MTY_JSONArraySet(j, i, MTY_JSONFromInt(v))

#define MTY_JSONArraySetUInt(j, i, v) \
	MTY_JSONArraySet(j, i, MTY_JSONFromUInt(v))

#define MTY_JSONArraySetFloat(j, i, v) \
	MTY_JSONArraySet(j, i, MTY_JSONFromFloat(v))

#define MTY_JSONArraySetBool(j, i, v) \
	MTY_JSONArraySet(j, i, MTY_JSONFromBool(v))


/// @module log

MTY_EXPORT void
MTY_SetLogCallback(void (*callback)(const char *msg, void *opaque), void *opaque);

MTY_EXPORT void
MTY_DisableLog(bool disabled);

MTY_EXPORT void
MTY_LogParams(const char *func, const char *msg, ...);

MTY_EXPORT void
MTY_FatalParams(const char *func, const char *msg, ...);

MTY_EXPORT const char *
MTY_GetLog(void);

#define MTY_Log(msg, ...) \
	MTY_LogParams(__FUNCTION__, msg, ##__VA_ARGS__)

#define MTY_Fatal(msg, ...) \
	MTY_FatalParams(__FUNCTION__, msg, ##__VA_ARGS__)


/// @module memory

MTY_EXPORT void *
MTY_Alloc(size_t nelem, size_t elsize);

MTY_EXPORT void *
MTY_AllocAligned(size_t size, size_t align);

MTY_EXPORT void *
MTY_Realloc(void *mem, size_t nelem, size_t elsize);

MTY_EXPORT void *
MTY_Dup(const void *mem, size_t size);

MTY_EXPORT char *
MTY_Strdup(const char *str);

MTY_EXPORT void
MTY_Strcat(char *dst, size_t size, const char *src);

MTY_EXPORT int32_t
MTY_Strcasecmp(const char *s1, const char *s2);

MTY_EXPORT void
MTY_Free(void *mem);

MTY_EXPORT void
MTY_FreeAligned(void *mem);

MTY_EXPORT uint16_t
MTY_Swap16(uint16_t value);

MTY_EXPORT uint32_t
MTY_Swap32(uint32_t value);

MTY_EXPORT uint64_t
MTY_Swap64(uint64_t value);

MTY_EXPORT uint16_t
MTY_SwapToBE16(uint16_t value);

MTY_EXPORT uint32_t
MTY_SwapToBE32(uint32_t value);

MTY_EXPORT uint64_t
MTY_SwapToBE64(uint64_t value);

MTY_EXPORT uint16_t
MTY_SwapFromBE16(uint16_t value);

MTY_EXPORT uint32_t
MTY_SwapFromBE32(uint32_t value);

MTY_EXPORT uint64_t
MTY_SwapFromBE64(uint64_t value);

MTY_EXPORT bool
MTY_WideToMulti(const wchar_t *src, char *dst, size_t len);

MTY_EXPORT char *
MTY_WideToMultiD(const wchar_t *src);

MTY_EXPORT bool
MTY_MultiToWide(const char *src, wchar_t *dst, uint32_t len);

MTY_EXPORT wchar_t *
MTY_MultiToWideD(const char *src);

MTY_EXPORT char *
MTY_Strtok(char *str, const char *delim, char **saveptr);

#define MTY_Align16(v) \
	((v) + 0xF & ~((uintptr_t) 0xF))

#define MTY_Align32(v) \
	((v) + 0x1F & ~((uintptr_t) 0x1F))


/// @module proc

typedef struct MTY_SO MTY_SO;

MTY_EXPORT MTY_SO *
MTY_SOLoad(const char *name);

MTY_EXPORT void *
MTY_SOGetSymbol(MTY_SO *so, const char *name);

MTY_EXPORT void
MTY_SOUnload(MTY_SO **so);

MTY_EXPORT const char *
MTY_ProcessName(void);

MTY_EXPORT bool
MTY_RestartProcess(int32_t argc, char * const *argv);

MTY_EXPORT const char *
MTY_Hostname(void);


/// @module render

typedef enum {
	MTY_GFX_NONE    = 0,
	MTY_GFX_GL      = 1,
	MTY_GFX_D3D9    = 2,
	MTY_GFX_D3D11   = 3,
	MTY_GFX_METAL   = 4,
	MTY_GFX_MAX,
	MTY_GFX_MAKE_32 = 0x7FFFFFFF,
} MTY_GFX;

typedef enum {
	MTY_COLOR_FORMAT_UNKNOWN = 0,
	MTY_COLOR_FORMAT_RGBA    = 1,
	MTY_COLOR_FORMAT_NV12    = 2,
	MTY_COLOR_FORMAT_I420    = 3,
	MTY_COLOR_FORMAT_I444    = 4,
	MTY_COLOR_FORMAT_NV16    = 5,
	MTY_COLOR_FORMAT_RGB565  = 6,
	MTY_COLOR_FORMAT_MAKE_32 = 0x7FFFFFFF,
} MTY_ColorFormat;

typedef enum {
	MTY_FILTER_NEAREST        = 1,
	MTY_FILTER_LINEAR         = 2,
	MTY_FILTER_GAUSSIAN_SHARP = 3,
	MTY_FILTER_GAUSSIAN_SOFT  = 4,
	MTY_FILTER_MAKE_32        = 0x7FFFFFFF,
} MTY_Filter;

typedef enum {
	MTY_EFFECT_NONE         = 0,
	MTY_EFFECT_SCANLINES    = 1,
	MTY_EFFECT_SCANLINES_X2 = 2,
	MTY_EFFECT_MAKE_32      = 0x7FFFFFFF,
} MTY_Effect;

typedef enum {
	MTY_ROTATION_NONE    = 0,
	MTY_ROTATION_90      = 1,
	MTY_ROTATION_180     = 2,
	MTY_ROTATION_270     = 3,
	MTY_ROTATION_MAKE_32 = 0x7FFFFFFF,
} MTY_Rotation;

typedef struct MTY_RenderDesc {
	MTY_ColorFormat format;
	MTY_Rotation rotation;
	MTY_Filter filter;
	MTY_Effect effect;
	uint32_t imageWidth;
	uint32_t imageHeight;
	uint32_t cropWidth;
	uint32_t cropHeight;
	uint32_t viewWidth;
	uint32_t viewHeight;
	float aspectRatio;
	float scale;
} MTY_RenderDesc;

typedef struct {
	float x;
	float y;
} MTY_Vec2;

typedef struct {
	float x;
	float y;
	float z;
	float w;
} MTY_Vec4;

typedef struct {
	MTY_Vec2 pos;
	MTY_Vec2 uv;
	uint32_t col;
} MTY_Vtx;

typedef struct {
	MTY_Vec4 clip;
	uint32_t texture;
	uint32_t elemCount;
	uint32_t idxOffset;
	uint32_t vtxOffset;
} MTY_Cmd;

typedef struct {
	MTY_Cmd *cmd;
	MTY_Vtx *vtx;
	uint16_t *idx;
	uint32_t cmdLength;
	uint32_t cmdMax;
	uint32_t vtxLength;
	uint32_t vtxMax;
	uint32_t idxLength;
	uint32_t idxMax;
} MTY_CmdList;

typedef struct {
	MTY_Vec2 displaySize;
	MTY_CmdList *cmdList;
	uint32_t cmdListLength;
	uint32_t cmdListMax;
	uint32_t idxTotalLength;
	uint32_t vtxTotalLength;
	bool clear;
} MTY_DrawData;

typedef struct MTY_Device MTY_Device;
typedef struct MTY_Context MTY_Context;
typedef struct MTY_Texture MTY_Texture;
typedef struct MTY_Renderer MTY_Renderer;
typedef struct MTY_RenderState MTY_RenderState;

MTY_EXPORT MTY_Renderer *
MTY_RendererCreate(void);

MTY_EXPORT bool
MTY_RendererDrawQuad(MTY_Renderer *ctx, MTY_GFX api, MTY_Device *device,
	MTY_Context *context, const void *image, const MTY_RenderDesc *desc,
	MTY_Texture *dest);

MTY_EXPORT bool
MTY_RendererDrawUI(MTY_Renderer *ctx, MTY_GFX api, MTY_Device *device,
	MTY_Context *context, const MTY_DrawData *dd, MTY_Texture *dest);

MTY_EXPORT void *
MTY_RendererSetUITexture(MTY_Renderer *ctx, MTY_GFX api, MTY_Device *device,
	MTY_Context *context, uint32_t id, const void *rgba, uint32_t width, uint32_t height);

MTY_EXPORT void *
MTY_RendererGetUITexture(MTY_Renderer *ctx, uint32_t id);

MTY_EXPORT void
MTY_RendererDestroy(MTY_Renderer **renderer);

MTY_EXPORT uint32_t
MTY_GetAvailableGFX(MTY_GFX *apis);

MTY_EXPORT MTY_RenderState *
MTY_GetRenderState(MTY_GFX api, MTY_Device *device, MTY_Context *context);

MTY_EXPORT void
MTY_SetRenderState(MTY_GFX api, MTY_Device *device, MTY_Context *context,
	MTY_RenderState *state);

MTY_EXPORT void
MTY_FreeRenderState(MTY_RenderState **state);


/// @module sort

MTY_EXPORT void
MTY_Sort(void *base, size_t nElements, size_t size,
	int32_t (*compare)(const void *a, const void *b));


/// @module struct

typedef struct MTY_ListNode {
	void *value;
	struct MTY_ListNode *prev;
	struct MTY_ListNode *next;
} MTY_ListNode;

typedef struct MTY_Hash MTY_Hash;
typedef struct MTY_Queue MTY_Queue;
typedef struct MTY_List MTY_List;

MTY_EXPORT MTY_Hash *
MTY_HashCreate(uint32_t numBuckets);

MTY_EXPORT void *
MTY_HashGet(MTY_Hash *ctx, const char *key);

MTY_EXPORT void *
MTY_HashGetInt(MTY_Hash *ctx, int64_t key);

MTY_EXPORT void *
MTY_HashSet(MTY_Hash *ctx, const char *key, void *value);

MTY_EXPORT void *
MTY_HashSetInt(MTY_Hash *ctx, int64_t key, void *value);

MTY_EXPORT void *
MTY_HashPop(MTY_Hash *ctx, const char *key);

MTY_EXPORT void *
MTY_HashPopInt(MTY_Hash *ctx, int64_t key);

MTY_EXPORT bool
MTY_HashNextKey(MTY_Hash *ctx, uint64_t *iter, const char **key);

MTY_EXPORT bool
MTY_HashNextKeyInt(MTY_Hash *ctx, uint64_t *iter, int64_t *key);

MTY_EXPORT void
MTY_HashDestroy(MTY_Hash **hash, void (*freeFunc)(void *value));

MTY_EXPORT MTY_Queue *
MTY_QueueCreate(uint32_t len, size_t bufSize);

MTY_EXPORT uint32_t
MTY_QueueLength(MTY_Queue *ctx);

MTY_EXPORT void *
MTY_QueueAcquireBuffer(MTY_Queue *ctx);

MTY_EXPORT void
MTY_QueuePush(MTY_Queue *ctx, size_t size);

MTY_EXPORT bool
MTY_QueuePop(MTY_Queue *ctx, int32_t timeout, void **buffer, size_t *size);

MTY_EXPORT bool
MTY_QueuePopLast(MTY_Queue *ctx, int32_t timeout, void **buffer, size_t *size);

MTY_EXPORT void
MTY_QueueReleaseBuffer(MTY_Queue *ctx);

MTY_EXPORT bool
MTY_QueuePushPtr(MTY_Queue *ctx, void *opaque, size_t size);

MTY_EXPORT bool
MTY_QueuePopPtr(MTY_Queue *ctx, int32_t timeout, void **opaque, size_t *size);

MTY_EXPORT void
MTY_QueueFlush(MTY_Queue *ctx, void (*freeFunc)(void *value));

MTY_EXPORT void
MTY_QueueDestroy(MTY_Queue **queue);

MTY_EXPORT MTY_List *
MTY_ListCreate(void);

MTY_EXPORT MTY_ListNode *
MTY_ListFirst(MTY_List *ctx);

MTY_EXPORT void
MTY_ListAppend(MTY_List *ctx, void *value);

MTY_EXPORT void *
MTY_ListRemove(MTY_List *ctx, MTY_ListNode *node);

MTY_EXPORT void
MTY_ListDestroy(MTY_List **list, void (*freeFunc)(void *value));


/// @module thread

typedef enum {
	MTY_THREAD_STATE_EMPTY    = 0,
	MTY_THREAD_STATE_RUNNING  = 1,
	MTY_THREAD_STATE_DETACHED = 2,
	MTY_THREAD_STATE_DONE     = 3,
	MTY_THREAD_STATE_MAKE_32  = 0x7FFFFFFF,
} MTY_ThreadState;

typedef struct {
	volatile int32_t value;
} MTY_Atomic32;

typedef struct {
	volatile int64_t value;
} MTY_Atomic64;

typedef struct MTY_Thread MTY_Thread;
typedef struct MTY_Mutex MTY_Mutex;
typedef struct MTY_Cond MTY_Cond;
typedef struct MTY_RWLock MTY_RWLock;
typedef struct MTY_Sync MTY_Sync;
typedef struct MTY_ThreadPool MTY_ThreadPool;

MTY_EXPORT MTY_Thread *
MTY_ThreadCreate(void *(*func)(void *opaque), void *opaque);

MTY_EXPORT void
MTY_ThreadDetach(void *(*func)(void *opaque), void *opaque);

MTY_EXPORT int64_t
MTY_ThreadGetID(MTY_Thread *ctx);

MTY_EXPORT void *
MTY_ThreadDestroy(MTY_Thread **thread);

MTY_EXPORT MTY_Mutex *
MTY_MutexCreate(void);

MTY_EXPORT void
MTY_MutexLock(MTY_Mutex *ctx);

MTY_EXPORT bool
MTY_MutexTryLock(MTY_Mutex *ctx);

MTY_EXPORT void
MTY_MutexUnlock(MTY_Mutex *ctx);

MTY_EXPORT void
MTY_MutexDestroy(MTY_Mutex **mutex);

MTY_EXPORT MTY_Cond *
MTY_CondCreate(void);

MTY_EXPORT bool
MTY_CondWait(MTY_Cond *ctx, MTY_Mutex *mutex, int32_t timeout);

MTY_EXPORT void
MTY_CondWake(MTY_Cond *ctx);

MTY_EXPORT void
MTY_CondWakeAll(MTY_Cond *ctx);

MTY_EXPORT MTY_RWLock *
MTY_RWLockCreate(void);

MTY_EXPORT void
MTY_RWLockReader(MTY_RWLock *ctx);

MTY_EXPORT void
MTY_RWLockWriter(MTY_RWLock *ctx);

MTY_EXPORT void
MTY_RWLockUnlock(MTY_RWLock *ctx);

MTY_EXPORT void
MTY_RWLockDestroy(MTY_RWLock **rwlock);

MTY_EXPORT void
MTY_CondDestroy(MTY_Cond **cond);

MTY_EXPORT MTY_Sync *
MTY_SyncCreate(void);

MTY_EXPORT bool
MTY_SyncWait(MTY_Sync *ctx, int32_t timeout);

MTY_EXPORT void
MTY_SyncWake(MTY_Sync *ctx);

MTY_EXPORT void
MTY_SyncDestroy(MTY_Sync **sync);

MTY_EXPORT MTY_ThreadPool *
MTY_ThreadPoolCreate(uint32_t maxThreads);

MTY_EXPORT uint32_t
MTY_ThreadPoolStart(MTY_ThreadPool *ctx, void (*func)(void *opaque), void *opaque);

MTY_EXPORT void
MTY_ThreadPoolDetach(MTY_ThreadPool *ctx, uint32_t index, void (*detach)(void *opaque));

MTY_EXPORT MTY_ThreadState
MTY_ThreadPoolState(MTY_ThreadPool *ctx, uint32_t index, void **opaque);

MTY_EXPORT void
MTY_ThreadPoolDestroy(MTY_ThreadPool **pool, void (*detach)(void *opaque));

MTY_EXPORT void
MTY_Atomic32Set(MTY_Atomic32 *atomic, int32_t value);

MTY_EXPORT void
MTY_Atomic64Set(MTY_Atomic64 *atomic, int64_t value);

MTY_EXPORT int32_t
MTY_Atomic32Get(MTY_Atomic32 *atomic);

MTY_EXPORT int64_t
MTY_Atomic64Get(MTY_Atomic64 *atomic);

MTY_EXPORT int32_t
MTY_Atomic32Add(MTY_Atomic32 *atomic, int32_t value);

MTY_EXPORT int64_t
MTY_Atomic64Add(MTY_Atomic64 *atomic, int64_t value);

MTY_EXPORT bool
MTY_Atomic32CAS(MTY_Atomic32 *atomic, int32_t oldValue, int32_t newValue);

MTY_EXPORT bool
MTY_Atomic64CAS(MTY_Atomic64 *atomic, int64_t oldValue, int64_t newValue);

MTY_EXPORT void
MTY_GlobalLock(MTY_Atomic32 *lock);

MTY_EXPORT void
MTY_GlobalUnlock(MTY_Atomic32 *lock);


/// @module time

MTY_EXPORT int64_t
MTY_Timestamp(void);

MTY_EXPORT float
MTY_TimeDiff(int64_t begin, int64_t end);

MTY_EXPORT void
MTY_Sleep(uint32_t timeout);

MTY_EXPORT void
MTY_SetTimerResolution(uint32_t res);

MTY_EXPORT void
MTY_RevertTimerResolution(uint32_t res);


// @module app

#define MTY_TITLE_MAX   1024
#define MTY_WINDOW_MAX  8

#define MTY_DPAD(c)       ((c)->values[MTY_CVALUE_DPAD].data)
#define MTY_DPAD_UP(c)    (MTY_DPAD(c) == 7 || MTY_DPAD(c) == 0 || MTY_DPAD(c) == 1)
#define MTY_DPAD_RIGHT(c) (MTY_DPAD(c) == 1 || MTY_DPAD(c) == 2 || MTY_DPAD(c) == 3)
#define MTY_DPAD_DOWN(c)  (MTY_DPAD(c) == 3 || MTY_DPAD(c) == 4 || MTY_DPAD(c) == 5)
#define MTY_DPAD_LEFT(c)  (MTY_DPAD(c) == 5 || MTY_DPAD(c) == 6 || MTY_DPAD(c) == 7)

typedef int8_t MTY_Window;

typedef enum {
	MTY_DETACH_NONE = 0,
	MTY_DETACH_GRAB = 1,
	MTY_DETACH_FULL = 2,
} MTY_Detach;

typedef enum {
	MTY_ORIENTATION_USER      = 0,
	MTY_ORIENTATION_LANDSCAPE = 1,
	MTY_ORIENTATION_PORTRAIT  = 2,
} MTY_Orientation;

typedef enum {
	MTY_MSG_NONE         = 0,
	MTY_MSG_CLOSE        = 1,
	MTY_MSG_QUIT         = 2,
	MTY_MSG_SHUTDOWN     = 3,
	MTY_MSG_FOCUS        = 4,
	MTY_MSG_KEYBOARD     = 5,
	MTY_MSG_HOTKEY       = 6,
	MTY_MSG_TEXT         = 7,
	MTY_MSG_MOUSE_WHEEL  = 8,
	MTY_MSG_MOUSE_BUTTON = 9,
	MTY_MSG_MOUSE_MOTION = 10,
	MTY_MSG_CONTROLLER   = 11,
	MTY_MSG_CONNECT      = 12,
	MTY_MSG_DISCONNECT   = 13,
	MTY_MSG_PEN          = 14,
	MTY_MSG_DROP         = 15,
	MTY_MSG_CLIPBOARD    = 16,
	MTY_MSG_TRAY         = 17,
	MTY_MSG_REOPEN       = 18,
	MTY_MSG_BACK         = 19,
	MTY_MSG_MAKE_32      = 0x7FFFFFFF,
} MTY_MsgType;

typedef enum {
	MTY_KEY_NONE           = 0x000,
	MTY_KEY_ESCAPE         = 0x001,
	MTY_KEY_1              = 0x002,
	MTY_KEY_2              = 0x003,
	MTY_KEY_3              = 0x004,
	MTY_KEY_4              = 0x005,
	MTY_KEY_5              = 0x006,
	MTY_KEY_6              = 0x007,
	MTY_KEY_7              = 0x008,
	MTY_KEY_8              = 0x009,
	MTY_KEY_9              = 0x00A,
	MTY_KEY_0              = 0x00B,
	MTY_KEY_MINUS          = 0x00C,
	MTY_KEY_EQUALS         = 0x00D,
	MTY_KEY_BACKSPACE      = 0x00E,
	MTY_KEY_TAB            = 0x00F,
	MTY_KEY_Q              = 0x010,
	MTY_KEY_AUDIO_PREV     = 0x110,
	MTY_KEY_W              = 0x011,
	MTY_KEY_E              = 0x012,
	MTY_KEY_R              = 0x013,
	MTY_KEY_T              = 0x014,
	MTY_KEY_Y              = 0x015,
	MTY_KEY_U              = 0x016,
	MTY_KEY_I              = 0x017,
	MTY_KEY_O              = 0x018,
	MTY_KEY_P              = 0x019,
	MTY_KEY_AUDIO_NEXT     = 0x119,
	MTY_KEY_LBRACKET       = 0x01A,
	MTY_KEY_RBRACKET       = 0x01B,
	MTY_KEY_ENTER          = 0x01C,
	MTY_KEY_NP_ENTER       = 0x11C,
	MTY_KEY_LCTRL          = 0x01D,
	MTY_KEY_RCTRL          = 0x11D,
	MTY_KEY_A              = 0x01E,
	MTY_KEY_S              = 0x01F,
	MTY_KEY_D              = 0x020,
	MTY_KEY_MUTE           = 0x120,
	MTY_KEY_F              = 0x021,
	MTY_KEY_G              = 0x022,
	MTY_KEY_AUDIO_PLAY     = 0x122,
	MTY_KEY_H              = 0x023,
	MTY_KEY_J              = 0x024,
	MTY_KEY_AUDIO_STOP     = 0x124,
	MTY_KEY_K              = 0x025,
	MTY_KEY_L              = 0x026,
	MTY_KEY_SEMICOLON      = 0x027,
	MTY_KEY_QUOTE          = 0x028,
	MTY_KEY_GRAVE          = 0x029,
	MTY_KEY_LSHIFT         = 0x02A,
	MTY_KEY_BACKSLASH      = 0x02B,
	MTY_KEY_Z              = 0x02C,
	MTY_KEY_X              = 0x02D,
	MTY_KEY_C              = 0x02E,
	MTY_KEY_VOLUME_DOWN    = 0x12E,
	MTY_KEY_V              = 0x02F,
	MTY_KEY_B              = 0x030,
	MTY_KEY_VOLUME_UP      = 0x130,
	MTY_KEY_N              = 0x031,
	MTY_KEY_M              = 0x032,
	MTY_KEY_COMMA          = 0x033,
	MTY_KEY_PERIOD         = 0x034,
	MTY_KEY_SLASH          = 0x035,
	MTY_KEY_NP_DIVIDE      = 0x135,
	MTY_KEY_RSHIFT         = 0x036,
	MTY_KEY_NP_MULTIPLY    = 0x037,
	MTY_KEY_PRINT_SCREEN   = 0x137,
	MTY_KEY_LALT           = 0x038,
	MTY_KEY_RALT           = 0x138,
	MTY_KEY_SPACE          = 0x039,
	MTY_KEY_CAPS           = 0x03A,
	MTY_KEY_F1             = 0x03B,
	MTY_KEY_F2             = 0x03C,
	MTY_KEY_F3             = 0x03D,
	MTY_KEY_F4             = 0x03E,
	MTY_KEY_F5             = 0x03F,
	MTY_KEY_F6             = 0x040,
	MTY_KEY_F7             = 0x041,
	MTY_KEY_F8             = 0x042,
	MTY_KEY_F9             = 0x043,
	MTY_KEY_F10            = 0x044,
	MTY_KEY_NUM_LOCK       = 0x045,
	MTY_KEY_SCROLL_LOCK    = 0x046,
	MTY_KEY_PAUSE          = 0x146,
	MTY_KEY_NP_7           = 0x047,
	MTY_KEY_HOME           = 0x147,
	MTY_KEY_NP_8           = 0x048,
	MTY_KEY_UP             = 0x148,
	MTY_KEY_NP_9           = 0x049,
	MTY_KEY_PAGE_UP        = 0x149,
	MTY_KEY_NP_MINUS       = 0x04A,
	MTY_KEY_NP_4           = 0x04B,
	MTY_KEY_LEFT           = 0x14B,
	MTY_KEY_NP_5           = 0x04C,
	MTY_KEY_NP_6           = 0x04D,
	MTY_KEY_RIGHT          = 0x14D,
	MTY_KEY_NP_PLUS        = 0x04E,
	MTY_KEY_NP_1           = 0x04F,
	MTY_KEY_END            = 0x14F,
	MTY_KEY_NP_2           = 0x050,
	MTY_KEY_DOWN           = 0x150,
	MTY_KEY_NP_3           = 0x051,
	MTY_KEY_PAGE_DOWN      = 0x151,
	MTY_KEY_NP_0           = 0x052,
	MTY_KEY_INSERT         = 0x152,
	MTY_KEY_NP_PERIOD      = 0x053,
	MTY_KEY_DELETE         = 0x153,
	MTY_KEY_INTL_BACKSLASH = 0x056,
	MTY_KEY_F11            = 0x057,
	MTY_KEY_F12            = 0x058,
	MTY_KEY_LWIN           = 0x15B,
	MTY_KEY_RWIN           = 0x15C,
	MTY_KEY_APP            = 0x15D,
	MTY_KEY_F13            = 0x064,
	MTY_KEY_F14            = 0x065,
	MTY_KEY_F15            = 0x066,
	MTY_KEY_F16            = 0x067,
	MTY_KEY_F17            = 0x068,
	MTY_KEY_F18            = 0x069,
	MTY_KEY_F19            = 0x06A,
	MTY_KEY_MEDIA_SELECT   = 0x16D,
	MTY_KEY_JP             = 0x070,
	MTY_KEY_RO             = 0x073,
	MTY_KEY_HENKAN         = 0x079,
	MTY_KEY_MUHENKAN       = 0x07B,
	MTY_KEY_INTL_COMMA     = 0x07E,
	MTY_KEY_YEN            = 0x07D,
	MTY_KEY_MAX            = 0x200,
	MTY_KEY_MAKE_32        = 0x7FFFFFFF,
} MTY_Key;

typedef enum {
	MTY_MOD_NONE    = 0x000,
	MTY_MOD_LSHIFT  = 0x001,
	MTY_MOD_RSHIFT  = 0x002,
	MTY_MOD_LCTRL   = 0x004,
	MTY_MOD_RCTRL   = 0x008,
	MTY_MOD_LALT    = 0x010,
	MTY_MOD_RALT    = 0x020,
	MTY_MOD_LWIN    = 0x040,
	MTY_MOD_RWIN    = 0x080,
	MTY_MOD_CAPS    = 0x100,
	MTY_MOD_NUM     = 0x200,
	MTY_MOD_SHIFT   = MTY_MOD_LSHIFT | MTY_MOD_RSHIFT,
	MTY_MOD_CTRL    = MTY_MOD_LCTRL  | MTY_MOD_RCTRL,
	MTY_MOD_ALT     = MTY_MOD_LALT   | MTY_MOD_RALT,
	MTY_MOD_WIN     = MTY_MOD_LWIN   | MTY_MOD_RWIN,
	MTY_MOD_MAKE_32 = 0x7FFFFFFF,
} MTY_Mod;

typedef enum {
	MTY_MOUSE_BUTTON_NONE    = 0,
	MTY_MOUSE_BUTTON_LEFT    = 1,
	MTY_MOUSE_BUTTON_RIGHT   = 2,
	MTY_MOUSE_BUTTON_MIDDLE  = 3,
	MTY_MOUSE_BUTTON_X1      = 4,
	MTY_MOUSE_BUTTON_X2      = 5,
	MTY_MOUSE_BUTTON_MAKE_32 = 0x7FFFFFFF,
} MTY_MouseButton;

typedef enum {
	MTY_HOTKEY_LOCAL   = 1,
	MTY_HOTKEY_GLOBAL  = 2,
	MTY_HOTKEY_MAKE_32 = 0x7FFFFFFF,
} MTY_Hotkey;

typedef enum {
	MTY_PEN_FLAG_LEAVE    = 0x01,
	MTY_PEN_FLAG_TOUCHING = 0x02,
	MTY_PEN_FLAG_INVERTED = 0x04,
	MTY_PEN_FLAG_ERASER   = 0x08,
	MTY_PEN_FLAG_BARREL   = 0x10,
	MTY_PEN_FLAG_MAKE_32  = 0x7FFFFFFF,
} MTY_PenFlag;

typedef enum {
	MTY_HID_DRIVER_DEFAULT = 1,
	MTY_HID_DRIVER_XINPUT  = 2,
	MTY_HID_DRIVER_SWITCH  = 3,
	MTY_HID_DRIVER_PS4     = 4,
	MTY_HID_DRIVER_PS5     = 5,
	MTY_HID_DRIVER_XBOX    = 6,
	MTY_HID_DRIVER_XBOXW   = 7,
} MTY_HIDDriver;

typedef enum {
	MTY_CBUTTON_X              = 0,
	MTY_CBUTTON_A              = 1,
	MTY_CBUTTON_B              = 2,
	MTY_CBUTTON_Y              = 3,
	MTY_CBUTTON_LEFT_SHOULDER  = 4,
	MTY_CBUTTON_RIGHT_SHOULDER = 5,
	MTY_CBUTTON_LEFT_TRIGGER   = 6,
	MTY_CBUTTON_RIGHT_TRIGGER  = 7,
	MTY_CBUTTON_BACK           = 8,
	MTY_CBUTTON_START          = 9,
	MTY_CBUTTON_LEFT_THUMB     = 10,
	MTY_CBUTTON_RIGHT_THUMB    = 11,
	MTY_CBUTTON_GUIDE          = 12,
	MTY_CBUTTON_TOUCHPAD       = 13,
	MTY_CBUTTON_MAX            = 64,
	MTY_CBUTTON_MAKE_32        = 0x7FFFFFFF,
} MTY_CButton;

typedef enum {
	MTY_CVALUE_THUMB_LX  = 0,
	MTY_CVALUE_THUMB_LY  = 1,
	MTY_CVALUE_THUMB_RX  = 2,
	MTY_CVALUE_THUMB_RY  = 3,
	MTY_CVALUE_TRIGGER_L = 4,
	MTY_CVALUE_TRIGGER_R = 5,
	MTY_CVALUE_DPAD      = 6,
	MTY_CVALUE_MAX       = 16,
	MTY_CVALUE_MAKE_32   = 0x7FFFFFFF,
} MTY_CValue;

typedef enum {
	MTY_POSITION_CENTER   = 0,
	MTY_POSITION_ABSOLUTE = 1,
} MTY_Position;

typedef enum {
	MTY_INPUT_UNSPECIFIED = 0,
	MTY_INPUT_TOUCHSCREEN = 1,
	MTY_INPUT_TRACKPAD    = 2,
} MTY_Input;

typedef enum {
	MTY_GFX_STATE_NORMAL      = 0,
	MTY_GFX_STATE_REFRESH     = 1,
	MTY_GFX_STATE_NEW_CONTEXT = 2,
} MTY_GFXState;

typedef struct {
	uint16_t usage;
	int16_t data;
	int16_t min;
	int16_t max;
} MTY_Value;

typedef struct {
	uint32_t id;
	uint16_t vid;
	uint16_t pid;
	uint8_t numButtons;
	uint8_t numValues;
	bool buttons[MTY_CBUTTON_MAX];
	MTY_Value values[MTY_CVALUE_MAX];
	MTY_HIDDriver driver;
} MTY_Controller;

typedef struct {
	MTY_MsgType type;
	MTY_Window window;

	union {
		MTY_Controller controller;
		const char *arg;
		uint32_t hotkey;
		uint32_t trayID;
		char text[8];
		bool focus;

		struct {
			MTY_Key key;
			MTY_Mod mod;
			bool pressed;
		} keyboard;

		struct {
			int32_t x;
			int32_t y;
			bool pixels;
		} mouseWheel;

		struct {
			int32_t x;
			int32_t y;
			bool pressed;
			MTY_MouseButton button;
		} mouseButton;

		struct {
			bool relative;
			bool synth;
			bool click;
			int32_t x;
			int32_t y;
		} mouseMotion;

		struct {
			const char *name;
			const void *data;
			size_t size;
		} drop;

		struct {
			MTY_PenFlag flags;
			uint16_t x;
			uint16_t y;
			uint16_t pressure;
			uint16_t rotation;
			int8_t tiltX;
			int8_t tiltY;
		} pen;
	};
} MTY_Msg;

typedef struct {
	wchar_t label[256];
	uint32_t trayID;
	bool (*checked)(void *opaque);
} MTY_MenuItem;

typedef struct {
	MTY_Position position;
	MTY_GFX api;
	uint32_t width;
	uint32_t height;
	uint32_t minWidth;
	uint32_t minHeight;
	uint32_t x;
	uint32_t y;
	float maxHeight;
	bool fullscreen;
	bool hidden;
	bool vsync;
} MTY_WindowDesc;

typedef bool (*MTY_AppFunc)(void *opaque);
typedef void (*MTY_MsgFunc)(const MTY_Msg *wmsg, void *opaque);

typedef struct MTY_App MTY_App;

MTY_EXPORT MTY_App *
MTY_AppCreate(MTY_AppFunc appFunc, MTY_MsgFunc msgFunc, void *opaque);

MTY_EXPORT void
MTY_AppDestroy(MTY_App **app);

MTY_EXPORT void
MTY_AppRun(MTY_App *ctx);

MTY_EXPORT void
MTY_AppSetTimeout(MTY_App *ctx, uint32_t timeout);

MTY_EXPORT bool
MTY_AppIsActive(MTY_App *ctx);

MTY_EXPORT void
MTY_AppActivate(MTY_App *ctx, bool active);

MTY_EXPORT MTY_Detach
MTY_AppGetDetached(MTY_App *ctx);

MTY_EXPORT void
MTY_AppDetach(MTY_App *ctx, MTY_Detach type);

MTY_EXPORT void
MTY_AppEnableScreenSaver(MTY_App *ctx, bool enable);

MTY_EXPORT char *
MTY_AppGetClipboard(MTY_App *ctx);

MTY_EXPORT void
MTY_AppSetClipboard(MTY_App *ctx, const char *text);

MTY_EXPORT void
MTY_AppGrabKeyboard(MTY_App *ctx, bool grab);

MTY_EXPORT void
MTY_AppGrabMouse(MTY_App *ctx, bool grab);

MTY_EXPORT void
MTY_AppSetTray(MTY_App *ctx, const char *tooltip, const MTY_MenuItem *items, uint32_t len);

MTY_EXPORT void
MTY_AppRemoveTray(MTY_App *ctx);

MTY_EXPORT void
MTY_AppNotification(MTY_App *ctx, const char *title, const char *msg);

MTY_EXPORT void
MTY_AppSetHotkey(MTY_App *ctx, MTY_Hotkey mode, MTY_Mod mod, MTY_Key key, uint32_t id);

MTY_EXPORT void
MTY_AppRemoveHotkeys(MTY_App *ctx, MTY_Hotkey mode);

MTY_EXPORT bool
MTY_AppGetRelativeMouse(MTY_App *ctx);

MTY_EXPORT void
MTY_AppSetRelativeMouse(MTY_App *ctx, bool relative);

MTY_EXPORT void
MTY_AppSetPNGCursor(MTY_App *ctx, const void *image, size_t size, uint32_t hotX, uint32_t hotY);

MTY_EXPORT void
MTY_AppUseDefaultCursor(MTY_App *ctx, bool useDefault);

MTY_EXPORT void
MTY_AppShowCursor(MTY_App *ctx, bool show);

MTY_EXPORT bool
MTY_AppCanWarpCursor(MTY_App *ctx);

MTY_EXPORT void
MTY_AppHotkeyToString(MTY_Mod mod, MTY_Key key, char *str, size_t len);

MTY_EXPORT void
MTY_AppEnableGlobalHotkeys(MTY_App *ctx, bool enable);

MTY_EXPORT void
MTY_AppShowSoftKeyboard(MTY_App *ctx, bool show);

MTY_EXPORT bool
MTY_AppSoftKeyboardIsShowing(MTY_App *app);

MTY_EXPORT void
MTY_AppSetOrientation(MTY_App *ctx, MTY_Orientation orientation);

MTY_EXPORT void
MTY_AppControllerRumble(MTY_App *ctx, uint32_t id, uint16_t low, uint16_t high);

MTY_EXPORT void
MTY_AppEnablePen(MTY_App *ctx, bool enable);

MTY_EXPORT void
MTY_AppSetInputMode(MTY_App *ctx, MTY_Input mode);

MTY_EXPORT MTY_Window
MTY_WindowCreate(MTY_App *app, const char *title, const MTY_WindowDesc *desc);

MTY_EXPORT void
MTY_WindowDestroy(MTY_App *app, MTY_Window window);

MTY_EXPORT bool
MTY_WindowGetSize(MTY_App *app, MTY_Window window, uint32_t *width, uint32_t *height);

MTY_EXPORT bool
MTY_WindowGetScreenSize(MTY_App *app, MTY_Window window, uint32_t *width, uint32_t *height);

MTY_EXPORT float
MTY_WindowGetScale(MTY_App *app, MTY_Window window);

MTY_EXPORT void
MTY_WindowSetTitle(MTY_App *app, MTY_Window window, const char *title);

MTY_EXPORT bool
MTY_WindowIsVisible(MTY_App *app, MTY_Window window);

MTY_EXPORT bool
MTY_WindowIsActive(MTY_App *app, MTY_Window window);

MTY_EXPORT void
MTY_WindowActivate(MTY_App *app, MTY_Window window, bool active);

MTY_EXPORT bool
MTY_WindowExists(MTY_App *app, MTY_Window window);

MTY_EXPORT bool
MTY_WindowIsFullscreen(MTY_App *app, MTY_Window window);

MTY_EXPORT void
MTY_WindowEnableFullscreen(MTY_App *app, MTY_Window window, bool fullscreen);

MTY_EXPORT void
MTY_WindowWarpCursor(MTY_App *app, MTY_Window window, uint32_t x, uint32_t y);

MTY_EXPORT MTY_Device *
MTY_WindowGetDevice(MTY_App *app, MTY_Window window);

MTY_EXPORT MTY_Context *
MTY_WindowGetContext(MTY_App *app, MTY_Window window);

MTY_EXPORT MTY_Texture *
MTY_WindowGetBackBuffer(MTY_App *app, MTY_Window window);

MTY_EXPORT void
MTY_WindowDrawQuad(MTY_App *app, MTY_Window window, const void *image, const MTY_RenderDesc *desc);

MTY_EXPORT void
MTY_WindowDrawUI(MTY_App *app, MTY_Window window, const MTY_DrawData *dd);

MTY_EXPORT void
MTY_WindowPresent(MTY_App *app, MTY_Window window, uint32_t numFrames);

MTY_EXPORT void *
MTY_WindowGetUITexture(MTY_App *app, MTY_Window window, uint32_t id);

MTY_EXPORT void
MTY_WindowSetUITexture(MTY_App *app, MTY_Window window, uint32_t id, const void *rgba, uint32_t width, uint32_t height);

MTY_EXPORT MTY_GFX
MTY_WindowGetGFX(MTY_App *app, MTY_Window window);

MTY_EXPORT bool
MTY_WindowSetGFX(MTY_App *app, MTY_Window window, MTY_GFX api, bool vsync);

MTY_EXPORT MTY_GFXState
MTY_WindowGFXState(MTY_App *app, MTY_Window window);


// @module gl

MTY_EXPORT void *
MTY_GLGetProcAddress(const char *name);


// @module platform

MTY_EXPORT void *
MTY_JNIEnv(void);


// @module net

typedef enum {
	MTY_ASYNC_OK       = 0,
	MTY_ASYNC_DONE     = 1,
	MTY_ASYNC_CONTINUE = 2,
	MTY_ASYNC_ERROR    = 3,
} MTY_Async;

typedef void (*MTY_HttpAsyncFunc)(uint16_t code, void **body, size_t *size);

typedef struct MTY_WebSocket MTY_WebSocket;

MTY_EXPORT bool
MTY_HttpSetCACert(const char *cacert, size_t size);

MTY_EXPORT bool
MTY_HttpSetProxy(const char *proxy);

MTY_EXPORT bool
MTY_HttpParseUrl(const char *url, char *host, size_t hostSize, char *path, size_t pathSize);

MTY_EXPORT void
MTY_HttpEncodeUrl(const char *src, char *dst, size_t size);

MTY_EXPORT bool
MTY_HttpRequest(const char *host, bool secure, const char *method, const char *path,
	const char *headers, const void *body, size_t bodySize, uint32_t timeout,
	void **response, size_t *responseSize, uint16_t *status);

MTY_EXPORT void
MTY_HttpAsyncCreate(uint32_t maxThreads);

MTY_EXPORT void
MTY_HttpAsyncDestroy(void);

MTY_EXPORT void
MTY_HttpAsyncRequest(uint32_t *index, const char *host, bool secure, const char *method,
	const char *path, const char *headers, const void *body, size_t size, uint32_t timeout,
	MTY_HttpAsyncFunc func);

MTY_EXPORT MTY_Async
MTY_HttpAsyncPoll(uint32_t index, void **response, size_t *size, uint16_t *status);

MTY_EXPORT void
MTY_HttpAsyncClear(uint32_t *index);

MTY_EXPORT MTY_WebSocket *
MTY_WebSocketListen(const char *host, uint16_t port);

MTY_EXPORT MTY_WebSocket *
MTY_WebSocketAccept(MTY_WebSocket *ws, const char * const *origins, uint32_t numOrigins,
	bool secureOrigin, uint32_t timeout);

MTY_EXPORT MTY_WebSocket *
MTY_WebSocketConnect(const char *host, uint16_t port, bool secure, const char *path,
	const char *headers, uint32_t timeout, uint16_t *upgradeStatus);

MTY_EXPORT void
MTY_WebSocketDestroy(MTY_WebSocket **ws);

MTY_EXPORT MTY_Async
MTY_WebSocketRead(MTY_WebSocket *ws, char *msg, size_t size, uint32_t timeout);

MTY_EXPORT bool
MTY_WebSocketWrite(MTY_WebSocket *ws, const char *msg);

MTY_EXPORT uint16_t
MTY_WebSocketGetCloseCode(MTY_WebSocket *ctx);


// @module tls

#define MTY_FINGERPRINT_MAX 512

typedef enum {
	MTY_TLS_TYPE_TLS  = 1,
	MTY_TLS_TYPE_DTLS = 2,
} MTY_TLSType;

typedef bool (*MTY_TLSWriteFunc)(const void *buf, size_t size, void *opaque);

typedef struct MTY_Cert MTY_Cert;
typedef struct MTY_TLS MTY_TLS;

MTY_EXPORT MTY_Cert *
MTY_CertCreate(void);

MTY_EXPORT void
MTY_CertGetFingerprint(MTY_Cert *ctx, char *fingerprint, size_t size);

MTY_EXPORT void
MTY_CertDestroy(MTY_Cert **cert);

MTY_EXPORT MTY_TLS *
MTY_TLSCreate(MTY_TLSType type, MTY_Cert *cert, const char *host, const char *peerFingerprint, uint32_t mtu);

MTY_EXPORT void
MTY_TLSDestroy(MTY_TLS **dtls);

MTY_EXPORT MTY_Async
MTY_TLSHandshake(MTY_TLS *ctx, const void *buf, size_t size, MTY_TLSWriteFunc writeFunc, void *opaque);

MTY_EXPORT bool
MTY_TLSEncrypt(MTY_TLS *ctx, const void *in, size_t inSize, void *out, size_t outSize, size_t *written);

MTY_EXPORT bool
MTY_TLSDecrypt(MTY_TLS *ctx, const void *in, size_t inSize, void *out, size_t outSize, size_t *read);

MTY_EXPORT bool
MTY_TLSIsHandshake(const void *buf, size_t size);

MTY_EXPORT bool
MTY_TLSIsApplicationData(const void *buf, size_t size);


#ifdef __cplusplus
}
#endif
