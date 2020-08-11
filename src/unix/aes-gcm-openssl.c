// Copyright (c) 2020 Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "matoya.h"

#include "crypto-dl.h"

struct MTY_AESGCM {
	EVP_CIPHER_CTX *enc;
	EVP_CIPHER_CTX *dec;
};

bool MTY_AESGCMCreate(const void *key, size_t keySize, MTY_AESGCM **aesgcm)
{
	if (!crypto_dl_global_init())
		return false;

	MTY_AESGCM *ctx = *aesgcm = MTY_Alloc(1, sizeof(MTY_AESGCM));
	bool r = true;

	const EVP_CIPHER *cipher = NULL;
	switch (keySize) {
		case 16:
			cipher = EVP_aes_128_gcm();
			break;
		case 24:
			cipher = EVP_aes_192_gcm();
			break;
		case 32:
			cipher = EVP_aes_256_gcm();
			break;
		default:
			MTY_Log("Invalid AES key size %zu", keySize);
			r = false;
			goto except;
	}

	ctx->enc = EVP_CIPHER_CTX_new();
	if (!ctx->enc) {
		MTY_Log("'EVP_CIPHER_CTX_new' failed");
		r = false;
		goto except;
	}

	ctx->dec = EVP_CIPHER_CTX_new();
	if (!ctx->dec) {
		MTY_Log("'EVP_CIPHER_CTX_new' failed");
		r = false;
		goto except;
	}

	int32_t e = EVP_CipherInit_ex(ctx->enc, cipher, NULL, key, NULL, 1);
	if (e != 1) {
		MTY_Log("'EVP_CipherInit_ex' failed with error %d", e);
		r = false;
		goto except;
	}

	e = EVP_CipherInit_ex(ctx->dec, cipher, NULL, key, NULL, 0);
	if (e != 1) {
		MTY_Log("'EVP_CipherInit_ex' failed with error %d", e);
		r = false;
		goto except;
	}

	except:

	if (!r)
		MTY_AESGCMDestroy(aesgcm);

	return r;
}

bool MTY_AESGCMEncrypt(MTY_AESGCM *ctx, const void *plainText, size_t textSize, const void *nonce,
	size_t nonceSize, void *hash, size_t hashSize, void *cipherText)
{
	if (nonceSize < 12) {
		MTY_Log("'nonceSize' is less than 12 bytes");
		return false;
	}

	int32_t e = EVP_CipherInit_ex(ctx->enc, NULL, NULL, NULL, nonce, 1);
	if (e != 1) {
		MTY_Log("'EVP_CipherInit' failed with error %d", e);
		return false;
	}

	int32_t len = 0;
	e = EVP_EncryptUpdate(ctx->enc, cipherText, &len, plainText, textSize);
	if (e != 1) {
		MTY_Log("'EVP_EncryptUpdate' failed with error %d", e);
		return false;
	}

	e = EVP_EncryptFinal_ex(ctx->enc, cipherText, &len);
	if (e != 1) {
		MTY_Log("'EVP_EncryptFinal_ex' failed with error %d", e);
		return false;
	}

	e = EVP_CIPHER_CTX_ctrl(ctx->enc, EVP_CTRL_GCM_GET_TAG, hashSize, hash);
	if (e != 1) {
		MTY_Log("'EVP_CIPHER_CTX_ctrl' failed with error %d", e);
		return false;
	}

	return true;
}

bool MTY_AESGCMDecrypt(MTY_AESGCM *ctx, const void *cipherText, size_t textSize, const void *nonce,
	size_t nonceSize, const void *hash, size_t hashSize, void *plainText)
{
	if (nonceSize < 12) {
		MTY_Log("'nonceSize' is less than 12 bytes");
		return false;
	}

	int32_t e = EVP_CipherInit_ex(ctx->dec, NULL, NULL, NULL, nonce, 0);
	if (e != 1) {
		MTY_Log("'EVP_CipherInit' failed with error %d", e);
		return false;
	}

	int32_t len = 0;
	e = EVP_DecryptUpdate(ctx->dec, plainText, &len, cipherText, textSize);
	if (e != 1) {
		MTY_Log("'EVP_DecryptUpdate' failed with error %d", e);
		return false;
	}

	e = EVP_CIPHER_CTX_ctrl(ctx->dec, EVP_CTRL_GCM_SET_TAG, hashSize, (void *) hash);
	if (e != 1) {
		MTY_Log("'EVP_CIPHER_CTX_ctrl' failed with error %d", e);
		return false;
	}

	e = EVP_DecryptFinal_ex(ctx->dec, plainText, &len);
	if (e != 1) {
		MTY_Log("'EVP_DecryptFinal_ex' failed with error %d", e);
		return false;
	}

	return true;
}

void MTY_AESGCMDestroy(MTY_AESGCM **aesgcm)
{
	if (!aesgcm || !*aesgcm)
		return;

	MTY_AESGCM *ctx = *aesgcm;

	if (ctx->dec)
		EVP_CIPHER_CTX_free(ctx->dec);

	if (ctx->enc)
		EVP_CIPHER_CTX_free(ctx->enc);

	MTY_Free(ctx);
	*aesgcm = NULL;
}
