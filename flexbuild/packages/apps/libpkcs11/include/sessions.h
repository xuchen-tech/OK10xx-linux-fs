/*
  * Copyright 2017 NXP
  * SPDX-License-Identifier:     BSD-3-Clause
*/

#ifndef ___SESSIONS_H_INC___
#define ___SESSIONS_H_INC___

#include <objects.h>

typedef struct _sign_verify_context {
	CK_OBJECT_HANDLE	key; /* Key handle */
	CK_MECHANISM		mech; /* current sign mechanism */
	CK_BYTE			*context; /* temporary work area */
	CK_ULONG		context_len;
	CK_BBOOL		multi;	/* is this a multi-part operation? */
	CK_BBOOL		recover; /* are we in recover mode? */
	CK_BBOOL		active; /* Is Sign already initialized? */
} sign_verify_context;

typedef struct _session {
	/* Information about this session */
	CK_SESSION_INFO		session_info;
	/* Objects matching the template during Find operation are kept
	  * in this find_list */
	CK_OBJECT_HANDLE_PTR	find_list;
	/* Number of objects in find_list */
	CK_ULONG		find_count;
	/* Current position in find_list */
	CK_ULONG		find_idx;
	/* op_active will be set when any operation find/crypto is in
	  * progress */
	CK_BBOOL		op_active;
	/* Sign context info per session */
	sign_verify_context sign_ctx;
} session;

/* The number of supported slots */
#define SLOT_COUNT 1

struct session_node {
	session sess;
	STAILQ_ENTRY(session_node) entry;
};

STAILQ_HEAD(session_list, session_node);

CK_RV initialize_session_list(CK_SLOT_ID slotID);

CK_RV destroy_session_list(CK_SLOT_ID slotID);

struct session_list *get_session_list(CK_SLOT_ID slotID);

session *get_session(CK_SESSION_HANDLE hSession);

CK_BBOOL is_session_valid(CK_SESSION_HANDLE hSession);

CK_RV create_session(CK_SLOT_ID slotID,  CK_FLAGS flags,
		CK_SESSION_HANDLE_PTR phSession);

CK_RV delete_session(CK_SESSION_HANDLE hSession);

CK_RV get_session_info(CK_SESSION_HANDLE hSession,
		CK_SESSION_INFO_PTR pInfo);

struct slot_info *get_global_slot_info(CK_SLOT_ID slotID);

struct SK_FUNCTION_LIST *get_slot_function_list(CK_SLOT_ID slotID);

#endif
