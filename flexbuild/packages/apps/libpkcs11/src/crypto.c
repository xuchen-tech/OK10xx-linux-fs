/*
  * Copyright 2017 NXP
  * SPDX-License-Identifier:     BSD-3-Clause
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <cryptoki.h>
#include <crypto.h>
#include <general.h>
#include <objects.h>

#include <securekey_api.h>
#include <securekey_api_types.h>

/* Init for sign mechanism */
CK_RV sign_init(CK_SESSION_HANDLE hSession, sign_verify_context *ctx,
		CK_MECHANISM *mech, CK_BBOOL recover_mode,
		CK_OBJECT_HANDLE key)
{
	CK_ATTRIBUTE attr[4] = {0};
	CK_BYTE *ptr = NULL;
	CK_KEY_TYPE keytype;
	CK_OBJECT_CLASS class;
	CK_BBOOL sign = FALSE, found = FALSE;
	CK_MECHANISM_TYPE_PTR obj_mechanisms = NULL;
	CK_ULONG n;
	CK_RV rc;

	if (ctx->active == TRUE) {
		rc = CKR_OPERATION_ACTIVE;
		goto out;
	}

	/* Get all object attributes needed */
	attr[0].type = CKA_SIGN;
	attr[1].type = CKA_ALLOWED_MECHANISMS;
	attr[2].type = CKA_KEY_TYPE;
	attr[3].type = CKA_CLASS;
	/* Check if key supports sign attribute */
	rc = get_attr_value(hSession, key, attr, 1);
	if (rc != CKR_OK) {
		rc = CKR_KEY_FUNCTION_NOT_PERMITTED;
		goto out;
	}
	rc = get_attr_value(hSession, key, &attr[1], 3);
	if (rc != CKR_OK)
		goto out;
	obj_mechanisms =
		(CK_MECHANISM_TYPE_PTR)malloc(attr[1].ulValueLen);
	if (!obj_mechanisms) {
		rc = CKR_HOST_MEMORY;
		goto out;
	}
	attr[0].pValue = &sign;
	attr[1].pValue = obj_mechanisms;
	attr[2].pValue = &keytype;
	attr[3].pValue = &class;
	rc = get_attr_value(hSession, key, attr, 4);
	if (rc != CKR_OK)
		goto out;

	/* Check if object can support sign mechanism */
	if (sign != TRUE) {
		rc = CKR_KEY_FUNCTION_NOT_PERMITTED;
		goto out;
	}

	/* Check if object can support sign mechanism type */
	for (n = 0; n < (attr[1].ulValueLen/sizeof(CK_MECHANISM_TYPE)); n++) {
		if (mech->mechanism ==
			*((CK_MECHANISM_TYPE_PTR)attr[1].pValue + n)) {
			found = TRUE;
			break;
		}
	}
	if (found != TRUE) {
		rc = CKR_MECHANISM_INVALID;
		goto out;
	}

	/* Check for key attributes if they match with mechanism provided */
	switch (mech->mechanism) {
	case CKM_RSA_PKCS:
	case CKM_MD5_RSA_PKCS:
	case CKM_SHA1_RSA_PKCS:
	case CKM_SHA256_RSA_PKCS:
	case CKM_SHA384_RSA_PKCS:
	case CKM_SHA512_RSA_PKCS:
		if (mech->ulParameterLen != 0) {
			rc = CKR_MECHANISM_PARAM_INVALID;
			goto out;
		}
		/* Key type must be RSA */
		if (keytype != CKK_RSA) {
			rc = CKR_KEY_TYPE_INCONSISTENT;
			goto out;
		}
		/* Key class must be Private */
		if (class != CKO_PRIVATE_KEY) {
			rc = CKR_KEY_FUNCTION_NOT_PERMITTED;
			goto out;
		}
		/* Currently we don't support multi-part RSA ops */
		ctx->context_len = 0;
		ctx->context     = NULL;
		break;

	default:
		rc = CKR_MECHANISM_INVALID;
		goto out;
	}

	if (mech->ulParameterLen > 0) {
		ptr = (CK_BYTE *)malloc(mech->ulParameterLen);
		if (!ptr) {
			rc = CKR_HOST_MEMORY;
			goto out;
		}
		memcpy(ptr, mech->pParameter, mech->ulParameterLen);
	}

	/* Keeping the sign information in session ctx */
	ctx->key                 = key;
	ctx->mech.ulParameterLen = mech->ulParameterLen;
	ctx->mech.mechanism      = mech->mechanism;
	ctx->mech.pParameter     = ptr;
	ctx->multi               = FALSE;
	ctx->active              = TRUE;
	ctx->recover             = recover_mode;

out:
	if (obj_mechanisms)
		free(obj_mechanisms);

	return rc;
}

