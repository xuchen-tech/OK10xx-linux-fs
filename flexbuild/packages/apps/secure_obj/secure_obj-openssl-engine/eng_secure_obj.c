/*
  * Copyright 2017 NXP
  * SPDX-License-Identifier:     BSD-3-Clause
*/

#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <sched.h>
#include <errno.h>
#include <pthread.h>
#include <stdint.h>
#include <stdbool.h>

#include <openssl/objects.h>
#include <openssl/engine.h>
#include <openssl/bn.h>
#include <openssl/crypto.h>
#include <crypto/evp/evp_locl.h>
#include <openssl/evp.h>
#include <openssl/ossl_typ.h>
#include <openssl/crypto.h>
#include <openssl/pem.h>

#include <openssl/hmac.h>
#include <openssl/md5.h>
#include <openssl/rand.h>
#include <openssl/tls1.h>

#include "securekey_api.h"

#define	SOBJ_KEY_ID	0xE1E2E3E4
#define	PRINT_ERROR

#ifdef PRINT_ERROR
#define print_error(msg, ...) { \
printf("[SECURE_OBJ_ENG:%s, %d] Error: ", __func__, __LINE__); \
printf(msg, ##__VA_ARGS__); \
}
#else
#define print_error(msg, ...)
#endif

#ifdef PRINT_INFO
#define print_info(msg, ...) { \
printf("[SECURE_OBJ_ENG:%s, %d] Info: ", __func__, __LINE__); \
printf(msg, ##__VA_ARGS__); \
}
#else
#define print_info(msg, ...)
#endif

static const char *engine_id = "eng_secure_obj";
static const char *engine_name = "Secure Object OpenSSL Engine.";

static RSA_METHOD secureobj_rsa;

#define	MAX_SEC_OBJECTS	50

