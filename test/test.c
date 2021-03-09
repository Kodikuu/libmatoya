#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <inttypes.h>

#include "matoya.h"


// Framework

#define CRED   "\033[31m"
#define CGREEN "\033[32m"
#define CCLEAR "\033[0m"

#define test_cmp_(name, cmp, val, fmt) { \
	printf("%-20s%-35s%s" fmt "\n", name, #cmp, \
		(cmp) ? CGREEN "Passed" CCLEAR : CRED "Failed" CCLEAR, val); \
	if (!(cmp)) return false; \
}

#define test_cmp(name, cmp) \
	test_cmp_(name, cmp, "", "%s")

#define test_cmpf(name, cmp, val) \
	test_cmp_(name, cmp, val, ": %.2f")

#define test_cmpi64(name, cmp, val) \
	test_cmp_(name, cmp, val, ": %" PRId64)


// AESGCM

#define AES_BUF_LEN 1024

static bool test_aesgcm_performance(void)
{
	uint32_t n = 5000000;
	uint8_t dummy[AES_BUF_LEN];
	uint8_t dummy2[AES_BUF_LEN];
	uint8_t key[16] = "8kdmanxjke8jds";
	uint8_t iv[12] = "abnmfkedlai";
	uint8_t tag[16];

	MTY_AESGCM *aesgcm = MTY_AESGCMCreate(key);

	int64_t begin = MTY_Timestamp();
	for (uint32_t x = 0; x < n; x++) {
		dummy[x % 1024] = x & 0xFF;
		MTY_AESGCMEncrypt(aesgcm, iv, dummy, AES_BUF_LEN, tag, dummy2);
		MTY_AESGCMDecrypt(aesgcm, iv, dummy2, AES_BUF_LEN, tag, dummy);
	}

	MTY_AESGCMDestroy(&aesgcm);

	float diff = MTY_TimeDiff(begin, MTY_Timestamp());
	test_cmpf("AESGCM", diff < 4000.0f, diff);

	return true;
}


// time

static bool test_time(void)
{
	int64_t ts = MTY_Timestamp();
	test_cmpi64("MTY_Timestamp", ts > 0, ts);

	MTY_Sleep(100);
	test_cmp("MTY_Sleep", 100);

	float diff = MTY_TimeDiff(ts, MTY_Timestamp());
	test_cmpf("MTY_TimeDiff", diff >= 95.0f && diff <= 105.0f, diff);

	return true;
}


// fs

#define TEST_FILE MTY_Path(".", "test.file")

static bool test_fs(void)
{
	const char *data = "This is arbitrary data.";

	bool r = MTY_WriteFile(TEST_FILE, data, strlen(data));
	test_cmp("MTY_WriteFile", r);

	r = MTY_FileExists(TEST_FILE);
	test_cmp("MTY_FileExists", r);

	size_t size = 0;
	char *rdata = MTY_ReadFile(TEST_FILE, &size);
	test_cmp("MTY_ReadFile", rdata && size == strlen(data));
	test_cmp("MTY_ReadFile", !strcmp(rdata, data));
	MTY_Free(rdata);

	r = MTY_DeleteFile(TEST_FILE);
	test_cmp("MTY_DeleteFile", r);

	r = MTY_FileExists(TEST_FILE);
	test_cmp("MTY_FileExists", !r);

	return true;
}


// Main

int32_t main(int32_t argc, char **argv)
{
	MTY_SetTimerResolution(1);

	if (!test_time())
		return 1;

	if (!test_fs())
		return 1;

	if (!test_aesgcm_performance())
		return 1;

	MTY_RevertTimerResolution(1);

	return 0;
}
