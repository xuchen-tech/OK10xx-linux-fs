/*
 * Copyright 2017 NXP
 * SPDX-License-Identifier:     BSD-3-Clause
 */
#ifndef __UTILS_H__
#define __UTILS_H__

typedef struct {
	uint8_t *rsa_modulus;
	uint8_t *rsa_pub_exp;
	uint8_t *rsa_priv_exp;
	uint8_t *rsa_prime1;
	uint8_t *rsa_prime2;
	uint8_t *rsa_exp1;
	uint8_t *rsa_exp2;
	uint8_t *rsa_coeff;
} rsa_3form_key_t;

#define PERFORM 1
#define PARSE 2

#define MAX_RSA_ATTRIBUTES 13
#define MAX_FIND_OBJ_SIZE 5
#define U32_INVALID 0xFFFFFFFE
#define U32_UNINTZD 0xFFFFFFFF
#define APP_OK 0
#define APP_SKR_ERR -1
#define APP_MALLOC_FAIL -2
#define APP_PEM_READ_ERROR -3
#define APP_IP_ERR -4
#define APP_OPSSL_KEY_GEN_ERR -6

void printKey(uint8_t *key, uint32_t keyLen);

void printRSA_key(rsa_3form_key_t *rsa_3form_key, uint32_t key_len);

SK_OBJECT_TYPE getObjectType(char *objTypeStr);

SK_OBJECT_TYPE getKeyType(char *keyTypeStr);

SK_OBJECT_TYPE getMechType(char *mechTypeStr);

char *getKeyTypeStr(SK_KEY_TYPE key_type);

char *getObjTypeStr(SK_OBJECT_TYPE obj_type);

SK_OBJECT_TYPE getMechType(char *mechTypeStr);
SK_OBJECT_TYPE getMechTypeFrmObjKeyT(SK_OBJECT_TYPE obj_type,
		SK_KEY_TYPE key_type);
int validate_key_len(uint32_t key_len);
#endif /* __UTILS_H__ */
