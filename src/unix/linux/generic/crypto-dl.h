// Copyright (c) 2020 Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once


// Interface

#if defined(MTY_CRYPTO_EXTERNAL)

#define FP(sym) sym
#define STATIC

#else

#define FP(sym) (*sym)
#define STATIC static
#endif

#define EVP_CTRL_AEAD_GET_TAG 0x10
#define EVP_CTRL_GCM_GET_TAG  EVP_CTRL_AEAD_GET_TAG

#define EVP_CTRL_AEAD_SET_TAG 0x11
#define EVP_CTRL_GCM_SET_TAG  EVP_CTRL_AEAD_SET_TAG

typedef struct engine_st ENGINE;
typedef struct evp_cipher_st EVP_CIPHER;
typedef struct evp_cipher_ctx_st EVP_CIPHER_CTX;
typedef struct evp_md_st EVP_MD;

STATIC const EVP_CIPHER *FP(EVP_aes_128_gcm)(void);
STATIC EVP_CIPHER_CTX *FP(EVP_CIPHER_CTX_new)(void);
STATIC void FP(EVP_CIPHER_CTX_free)(EVP_CIPHER_CTX *c);
STATIC int FP(EVP_CipherInit_ex)(EVP_CIPHER_CTX *ctx, const EVP_CIPHER *cipher, ENGINE *impl,
	const unsigned char *key, const unsigned char *iv, int enc);
STATIC int FP(EVP_EncryptUpdate)(EVP_CIPHER_CTX *ctx, unsigned char *out, int *outl,
	const unsigned char *in, int inl);
STATIC int FP(EVP_DecryptUpdate)(EVP_CIPHER_CTX *ctx, unsigned char *out, int *outl,
	const unsigned char *in, int inl);
STATIC int FP(EVP_EncryptFinal_ex)(EVP_CIPHER_CTX *ctx, unsigned char *out, int *outl);
STATIC int FP(EVP_DecryptFinal_ex)(EVP_CIPHER_CTX *ctx, unsigned char *outm, int *outl);
STATIC int FP(EVP_CIPHER_CTX_ctrl)(EVP_CIPHER_CTX *ctx, int type, int arg, void *ptr);

STATIC const EVP_MD *FP(EVP_sha1)(void);
STATIC const EVP_MD *FP(EVP_sha256)(void);
STATIC unsigned char *FP(SHA1)(const unsigned char *d, size_t n, unsigned char *md);
STATIC unsigned char *FP(SHA256)(const unsigned char *d, size_t n, unsigned char *md);
STATIC unsigned char *FP(HMAC)(const EVP_MD *evp_md, const void *key, int key_len,
	const unsigned char *d, size_t n, unsigned char *md, unsigned int *md_len);

STATIC int FP(RAND_bytes)(unsigned char *buf, int num);


// Runtime open

#if defined(MTY_CRYPTO_EXTERNAL)

#define crypto_dl_global_init() true
#define crypto_dl_global_destroy()

#else

static MTY_Atomic32 CRYPTO_DL_LOCK;
static MTY_SO *CRYPTO_DL_SO;
static bool CRYPTO_DL_INIT;

static void crypto_dl_global_destroy(void)
{
	MTY_SOUnload(&CRYPTO_DL_SO);
	CRYPTO_DL_INIT = false;
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

#endif
