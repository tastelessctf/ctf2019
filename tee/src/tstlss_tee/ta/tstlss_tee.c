/*
 * Copyright (c) 2016, Linaro Limited
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#include <stdlib.h>
#include <string.h>

#include <tee_internal_api.h>
#include <tee_internal_api_extensions.h>
#include <utee_syscalls.h>

#include <tstlss_tee.h>

#define RELEASE 0
#define FLAG_SIZE 32


static int flagged = 0;

// Thx stackoverflow
static unsigned long atoi(const char *s) {
  unsigned long n = 0;
  while ( (*s>= '0') && (*s <= '9')) n = 10 * n + *s++ - '0';
  return n;
}


/*
 * Called when the instance of the TA is created. This is the first call in
 * the TA.
 */
TEE_Result TA_CreateEntryPoint(void)
{
    return TEE_SUCCESS;
}

/*
 * Called when the instance of the TA is destroyed if the TA has not
 * crashed or panicked. This is the last call in the TA.
 */
void TA_DestroyEntryPoint(void)
{
    TEE_Result res;
    TEE_ObjectHandle object;

    if(flagged) {
        res = TEE_OpenPersistentObject(TEE_STORAGE_PRIVATE,
                        "flag", 4,
                        TEE_DATA_FLAG_ACCESS_READ |
                        TEE_DATA_FLAG_ACCESS_WRITE_META, 
                        &object);
        if (res != TEE_SUCCESS) {
            EMSG("Failed to open flag for destroying :O res=0x%08x", res);
            return;
        }
        TEE_CloseAndDeletePersistentObject1(object);
        DMSG("RM'd the flag");
    }
}

/*
 * Called when a new session is opened to the TA. *sess_ctx can be updated
 * with a value to be able to identify this session in subsequent calls to the
 * TA. In this function you will normally do the global initialization for the
 * TA.
 */
TEE_Result TA_OpenSessionEntryPoint(uint32_t param_types,
		TEE_Param __maybe_unused params[4],
		void __maybe_unused **sess_ctx)
{
	uint32_t exp_param_types = TEE_PARAM_TYPES(TEE_PARAM_TYPE_NONE,
						   TEE_PARAM_TYPE_NONE,
						   TEE_PARAM_TYPE_NONE,
						   TEE_PARAM_TYPE_NONE);

	DMSG("has been called");

	if (param_types != exp_param_types)
		return TEE_ERROR_BAD_PARAMETERS;

	/* Unused parameters */
	(void)&params;
	(void)&sess_ctx;

	/* If return value != TEE_SUCCESS the session will not be created. */
	return TEE_SUCCESS;
}

/*
 * Called when a session is closed, sess_ctx hold the value that was
 * assigned by TA_OpenSessionEntryPoint().
 */
void TA_CloseSessionEntryPoint(void __maybe_unused *sess_ctx)
{
	(void)&sess_ctx; /* Unused parameter */
    DMSG("called");
}

static __attribute__((noinline)) TEE_Result put_flag(uint32_t param_types, TEE_Param params[4])
{
	const uint32_t exp_param_types =
		TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_INPUT,
				TEE_PARAM_TYPE_NONE,
				TEE_PARAM_TYPE_NONE,
				TEE_PARAM_TYPE_NONE);
	TEE_ObjectHandle object;
	TEE_Result res;
	char *data;
	size_t data_sz;
    char flag_str[64];

    //if(flagged){
        //EMSG("Flag already registered, what are you doing?!?!");
        //return TEE_ERROR_ACCESS_DENIED;
    //}
	/*
	 * Safely get the invocation parameters
	 */
    if (param_types != exp_param_types)
        return TEE_ERROR_BAD_PARAMETERS;

	data = (char *)params[0].memref.buffer;
	data_sz = params[0].memref.size;

    TEE_MemMove(flag_str, params[0].memref.buffer, params[0].memref.size);

	res = TEE_CreatePersistentObject(TEE_STORAGE_PRIVATE,
					"flag", 4,
					TEE_DATA_FLAG_ACCESS_READ | TEE_DATA_FLAG_ACCESS_WRITE_META,
					TEE_HANDLE_NULL,
					data, data_sz,		
					&object);
	if (res != TEE_SUCCESS) {
		EMSG("Placing flag in TEE failed 0x%08x", res);
	}
    else {
        DMSG("Successfully placed flag in TEE");
    }

    memset(flag_str, 0, 64); // better safe than sorry
    TEE_CloseObject(object);

    flagged = 1;
	return res;
}