/* Implementation of raw sign api */
static CK_RV rsa_sign_pkcs(CK_SESSION_HANDLE hSession, session *sess,
			   CK_BYTE_PTR pData, CK_ULONG ulDataLen,
			   CK_BYTE_PTR pSignature,
			   CK_ULONG_PTR pulSignatureLen)
{
	CK_RV rc = CKR_OK;
	sign_verify_context *ctx = &sess->sign_ctx;
	CK_ATTRIBUTE attr = {0};
	CK_ULONG req_sig_len = 0, padding_len, i;
	CK_BYTE data[MAX_RSA_KEYLEN];
	SK_FUNCTION_LIST_PTR sk_funcs = NULL;
	SK_RET_CODE ret = SKR_OK;
	SK_MECHANISM_INFO mechType = {0};
	SK_OBJECT_HANDLE sk_key;

	/* Get required signature buffer size from size of modulus */
	attr.type = CKA_MODULUS;
	rc = get_attr_value(hSession, ctx->key, &attr, 1);
	if (rc != CKR_OK)
		goto out;
	req_sig_len = attr.ulValueLen;

	/*
	 * If signature buffer is NULL then return size of
	 * buffer to be allocated.
	 */
	if (!pSignature) {
		*pulSignatureLen = req_sig_len;
		rc = CKR_OK;
		goto out;
	}

	/* Signature length should not be less than required size */
	if (*pulSignatureLen < req_sig_len) {
		rc = CKR_BUFFER_TOO_SMALL;
		goto out;
	}

	/*
	 * Check if input data length > (key_length - 11), if yes
	 * return error as PKCS 1.5 Block type = 01 (used in RSA signature
	 * scheme) requires input data to be less  than or equal to
	 * key_length - 11.
	 */
	if (ulDataLen > (req_sig_len - 11)) {
		rc = CKR_DATA_LEN_RANGE;
		goto out;
	}

	/* The padding string PS shall consist of k-3-||D|| octets. */
	padding_len = req_sig_len - 3 - ulDataLen;

	/*
	 * For block type 01, PS shall have value FF.
	 * EB = 00 || 01 || PS * i || 00 || D
	 */
	data[0] = (CK_BYTE)0;
	data[1] = (CK_BYTE)RSA_PKCS_BT_1;
	for (i = 2; i < (padding_len + 2); i++)
		data[i] = (CK_BYTE)0xff;
	data[i] = (CK_BYTE)0;
	i++;
	memcpy(&data[i], pData, ulDataLen);

	/* Maps RSA sign --> SK_decrypt for private key operation */
	sk_funcs = get_slot_function_list(sess->session_info.slotID);
	if (!sk_funcs)
		return CKR_ARGUMENTS_BAD;

	mechType.mechanism = SKM_RSA_PKCS_NOPAD;
	sk_key = ((struct object_node *)ctx->key)->object.sk_obj_handle;

	ret = sk_funcs->SK_Decrypt(&mechType, sk_key, data, req_sig_len,
				   pSignature, (uint16_t *)pulSignatureLen);
	if (ret != SKR_OK) {
		print_error("SK_Decrypt failed with ret code 0x%x\n", ret);
		rc = CKR_GENERAL_ERROR;
		goto out;
	}

out:
	return rc;
}

