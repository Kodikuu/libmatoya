// Copyright (c) Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "matoya.h"

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <float.h>
#include <string.h>
#include <limits.h>
#include <math.h>

#include "cJSON.h"

MTY_JSON *MTY_JSONParse(const char *input)
{
	return (MTY_JSON *) cJSON_Parse(input);
}

char *MTY_JSONStringify(const MTY_JSON *json)
{
	cJSON *cj = (cJSON *) json;

	if (!cj)
		return MTY_Strdup("null");

	return cJSON_PrintUnformatted(cj);
}

MTY_JSON *MTY_JSONReadFile(const char *path)
{
	MTY_JSON *j = NULL;
	void *jstr = MTY_ReadFile(path, NULL);

	if (jstr)
		j = MTY_JSONParse(jstr);

	MTY_Free(jstr);

	return j;
}

bool MTY_JSONWriteFile(const char *path, const MTY_JSON *json)
{
	char *jstr = MTY_JSONStringify(json);

	bool r = MTY_WriteFile(path, jstr, strlen(jstr));
	MTY_Free(jstr);

	return r;
}

MTY_JSON *MTY_JSONDuplicate(const MTY_JSON *json)
{
	return (MTY_JSON *) (!json ? cJSON_CreateNull() : cJSON_Duplicate((cJSON *) json, true));
}

uint32_t MTY_JSONLength(const MTY_JSON *json)
{
	cJSON *cj = (cJSON *) json;

	// This will work on both arrays and objects
	return !cj ? 0 : cJSON_GetArraySize(cj);
}

void MTY_JSONDestroy(MTY_JSON **json)
{
	if (!json || !*json)
		return;

	cJSON_Delete((cJSON *) *json);

	*json = NULL;
}

bool MTY_JSONToString(const MTY_JSON *json, char *value, size_t size)
{
	cJSON *cj = (cJSON *) json;

	if (!cj || !cJSON_IsString(cj))
		return false;

	snprintf(value, size, "%s", cj->valuestring);

	return true;
}

bool MTY_JSONToInt(const MTY_JSON *json, int32_t *value)
{
	cJSON *cj = (cJSON *) json;

	if (!cj || !cJSON_IsNumber(cj))
		return false;

	*value = cj->valueint;

	return true;
}

bool MTY_JSONToUInt(const MTY_JSON *json, uint32_t *value)
{
	return MTY_JSONToInt(json, (int32_t *) value);
}

bool MTY_JSONToInt16(const MTY_JSON *json, int16_t *value)
{
	int32_t v = 0;
	bool r = MTY_JSONToInt(json, &v);

	*value = (int16_t) v;

	return r;
}

bool MTY_JSONToUInt16(const MTY_JSON *json, uint16_t *value)
{
	return MTY_JSONToInt16(json, (int16_t *) value);
}

bool MTY_JSONToInt8(const MTY_JSON *json, int8_t *value)
{
	int32_t v = 0;
	bool r = MTY_JSONToInt(json, &v);

	*value = (int8_t) v;

	return r;
}

bool MTY_JSONToUInt8(const MTY_JSON *json, uint8_t *value)
{
	return MTY_JSONToInt8(json, (int8_t *) value);
}

bool MTY_JSONToFloat(const MTY_JSON *json, float *value)
{
	cJSON *cj = (cJSON *) json;

	if (!cj || !cJSON_IsNumber(cj))
		return false;

	*value = (float) cj->valuedouble;

	return true;
}

bool MTY_JSONToBool(const MTY_JSON *json, bool *value)
{
	cJSON *cj = (cJSON *) json;

	if (!cj || !cJSON_IsBool(cj))
		return false;

	// cJSON stores bool information as part of the type
	// XXX DO NOT be tempted to use item->valueint!
	*value = cj->type & cJSON_True;

	return true;
}

bool MTY_JSONIsNull(const MTY_JSON *json)
{
	if (!json)
		return false;

	return cJSON_IsNull((cJSON *) json);
}

MTY_JSON *MTY_JSONFromString(const char *value)
{
	return (MTY_JSON *) cJSON_CreateString(value);
}