static __attribute__((noinline)) TEE_Result read_flag(uint32_t param_types, TEE_Param params[4])
{
	const uint32_t exp_param_types =
		TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_OUTPUT,
				TEE_PARAM_TYPE_VALUE_INPUT,
				TEE_PARAM_TYPE_VALUE_INPUT,
				TEE_PARAM_TYPE_NONE);
	TEE_ObjectHandle object;
    TEE_ObjectInfo object_info;
	TEE_Result res;
    unsigned int data_sz;
	char *data;
    uint32_t read_bytes;

    IMSG("WTF");
	if (param_types != exp_param_types)
		return TEE_ERROR_BAD_PARAMETERS;
    IMSG("WTF2");

	data = (char *)params[0].memref.buffer;
	data_sz = params[0].memref.size;

#if RELEASE == 1
    const char *passive_agressive_string = "in your dreams";
    if (data_sz < strlen(passive_agressive_string)){
            return TEE_ERROR_SHORT_BUFFER;
    }

    strcpy(data, passive_agressive_string);       
	params[0].memref.size = strlen(passive_agressive_string);

#else

    if (params[1].value.a != 0xf1a6f1a6 || 
        params[1].value.b != 0xdeadbeef ||
        params[2].value.a != 0x7a57e1e5 )
    {
    IMSG("KEYINCORR: %0x %0x %0x", params[1].value.a, params[1].value.b, params[2].value.b);
    
        const char *passive_agressive_string = "in your dreams";
        if (data_sz < strlen(passive_agressive_string)){
                return TEE_ERROR_SHORT_BUFFER;
        }
        strcpy(data, passive_agressive_string);       
        params[0].memref.size = strlen(passive_agressive_string);
        return TEE_ERROR_SECURITY;
    }

    IMSG("KEYCORR");




	res = TEE_OpenPersistentObject(TEE_STORAGE_PRIVATE,
					"flag", 4,
					TEE_DATA_FLAG_ACCESS_READ,
					&object);
	if (res != TEE_SUCCESS) {
		EMSG("Failed to open flag, res=0x%08x", res);
		return res;
	}

    IMSG("KEYCORR1");
	res = TEE_GetObjectInfo1(object, &object_info);
	if (res != TEE_SUCCESS) {
		EMSG("Failed to get flag metadata, res=0x%08x", res);
        TEE_CloseObject(object);
        return res;
	}
    IMSG("KEYCORR2");
	res = TEE_ReadObjectData(object, data, object_info.dataSize,
				 &read_bytes);
	if (res != TEE_SUCCESS || read_bytes != object_info.dataSize) {
		EMSG("TEE_Read FlagData failed 0x%08x, read %" PRIu32 " over %u",
				res, read_bytes, object_info.dataSize);
	}

    IMSG("KEYCORR3");
	params[0].memref.size = read_bytes;
    TEE_CloseObject(object);
#endif

    return res;
}


static __attribute__((noinline)) unsigned int get_steep_time(char *obj_id) 
{
	TEE_ObjectHandle object;
    TEE_Result res;
    char data[4];
	uint32_t read_bytes;

    res = TEE_OpenPersistentObject(TEE_STORAGE_PRIVATE,
					obj_id, strlen(obj_id),
					TEE_DATA_FLAG_ACCESS_READ,
					&object);
	if (res != TEE_SUCCESS) {
        EMSG("Couldn't open %s", obj_id);
        return 0;
    }

    res = TEE_ReadObjectData(object, data, 4, &read_bytes);
	if (res != TEE_SUCCESS || read_bytes != 4) {
        EMSG("Couldn't read steep time.");
        TEE_CloseObject(object);
        return 0;
	}

    IMSG("Steep time data: %s", data); 
    IMSG("Steep time data: %d %d %d %d", data[0], data[1], data[2], data[3]); 
    IMSG("Got steep time: %d", atoi(data));

    TEE_CloseObject(object);
    return atoi(data);
}