/* Implementation of hash based sign api */
static CK_RV rsa_hash_sign_pkcs(CK_SESSION_HANDLE hSession, session *sess,
				CK_BYTE_PTR pData, CK_ULONG ulDataLen,
				CK_BYTE_PTR pSignature,
				CK_ULONG_PTR pulSignatureLen)
{
	CK_RV rc = CKR_OK;
	sign_verify_context *ctx = &sess->sign_ctx;
	CK_ATTRIBUTE attr = {0};
	CK_ULONG req_sig_len = 0;
	CK_BYTE hash[MAX_HASH_LEN];
	CK_ULONG hash_len = MAX_HASH_LEN;
	SK_FUNCTION_LIST_PTR sk_funcs = NULL;
	SK_RET_CODE ret = SKR_OK;
	SK_MECHANISM_INFO signType = {0}, digestType = {0};
	SK_OBJECT_HANDLE sk_key;

	/* Get required signature buffer size from size of modulus */
	attr.type = CKA_MODULUS;
	rc = get_attr_value(hSession, ctx->key, &attr, 1);
	if (rc != CKR_OK)
		goto out;
	req_sig_len = attr.ulValueLen;

	/*
	 * If signature buffer is NULL then return size of
	 * buffer to be allocated.
	 */
	if (!pSignature) {
		*pulSignatureLen = req_sig_len;
		rc = CKR_OK;
		goto out;
	}

	/* Signature length should not be less than required size */
	if (*pulSignatureLen < req_sig_len) {
		rc = CKR_BUFFER_TOO_SMALL;
		goto out;
	}

	/* Maps RSA hash based sign --> SK_Digest and SK_Sign */
	sk_funcs = get_slot_function_list(sess->session_info.slotID);
	if (!sk_funcs)
		return CKR_ARGUMENTS_BAD;

	switch (ctx->mech.mechanism) {
	case CKM_MD5_RSA_PKCS:
		signType.mechanism = SKM_RSASSA_PKCS1_V1_5_MD5;
		digestType.mechanism = SKM_MD5;
		break;
	case CKM_SHA1_RSA_PKCS:
		signType.mechanism = SKM_RSASSA_PKCS1_V1_5_SHA1;
		digestType.mechanism = SKM_SHA1;
		break;
	case CKM_SHA256_RSA_PKCS:
		signType.mechanism = SKM_RSASSA_PKCS1_V1_5_SHA256;
		digestType.mechanism = SKM_SHA256;
		break;
	case CKM_SHA384_RSA_PKCS:
		signType.mechanism = SKM_RSASSA_PKCS1_V1_5_SHA384;
		digestType.mechanism = SKM_SHA384;
		break;
	case CKM_SHA512_RSA_PKCS:
		signType.mechanism = SKM_RSASSA_PKCS1_V1_5_SHA512;
		digestType.mechanism = SKM_SHA512;
		break;
	default:
		rc = CKR_MECHANISM_INVALID;
		goto out;
	}

	sk_key = ((struct object_node *)ctx->key)->object.sk_obj_handle;

	ret = sk_funcs->SK_Digest(&digestType, pData, ulDataLen, hash,
				  (uint16_t *)&hash_len);
	if (ret != SKR_OK) {
		print_error("SK_Digest failed with ret code 0x%x\n", ret);
		rc = CKR_GENERAL_ERROR;
		goto out;
	}

	ret = sk_funcs->SK_Sign(&signType, sk_key, hash, hash_len,
				pSignature, (uint16_t *)pulSignatureLen);
	if (ret != SKR_OK) {
		print_error("SK_Sign failed with ret code 0x%x\n", ret);
		rc = CKR_GENERAL_ERROR;
		goto out;
	}

out:
	return rc;
}

/* Implementation of sign api */
CK_RV sign(CK_SESSION_HANDLE hSession, session *sess, CK_BYTE_PTR pData,
	   CK_ULONG ulDataLen, CK_BYTE_PTR pSignature,
	   CK_ULONG_PTR pulSignatureLen)
{
	sign_verify_context *ctx = &sess->sign_ctx;
	CK_RV rc = CKR_OK;

	if (ctx->active == FALSE) {
		rc = CKR_OPERATION_NOT_INITIALIZED;
		goto out;
	}

	switch (ctx->mech.mechanism) {
	case CKM_RSA_PKCS:
		rc = rsa_sign_pkcs(hSession, sess, pData, ulDataLen,
				     pSignature, pulSignatureLen);
		if (((rc == CKR_OK) && (pSignature == NULL)) ||
			(rc == CKR_BUFFER_TOO_SMALL))
			goto out;
		break;

	case CKM_MD5_RSA_PKCS:
	case CKM_SHA1_RSA_PKCS:
	case CKM_SHA256_RSA_PKCS:
	case CKM_SHA384_RSA_PKCS:
	case CKM_SHA512_RSA_PKCS:
		rc = rsa_hash_sign_pkcs(hSession, sess, pData, ulDataLen,
					  pSignature, pulSignatureLen);
		if (((rc == CKR_OK) && (pSignature == NULL)) ||
			(rc == CKR_BUFFER_TOO_SMALL))
			goto out;
		break;
	default:
		rc = CKR_MECHANISM_INVALID;
		goto out;
	}

	ctx->key = 0;
	ctx->mech.ulParameterLen = 0;
	ctx->mech.mechanism = 0;
	ctx->multi = FALSE;
	ctx->active = FALSE;
	ctx->recover = FALSE;
	ctx->context_len = 0;

	if (ctx->mech.pParameter) {
		free( ctx->mech.pParameter );
		ctx->mech.pParameter = NULL;
	}

	if (ctx->context) {
		free( ctx->context );
		ctx->context = NULL;
	}

out:
	return rc;
}
