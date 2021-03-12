// Copyright (c) Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the MIT License.
// If a copy of the MIT License was not distributed with this file,
// You can obtain one at https://spdx.org/licenses/MIT.html.

#pragma once


// Interface

#define EVP_CTRL_AEAD_GET_TAG 0x10
#define EVP_CTRL_GCM_GET_TAG  EVP_CTRL_AEAD_GET_TAG

#define EVP_CTRL_AEAD_SET_TAG 0x11
#define EVP_CTRL_GCM_SET_TAG  EVP_CTRL_AEAD_SET_TAG

typedef struct engine_st ENGINE;
typedef struct evp_cipher_st EVP_CIPHER;
typedef struct evp_cipher_ctx_st EVP_CIPHER_CTX;
typedef struct evp_md_st EVP_MD;

static const EVP_CIPHER *(*EVP_aes_128_gcm)(void);
static EVP_CIPHER_CTX *(*EVP_CIPHER_CTX_new)(void);
static void (*EVP_CIPHER_CTX_free)(EVP_CIPHER_CTX *c);
static int (*EVP_CipherInit_ex)(EVP_CIPHER_CTX *ctx, const EVP_CIPHER *cipher, ENGINE *impl,
	const unsigned char *key, const unsigned char *iv, int enc);
static int (*EVP_EncryptUpdate)(EVP_CIPHER_CTX *ctx, unsigned char *out, int *outl,
	const unsigned char *in, int inl);
static int (*EVP_DecryptUpdate)(EVP_CIPHER_CTX *ctx, unsigned char *out, int *outl,
	const unsigned char *in, int inl);
static int (*EVP_EncryptFinal_ex)(EVP_CIPHER_CTX *ctx, unsigned char *out, int *outl);
static int (*EVP_DecryptFinal_ex)(EVP_CIPHER_CTX *ctx, unsigned char *outm, int *outl);
static int (*EVP_CIPHER_CTX_ctrl)(EVP_CIPHER_CTX *ctx, int type, int arg, void *ptr);

static const EVP_MD *(*EVP_sha1)(void);
static const EVP_MD *(*EVP_sha256)(void);
static unsigned char *(*SHA1)(const unsigned char *d, size_t n, unsigned char *md);
static unsigned char *(*SHA256)(const unsigned char *d, size_t n, unsigned char *md);
static unsigned char *(*HMAC)(const EVP_MD *evp_md, const void *key, int key_len,
	const unsigned char *d, size_t n, unsigned char *md, unsigned int *md_len);

static int (*RAND_bytes)(unsigned char *buf, int num);


// Runtime open

static MTY_Atomic32 CRYPTO_DL_LOCK;
static MTY_SO *CRYPTO_DL_SO;
static bool CRYPTO_DL_INIT;

static void __attribute__((destructor)) crypto_dl_global_destroy(void)
{
	MTY_GlobalLock(&CRYPTO_DL_LOCK);

	MTY_SOUnload(&CRYPTO_DL_SO);
	CRYPTO_DL_INIT = false;

	MTY_GlobalUnlock(&CRYPTO_DL_LOCK);
}

static bool crypto_dl_global_init(void)
{
	MTY_GlobalLock(&CRYPTO_DL_LOCK);

	if (!CRYPTO_DL_INIT) {
		bool r = true;
		CRYPTO_DL_SO = MTY_SOLoad("libcrypto.so.1.1");

		if (!CRYPTO_DL_SO)
			CRYPTO_DL_SO = MTY_SOLoad("libcrypto.so.1.0.0");

		if (!CRYPTO_DL_SO) {
			r = false;
			goto except;
		}

		#define LOAD_SYM(so, name) \
			name = MTY_SOGetSymbol(so, #name); \
			if (!name) {r = false; goto except;}

		LOAD_SYM(CRYPTO_DL_SO, EVP_aes_128_gcm);
		LOAD_SYM(CRYPTO_DL_SO, EVP_CIPHER_CTX_new);
		LOAD_SYM(CRYPTO_DL_SO, EVP_CIPHER_CTX_free);
		LOAD_SYM(CRYPTO_DL_SO, EVP_CipherInit_ex);
		LOAD_SYM(CRYPTO_DL_SO, EVP_EncryptUpdate);
		LOAD_SYM(CRYPTO_DL_SO, EVP_DecryptUpdate);
		LOAD_SYM(CRYPTO_DL_SO, EVP_EncryptFinal_ex);
		LOAD_SYM(CRYPTO_DL_SO, EVP_DecryptFinal_ex);
		LOAD_SYM(CRYPTO_DL_SO, EVP_CIPHER_CTX_ctrl);

		LOAD_SYM(CRYPTO_DL_SO, EVP_sha1);
		LOAD_SYM(CRYPTO_DL_SO, EVP_sha256);
		LOAD_SYM(CRYPTO_DL_SO, SHA1);
		LOAD_SYM(CRYPTO_DL_SO, SHA256);
		LOAD_SYM(CRYPTO_DL_SO, HMAC);

		LOAD_SYM(CRYPTO_DL_SO, RAND_bytes);

		except:

		if (!r)
			crypto_dl_global_destroy();

		CRYPTO_DL_INIT = r;
	}

	MTY_GlobalUnlock(&CRYPTO_DL_LOCK);

	return CRYPTO_DL_INIT;
}