/* returns NULL on error */
static __attribute__((noinline)) unsigned int get_desc_len(char *obj_id)
{
	TEE_ObjectHandle object;
	TEE_ObjectInfo object_info;
    TEE_Result res;

    res = TEE_OpenPersistentObject(TEE_STORAGE_PRIVATE,
					obj_id, strlen(obj_id),
					TEE_DATA_FLAG_ACCESS_READ,
					&object);
	if (res != TEE_SUCCESS) {
        EMSG("Couldn't open %s", obj_id);
        return 0;
    }

    res = TEE_GetObjectInfo1(object, &object_info);
	if (res != TEE_SUCCESS) {
		EMSG("Uhm. THAT tea is weird: 0x%x", res);
        TEE_CloseObject(object);
        return 0;
	}

    IMSG("Got desc_len: %d", object_info.dataSize-4);

    TEE_CloseObject(object);
    return object_info.dataSize - 4;
}

/* returns NULL on error */
static __attribute__((noinline)) char * get_description(char *obj_id)
{
	TEE_ObjectHandle object;
    TEE_Result res;
	uint32_t read_bytes;
    unsigned int size;
    char *data;

    size = get_desc_len(obj_id);
    data = TEE_Malloc(size, 0);
    if(!data) return TEE_ERROR_OUT_OF_MEMORY;

    res = TEE_OpenPersistentObject(TEE_STORAGE_PRIVATE,
					obj_id, strlen(obj_id),
					TEE_DATA_FLAG_ACCESS_READ,
					&object);
	if (res != TEE_SUCCESS) {
        EMSG("Couldn't open %s", obj_id);
        free(data);
        return NULL;
    }

    res = TEE_SeekObjectData(object, 4, TEE_DATA_SEEK_SET);
	if (res != TEE_SUCCESS) {
		EMSG("Tea seek failed 0x%08x", res);
        free(data);
        TEE_CloseObject(object);
        return NULL;
    }



    res = TEE_ReadObjectData(object, data, size, &read_bytes);
	if (res != TEE_SUCCESS || read_bytes != size) {
		EMSG("Reading description failed 0x%08x, read %d over %u",
				res, read_bytes, size);
        free(data);
        data = NULL;
    }

    IMSG("Got desc: %s", data);

    TEE_CloseObject(object);
    return data;
}


static __attribute__((noinline)) TEE_Result set_description(char *obj_id,
        char *data, unsigned int size)
{
	TEE_ObjectHandle object;
    TEE_Result res;

    res = TEE_OpenPersistentObject(TEE_STORAGE_PRIVATE,
					obj_id, strlen(obj_id),
					TEE_DATA_FLAG_ACCESS_READ | TEE_DATA_FLAG_ACCESS_WRITE,
					&object);
	if (res != TEE_SUCCESS) {
        EMSG("Couldn't open %s", obj_id);
        return res;
    }

    res = TEE_TruncateObjectData(object, size+4);
	if (res != TEE_SUCCESS) {
		EMSG("Tea truncate failed 0x%08x", res);
        TEE_CloseObject(object);
        return res;
    }

    res = TEE_SeekObjectData(object, 4, TEE_DATA_SEEK_SET);
	if (res != TEE_SUCCESS) {
		EMSG("Tea seek failed 0x%08x", res);
        TEE_CloseObject(object);
        return res;
    }

	res = TEE_WriteObjectData(object, data, size);
	if (res != TEE_SUCCESS) {
		EMSG("Tea modification failed 0x%08x", res);
    }

    IMSG("Set desc to %s", data);

    TEE_CloseObject(object);
    return TEE_SUCCESS;
}