static int secure_obj_rsa_priv_enc(int flen, const unsigned char *from,
                         unsigned char *to, RSA *rsa, int padding)
{
	uint8_t *padded_from = NULL;
	uint16_t out_len = 0;
	int ret = 0, i = 0, j = 0;

	SK_RET_CODE sk_ret = SKR_OK;
	SK_MECHANISM_INFO mechType = {0};

	SK_ATTRIBUTE attrs[3];
	SK_OBJECT_HANDLE hObject = 0xFFFF, temp_hObject[MAX_SEC_OBJECTS];
	SK_OBJECT_TYPE obj_type;
	SK_KEY_TYPE key_type;
	uint32_t objCount, key_index;
	uint32_t rsa_key_len = 0;
	char *priv_exp = NULL, *modulus = NULL;
	uint32_t sobj_key_id[2] = { 0, 0 };

	memset(attrs, 0, 3 * sizeof(SK_ATTRIBUTE));
	memset(temp_hObject, 0, sizeof(SK_OBJECT_HANDLE) * MAX_SEC_OBJECTS);
	rsa_key_len = RSA_size(rsa);

	priv_exp = malloc(rsa_key_len);
	if (!priv_exp) {
		print_error("malloc failed for priv_exp_temp\n");
		ret = -1;
		goto failure;
	}

	modulus = malloc(rsa_key_len);
	if (!modulus) {
		print_error("malloc failed for modulus\n");
		ret = -1;
		goto failure;
	}

	BN_bn2bin(rsa->d, priv_exp);
	BN_bn2bin(rsa->n, modulus);

	for (j = 0; j<2; j++) {
		for (i = 5;i<9;i++) {
			sobj_key_id[j] |=priv_exp[rsa_key_len - i - (j * 4)] << 8 * (i - 5);
		}
	}

	if (!(((unsigned int)sobj_key_id[0] == (unsigned int)SOBJ_KEY_ID) &&
		((unsigned int)sobj_key_id[1] == (unsigned int)SOBJ_KEY_ID))) {
		print_info("Not a valid Secure Object Key, passing control to OpenSSL Function\n");
		ret = -2;
		goto failure;
	}

	key_index = priv_exp[rsa_key_len - 1];

	obj_type = SK_KEY_PAIR;
	key_type = SKK_RSA;

	mechType.mechanism = SKM_RSA_PKCS_NOPAD;

	attrs[0].type = SK_ATTR_OBJECT_TYPE;
	attrs[0].value = &obj_type;
	attrs[0].valueLen = sizeof(SK_OBJECT_TYPE);

	attrs[1].type = SK_ATTR_KEY_TYPE;
	attrs[1].value = &key_type;
	attrs[1].valueLen = sizeof(SK_KEY_TYPE);

	attrs[2].type = SK_ATTR_OBJECT_INDEX;
	attrs[2].value = (void *)&key_index;
	attrs[2].valueLen = sizeof(uint32_t);

	sk_ret = SK_EnumerateObjects(attrs, 3, temp_hObject, MAX_SEC_OBJECTS, &objCount);
	if (sk_ret != SKR_OK) {
		print_error("SK_EnumerateObjects failed with code = 0x%x\n", sk_ret);
		ret = -1;
		memset(attrs, 0, 3 * sizeof(SK_ATTRIBUTE));
		goto failure;
	}

	memset(attrs, 0, 3 * sizeof(SK_ATTRIBUTE));
	if (objCount == 0) {
		print_error("No object found\n");
		ret = -1;
		goto failure;
	}

	for (i = 0; i < objCount; i++) {
		memset(attrs, 0, 3 * sizeof(SK_ATTRIBUTE));
		attrs[0].type = SK_ATTR_MODULUS;
		attrs[0].value = NULL;
		attrs[0].valueLen = 0;

		sk_ret = SK_GetObjectAttribute(temp_hObject[i], attrs, 1);
		if (sk_ret != SKR_OK) {
			print_error("SK_GetObjectAttribute failed for object %u with code = 0x%x\n",
				temp_hObject[i], sk_ret);
			continue;
		}

		if ((int16_t)(attrs[0].valueLen) != -1) {
			attrs[0].value =
				(void *)malloc(attrs[0].valueLen);
			if (!attrs[0].value) {
				print_error("malloc failed ATTR[%d].Value\n", i);
				goto failure;
			}
		}

		sk_ret = SK_GetObjectAttribute(temp_hObject[i], attrs, 1);
		if (sk_ret != SKR_OK) {
			print_error("SK_GetObjectAttribute failed for object %u with code = 0x%x\n",
				temp_hObject[i], sk_ret);
			continue;
		}

		if (!memcmp(attrs[0].value, modulus, rsa_key_len)) {
			hObject = temp_hObject[i];
			break;
		}
	}

	if (hObject == 0xFFFF) {
		print_error("Key Correponding to pem passed is not present in HSM\n");
		ret = -1;
		goto failure;
	}

	out_len = rsa_key_len;

	padded_from = (uint8_t *)malloc(rsa_key_len);
	if (!padded_from) {
		print_error("padded_from malloc failed\n");
		ret = -1;
		goto failure;
	}

	switch (padding) {
		case RSA_PKCS1_PADDING:
			ret = RSA_padding_add_PKCS1_type_1(padded_from,
				rsa_key_len, from, flen);
			if (ret == 0) {
				print_error("RSA_padding_add_PKCS1_type_1 failed\n");
				ret = -1;
				goto failure;
			}
			break;
		default:
			print_error("Unsupported padding type, only RSA_PKCS1_PADDING is supported\n");
			ret  = -1;
			goto failure;
	}

	sk_ret = SK_Decrypt(&mechType, hObject, padded_from,
			rsa_key_len, to, &out_len);
	if (sk_ret != SKR_OK) {
		print_error("SK_Decrypt failed with ret code 0x%x\n", sk_ret);
		ret = -1;
		goto failure;
	}

	ret = rsa_key_len;

failure:
	if (padded_from)
		free(padded_from);
	if (modulus)
		free(modulus);
	if (priv_exp)
		free(priv_exp);

	for (i = 0; i < 3; i++) {
		if (attrs[i].value)
			free(attrs[i].value);
	}

	if (ret == -2) {
		const RSA_METHOD *rsa_meth = RSA_PKCS1_SSLeay();
		ret = rsa_meth->rsa_priv_enc(flen, from, to, rsa, padding);
	}

	return ret;
}