MTY_JSON *MTY_JSONFromInt(int32_t value)
{
	return (MTY_JSON *) cJSON_CreateNumber((double) value);
}

MTY_JSON *MTY_JSONFromUInt(uint32_t value)
{
	return MTY_JSONFromInt((int32_t) value);
}

MTY_JSON *MTY_JSONFromFloat(float value)
{
	return (MTY_JSON *) cJSON_CreateNumber((double) value);
}

MTY_JSON *MTY_JSONFromBool(bool value)
{
	return (MTY_JSON *) cJSON_CreateBool(value);
}

MTY_JSON *MTY_JSONNull(void)
{
	return (MTY_JSON *) cJSON_CreateNull();
}

MTY_JSON *MTY_JSONObj(void)
{
	return (MTY_JSON *) cJSON_CreateObject();
}

MTY_JSON *MTY_JSONArray(void)
{
	return (MTY_JSON *) cJSON_CreateArray();
}

bool MTY_JSONObjKeyExists(const MTY_JSON *json, const char *key)
{
	if (!json)
		return false;

	return cJSON_GetObjectItemCaseSensitive((cJSON *) json, key) ? true : false;
}

const char *MTY_JSONObjGetKey(const MTY_JSON *json, uint32_t index)
{
	cJSON *cj = (cJSON *) json;

	if (!cj || !cJSON_IsObject(cj))
		return NULL;

	cJSON *item = cJSON_GetArrayItem(cj, index);
	if (!item)
		return NULL;

	return item->string;
}

void MTY_JSONObjDeleteKey(MTY_JSON *json, const char *key)
{
	if (!json)
		return;

	cJSON_DeleteItemFromObjectCaseSensitive((cJSON *) json, key);
}

const MTY_JSON *MTY_JSONObjGet(const MTY_JSON *json, const char *key)
{
	const cJSON *cj = (const cJSON *) json;

	if (!cj || !cJSON_IsObject(cj))
		return NULL;

	return (const MTY_JSON *) cJSON_GetObjectItemCaseSensitive(cj, key);
}

void MTY_JSONObjSet(MTY_JSON *json, const char *key, const MTY_JSON *value)
{
	cJSON *cj = (cJSON *) json;

	if (!cj || !cJSON_IsObject(cj) || !value)
		return;

	if (cJSON_GetObjectItemCaseSensitive(cj, key)) {
		cJSON_ReplaceItemInObjectCaseSensitive(cj, key, (cJSON *) value);

	} else {
		cJSON_AddItemToObject(cj, key, (cJSON *) value);
	}
}

bool MTY_JSONArrayIndexExists(const MTY_JSON *json, uint32_t index)
{
	if (!json)
		return false;

	return cJSON_GetArrayItem((cJSON *) json, index) ? true : false;
}

void MTY_JSONArrayDeleteIndex(MTY_JSON *json, uint32_t index)
{
	if (!json)
		return;

	cJSON_DeleteItemFromArray((cJSON *) json, index);
}

const MTY_JSON *MTY_JSONArrayGet(const MTY_JSON *json, uint32_t index)
{
	const cJSON *cj = (const cJSON *) json;

	// cJSON allows you to iterate through and index objects
	if (!cj || (!cJSON_IsArray(cj) && !cJSON_IsObject(cj)))
		return NULL;

	return (const MTY_JSON *) cJSON_GetArrayItem(cj, index);
}

void MTY_JSONArraySet(MTY_JSON *json, uint32_t index, const MTY_JSON *value)
{
	cJSON *cj = (cJSON *) json;

	if (!cj || !cJSON_IsArray(cj) || !value)
		return;

	cJSON_InsertItemInArray(cj, index, (cJSON *) value);
}

void MTY_JSONArrayAppend(MTY_JSON *json, const MTY_JSON *value)
{
	cJSON *cj = (cJSON *) json;

	if (!cj || !cJSON_IsArray(cj) || !value)
		return;

	cJSON_AddItemToArray(cj, (cJSON *) value);
}