static __attribute__((noinline)) TEE_Result set_steep_time(char *obj_id, unsigned int steep_time)
{
	TEE_ObjectHandle object;
    TEE_Result res;
    char data[5] = {0};



    res = TEE_OpenPersistentObject(TEE_STORAGE_PRIVATE,
					obj_id, strlen(obj_id),
					TEE_DATA_FLAG_ACCESS_READ | TEE_DATA_FLAG_ACCESS_WRITE,
					&object);
	if (res != TEE_SUCCESS) {
        EMSG("Couldn't open %s", obj_id);
        return res;
    }


    res = TEE_SeekObjectData(object, 0, TEE_DATA_SEEK_SET);
	if (res != TEE_SUCCESS) {
		EMSG("Tea seek failed 0x%08x", res);
        TEE_CloseObject(object);
        return res;
    }

    snprintf(data, 5, "%d", steep_time);
    if (data[3] != 0){
        TEE_CloseObject(object);
        return TEE_ERROR_TIME_NOT_SET;
    }


	res = TEE_WriteObjectData(object, data, 4);
	if (res != TEE_SUCCESS) {
		EMSG("Tea modification failed 0x%08x", res);
    }


    IMSG("Set steep time to %s", data);

    TEE_CloseObject(object);
    return TEE_SUCCESS;
}



static __attribute__((noinline))  TEE_Result add_tea(uint32_t param_types,
	TEE_Param params[4])
{
	uint32_t exp_param_types = TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_INPUT, //name
						   TEE_PARAM_TYPE_MEMREF_INPUT, //desc
						   TEE_PARAM_TYPE_VALUE_INPUT, //steep time
						   TEE_PARAM_TYPE_NONE);

  	TEE_ObjectHandle object;
	TEE_Result res;
	char *obj_id;
	size_t obj_id_sz;
	char *data;
	size_t data_sz;
	uint32_t obj_data_flag;

	DMSG("has been called");

	if (param_types != exp_param_types)
		return TEE_ERROR_BAD_PARAMETERS;

 
    if (params[2].value.a > 999) {
		EMSG("Too damn high steep time - you don't know how to make tea, eh?");
        return TEE_ERROR_TIME_NOT_SET;
    }
    
    if (params[2].value.a == 0) {
		EMSG("You really just want hot water? I don't think so");
        return TEE_ERROR_TIME_NOT_SET;
    }
    
	DMSG("has been called");

    // Setup ID
    obj_id_sz = strlen(params[0].memref.buffer);
	obj_id = TEE_Malloc(obj_id_sz, 0);
	if (!obj_id) return TEE_ERROR_OUT_OF_MEMORY;
    TEE_MemMove(obj_id, params[0].memref.buffer, obj_id_sz);

    // Setup data
    data_sz = params[1].memref.size + 4;
    data =  TEE_Malloc(data_sz, 0);

    if (!data) {
        free(obj_id);
        return TEE_ERROR_OUT_OF_MEMORY;
    } 


    snprintf(data, 4, "%d", params[2].value.a);
    TEE_MemMove(data+4, params[1].memref.buffer, params[1].memref.size);

    IMSG("Received steep time: %d", params[2].value.a);
    IMSG("Parsed steep time: %s", data);
    IMSG("Parsed steep time: %d %d %d %d %d", data[0], data[1], data[2], data[3], data[4]);
    // create and store
    obj_data_flag = TEE_DATA_FLAG_ACCESS_READ | TEE_DATA_FLAG_ACCESS_WRITE |
                    TEE_DATA_FLAG_ACCESS_WRITE_META;

	res = TEE_CreatePersistentObject(TEE_STORAGE_PRIVATE,
					obj_id, obj_id_sz,
					obj_data_flag,
					TEE_HANDLE_NULL,
					data, data_sz,
					&object);
	if (res != TEE_SUCCESS) {
		EMSG("Storing tea failed 0x%08x", res);
	}

    TEE_CloseObject(object);
    TEE_Free(obj_id);
    TEE_Free(data);
    return res;
}