static int secure_obj_rsa_priv_dec(int flen, const unsigned char *from,
                         unsigned char *to, RSA *rsa, int padding)
{
	uint8_t *padded_to = NULL;
	uint16_t out_len = 0;
	int ret = 0, i = 0, j = 0;

	SK_RET_CODE sk_ret = SKR_OK;
	SK_MECHANISM_INFO mechType = {0};

	SK_ATTRIBUTE attrs[3];
	SK_OBJECT_HANDLE hObject = 0xFFFF, temp_hObject[MAX_SEC_OBJECTS];
	SK_OBJECT_TYPE obj_type;
	SK_KEY_TYPE key_type;
	uint32_t objCount, key_index;
	uint32_t rsa_key_len = 0;
	char *priv_exp = NULL, *modulus = NULL;
	uint32_t sobj_key_id[2] = { 0, 0 };

	memset(attrs, 0, 3 * sizeof(SK_ATTRIBUTE));
	memset(temp_hObject, 0, sizeof(SK_OBJECT_HANDLE) * MAX_SEC_OBJECTS);
	rsa_key_len = RSA_size(rsa);

	priv_exp = malloc(rsa_key_len);
	if (!priv_exp) {
		print_error("malloc failed for priv_exp_temp\n");
		ret = -1;
		goto failure;
	}

	modulus = malloc(rsa_key_len);
	if (!modulus) {
		print_error("malloc failed for modulus\n");
		ret = -1;
		goto failure;
	}

	BN_bn2bin(rsa->d, priv_exp);
	BN_bn2bin(rsa->n, modulus);

	for (j = 0; j < 2; j++) {
		for (i = 5; i < 9; i++) {
			sobj_key_id[j] |= priv_exp[rsa_key_len - i - (j * 4)] << 8 * (i - 5);
		}
	}

	if (!(((unsigned int)sobj_key_id[0] == (unsigned int)SOBJ_KEY_ID) &&
		((unsigned int)sobj_key_id[1] == (unsigned int)SOBJ_KEY_ID))) {
		print_info("Not a valid Secure Object Key, passing control to OpenSSL Function\n");
		ret = -2;
		goto failure;
	}

	key_index = priv_exp[rsa_key_len - 1];

	obj_type = SK_KEY_PAIR;
	key_type = SKK_RSA;

	print_info("byte_key_size = %d, flen = %d, padding = %d\n",
		rsa_key_len, flen, padding);

	attrs[0].type = SK_ATTR_OBJECT_TYPE;
	attrs[0].value = &obj_type;
	attrs[0].valueLen = sizeof(SK_OBJECT_TYPE);

	attrs[1].type = SK_ATTR_KEY_TYPE;
	attrs[1].value = &key_type;
	attrs[1].valueLen = sizeof(SK_KEY_TYPE);

	attrs[2].type = SK_ATTR_OBJECT_INDEX;
	attrs[2].value = (void *)&key_index;
	attrs[2].valueLen = sizeof(uint32_t);

	sk_ret = SK_EnumerateObjects(attrs, 3, temp_hObject, MAX_SEC_OBJECTS, &objCount);
	if (sk_ret != SKR_OK) {
		print_error("SK_EnumerateObjects failed with code = 0x%x\n", sk_ret);
		ret = -1;
		memset(attrs, 0, 3 * sizeof(SK_ATTRIBUTE));
		goto failure;
	}

	memset(attrs, 0, 3 * sizeof(SK_ATTRIBUTE));
	if (objCount == 0) {
		print_error("No object found\n");
		ret = -1;
		goto failure;
	}

	for (i = 0; i < objCount; i++) {
		memset(attrs, 0, 3 * sizeof(SK_ATTRIBUTE));
		attrs[0].type = SK_ATTR_MODULUS;
		attrs[0].value = NULL;
		attrs[0].valueLen = 0;

		sk_ret = SK_GetObjectAttribute(temp_hObject[i], attrs, 1);
		if (sk_ret != SKR_OK) {
			print_error("SK_GetObjectAttribute failed for object %u with code = 0x%x\n",
				temp_hObject[i], sk_ret);
			continue;
		}

		if ((int16_t)(attrs[0].valueLen) != -1) {
			attrs[0].value =
				(void *)malloc(attrs[0].valueLen);
			if (!attrs[0].value) {
				print_error("malloc failed ATTR[%d].Value\n", i);
				goto failure;
			}
		}

		sk_ret = SK_GetObjectAttribute(temp_hObject[i], attrs, 1);
		if (sk_ret != SKR_OK) {
			print_error("SK_GetObjectAttribute failed for object %u with code = 0x%x\n",
				temp_hObject[i], sk_ret);
			continue;
		}

		if (!memcmp(attrs[0].value, modulus, rsa_key_len)) {
			hObject = temp_hObject[i];
			break;
		}
	}

	if (hObject == 0xFFFF) {
		print_error("Key Correponding to pem passed is not present in HSM\n");
		ret = -1;
		goto failure;
	}

	padded_to = (uint8_t *)malloc(rsa_key_len);
	if (padded_to == NULL) {
		print_error("padded_to malloc  failed\n");
		ret = -1;
		goto failure;
	}

	mechType.mechanism = SKM_RSA_PKCS_NOPAD;

	out_len = rsa_key_len;

	sk_ret = SK_Decrypt(&mechType, hObject, from, flen,
			padded_to, &out_len);
	if (sk_ret != SKR_OK) {
		print_error("SK_Decrypt failed with ret code 0x%x\n", sk_ret);
		ret = -1;
		goto failure;
	}

	print_info("out_len = %u\n", out_len);

	switch (padding) {
		case RSA_PKCS1_PADDING:
			ret = RSA_padding_check_PKCS1_type_2(to,
				rsa_key_len, padded_to, out_len,
				rsa_key_len);
			if (ret == -1) {
				print_error("RSA_padding_check_PKCS1_type_2 failed\n");
				ret = -1;
				goto failure;
			}
			break;
		default:
			print_error("Unsupported padding type, only RSA_PKCS1_PADDING is supported\n");
			ret = -1;
			goto failure;
	}

failure:
	if (padded_to)
		free(padded_to);
	if (modulus)
		free(modulus);
	if (priv_exp)
		free(priv_exp);

	for (i = 0; i < 3; i++) {
		if (attrs[i].value)
			free(attrs[i].value);
	}

	if (ret == -2) {
		const RSA_METHOD *rsa_meth = RSA_PKCS1_SSLeay();
		ret = rsa_meth->rsa_priv_dec(flen, from, to, rsa, padding);
	}

	return ret;
}