static __attribute__((noinline)) TEE_Result del_tea(uint32_t param_types,
	TEE_Param params[4])
{
	uint32_t exp_param_types = TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_INPUT, //name
						   TEE_PARAM_TYPE_NONE,
						   TEE_PARAM_TYPE_NONE,
						   TEE_PARAM_TYPE_NONE);

    TEE_ObjectHandle object;
	TEE_Result res;
	char *obj_id;
	size_t obj_id_sz;


	DMSG("has been called");

	if (param_types != exp_param_types)
		return TEE_ERROR_BAD_PARAMETERS;

	obj_id_sz = strlen(params[0].memref.buffer);
	obj_id = TEE_Malloc(obj_id_sz, 0);
	if (!obj_id)
		return TEE_ERROR_OUT_OF_MEMORY;

	TEE_MemMove(obj_id, params[0].memref.buffer, obj_id_sz);

	res = TEE_OpenPersistentObject(TEE_STORAGE_PRIVATE,
					obj_id, obj_id_sz,
					TEE_DATA_FLAG_ACCESS_READ | TEE_DATA_FLAG_ACCESS_WRITE_META, 
					&object);

	if (res == TEE_SUCCESS) 
        TEE_CloseAndDeletePersistentObject1(object);
    else 
		EMSG("Failed to delete tea: 0x%08x", res);
	
    TEE_Free(obj_id);
	return res;
}


static __attribute__((noinline)) TEE_Result mfy_tea(uint32_t param_types,
	TEE_Param params[4])
{
	uint32_t exp_param_types = TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_INPUT, //name
						   TEE_PARAM_TYPE_MEMREF_INPUT, //desc
						   TEE_PARAM_TYPE_VALUE_INPUT, //steep time
						   TEE_PARAM_TYPE_NONE);


	TEE_Result res;
	char *obj_id;
	size_t obj_id_sz;
	char *data;
	size_t data_sz;
    //unsigned int steep_time;


	DMSG("has been called");

	if (param_types != exp_param_types)
		return TEE_ERROR_BAD_PARAMETERS;

    obj_id_sz = strlen(params[0].memref.buffer);
	obj_id = TEE_Malloc(obj_id_sz, 0);
	TEE_MemMove(obj_id, params[0].memref.buffer, obj_id_sz);

    DMSG("Object: %s", obj_id);

    // Read old data
    //data = get_description(obj_id, obj_id_sz);
    //steep_time = get_steep_time(obj_id, obj_id_sz);

    if (params[2].value.a != 0) {

        IMSG("Updating steep name");
        //if (params[2].value.a > 999) {
            //EMSG("You should know better by now");
            //res = TEE_ERROR_TIME_NOT_SET;
            //goto end;
        //}
        res = set_steep_time(obj_id, params[2].value.a);
        if (res != TEE_SUCCESS) goto end;
    }  

    // Update desc, if necessarry
    if (strncmp(params[1].memref.buffer, "NOPE", 4)) {
        IMSG("Updating name %s", (void * )params[1].memref.buffer);
        res = set_description(obj_id, params[1].memref.buffer, params[1].memref.size);

    }  
    // Write updated buffer back
end:
    free(data);
    free(obj_id);
	return res;
}



static __attribute__((noinline)) TEE_Result get_tea(uint32_t param_types,
	TEE_Param params[4])
{
	uint32_t exp_param_types = TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_INPUT, //name
						   TEE_PARAM_TYPE_MEMREF_OUTPUT, //desc
						   TEE_PARAM_TYPE_VALUE_OUTPUT, //steep time 
						   TEE_PARAM_TYPE_NONE);
	char *obj_id;
	size_t obj_id_sz;
    unsigned int steep_time;
	char *data;
	size_t data_sz;

	DMSG("has been called");

	if (param_types != exp_param_types)
		return TEE_ERROR_BAD_PARAMETERS;

    obj_id_sz = strlen(params[0].memref.buffer);
	obj_id = TEE_Malloc(obj_id_sz, 0);

    if (!obj_id) return TEE_ERROR_OUT_OF_MEMORY;
	TEE_MemMove(obj_id, params[0].memref.buffer, obj_id_sz);


    DMSG("Object: %s", obj_id);


    steep_time = get_steep_time(obj_id);
    DMSG("steep time: %d", steep_time);
    if (steep_time == 0) {
        free(obj_id);
        return TEE_ERROR_TIME_NOT_SET;
    }

    data = get_description(obj_id);
    if (data == NULL) {
        free(obj_id);
        return TEE_ERROR_NO_DATA;
    }

    DMSG("DESC: %s", data);

    data_sz = get_desc_len(obj_id);
    if(data_sz == 0) {
        free(obj_id);
        free(data);
        return TEE_ERROR_NO_DATA;
    }

	TEE_MemMove(params[1].memref.buffer, data, data_sz);
    params[2].value.a = steep_time;

    free(obj_id);
    free(data);
	return TEE_SUCCESS;
}

static __attribute__((noinline)) TEE_Result brw_tea(uint32_t param_types,
	TEE_Param params[4])
{
	uint32_t exp_param_types = TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_INPUT, //name
						   TEE_PARAM_TYPE_MEMREF_OUTPUT, //tea
						   TEE_PARAM_TYPE_NONE, 
						   TEE_PARAM_TYPE_NONE);

	DMSG("has been called");
 
    TEE_ObjectHandle object;
	TEE_ObjectInfo object_info;
	TEE_Result res;
	char *obj_id;
	size_t obj_id_sz;
    char * data;
	size_t data_sz;
    char output[BREWED_TEA_MAX_LEN];
    bool use_desc;

	if (param_types != exp_param_types)
		return TEE_ERROR_BAD_PARAMETERS;


	DMSG("has been called");

	obj_id_sz = strlen(params[0].memref.buffer);
	obj_id = TEE_Malloc(obj_id_sz, 0);
	if (!obj_id)
		return TEE_ERROR_OUT_OF_MEMORY;

    DMSG("Object: %s", obj_id);

	TEE_MemMove(obj_id, params[0].memref.buffer, obj_id_sz);


    TEE_Wait(get_steep_time(obj_id)*1000);


    strcpy(output, BREWED_TEA);


    //if (get_desc_len(obj_id) + BREWED_TEA_LEN < BREWED_TEA_MAX_LEN)
    //{
        //data = get_description(obj_id);
        //data_sz = get_desc_len(obj_id);

        //TEE_MemMove(output+BREWED_TEA_LEN, data, data_sz);
        //free(data);
    //}
    TEE_MemMove(params[1].memref.buffer, output, params[1].memref.size);


	return TEE_SUCCESS;
}

/*
 * Called when a TA is invoked. sess_ctx hold that value that was
 * assigned by TA_OpenSessionEntryPoint(). The rest of the paramters
 * comes from normal world.
 */
TEE_Result TA_InvokeCommandEntryPoint(void __maybe_unused *sess_ctx,
			uint32_t cmd_id,
			uint32_t param_types, TEE_Param params[4])
{
    TEE_Result res;
	(void)&sess_ctx; /* Unused parameter */
    IMSG(" has been called");
    
    /*  Make sure no one wants to do anything with our precious flag! */
    if (cmd_id == TA_TSTLSS_GET_TEA || cmd_id == TA_TSTLSS_BRW_TEA) {
        if (strstr(params[0].memref.buffer,"flag")) {
            return TEE_ERROR_SECURITY;
        }
    }

	switch (cmd_id) {
        case TA_TSTLSS_ADD_TEA:
            return add_tea(param_types, params);
        case TA_TSTLSS_DEL_TEA:
            return del_tea(param_types, params);
        case TA_TSTLSS_MFY_TEA:
            return mfy_tea(param_types, params);
        case TA_TSTLSS_BRW_TEA:
            return brw_tea(param_types, params);
        case TA_TSTLSS_GET_TEA:
            return get_tea(param_types, params);
        case TA_TSTLSS_CMD_PUTFLAG:
            return put_flag(param_types, params);
        case TA_TSTLSS_CMD_GETFLAG: 
            return read_flag(param_types, params);

        default:
            return TEE_ERROR_BAD_PARAMETERS;
	}
}