static int bind(ENGINE *engine, const char *id)
{
	int ret = 0;

	if (!ENGINE_set_id(engine, engine_id) ||
		!ENGINE_set_name(engine, engine_name)) {
		print_error("ENGINE_set_id or ENGINE_set_name or ENGINE_set_init_function failed\n");
		goto end;
	}

	if (ENGINE_set_RSA(engine, &secureobj_rsa)) {
		const RSA_METHOD *rsa_meth = RSA_PKCS1_SSLeay();

		memset(&secureobj_rsa, 0, sizeof(RSA_METHOD));
		secureobj_rsa.name = "Secure Object RSA Engine";
		secureobj_rsa.rsa_pub_enc = rsa_meth->rsa_pub_enc;
		secureobj_rsa.rsa_pub_dec = rsa_meth->rsa_pub_dec;
		secureobj_rsa.rsa_priv_enc = secure_obj_rsa_priv_enc;
		secureobj_rsa.rsa_priv_dec = secure_obj_rsa_priv_dec;
		secureobj_rsa.rsa_mod_exp = rsa_meth->rsa_mod_exp;
		secureobj_rsa.bn_mod_exp = rsa_meth->bn_mod_exp;
		secureobj_rsa.init = NULL;
		secureobj_rsa.finish = NULL;
		secureobj_rsa.flags = 0;
		secureobj_rsa.app_data = NULL;
		secureobj_rsa.rsa_sign = NULL;
		secureobj_rsa.rsa_verify = NULL;
		secureobj_rsa.rsa_keygen = rsa_meth->rsa_keygen;
	} else {
		print_error("ENGINE_set_RSA failed\n");
		goto end;
	}

	if (!ENGINE_set_default_RSA(engine)) {
		print_error("ENGINE_set_default_RSA failed\n");
		goto end;
	}

	ret = 1;
end:
	return ret;
}

IMPLEMENT_DYNAMIC_BIND_FN(bind)
IMPLEMENT_DYNAMIC_CHECK_FN()
