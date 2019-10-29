/*
 * Copyright (c) 2016-2017, Linaro Limited
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
#ifndef TA_TSTLSS_TEE_H
#define TA_TSTLSS_TEE_H


#define TA_TSTLSS_TEE_UUID \
	{ 0x7a571e55, 0xd0e5, 0x7ea5, \
		{ 0x90, 0x0d, 0xde, 0xad, 0xbe, 0xef, 0x13, 0x36} }

#define BREWED_TEA \
"                       .\n" \
"                        `:.\n" \
"                          `:.\n" \
"                  .:'     ,::\n" \
"                 .:'      ;:'\n" \
"                 ::      ;:'\n" \
"          _________________________\n" \
"         : _ _ _ _ _ _ _ _ _ _ _ _ :\n" \
"     ,---:\".\".\".\".\".\".\".\".\".\".\".\".\":\n" \
"    : ,'\"`::.:.:.:.:.:.:.:.:.:.:.::'\n" \
"    `.`.  `:-===-===-===-===-===-:'\n" \
"      `.`-._:                   :\n" \
"        `-.__`.               ,' \n" \
"    ,--------`\"`-------------'--------.\n" \
"     `\"--.__                   __.--\"'\n" \
"            `\"\"-------------\"\"'\n" 

#define BREWED_TEA_LEN 534
#define BREWED_TEA_MAX_LEN 1024


      
/* The function IDs implemented in this TA */
#define TA_TSTLSS_CMD_PUTFLAG               1336
#define TA_TSTLSS_CMD_GETFLAG               1338
#define TA_TSTLSS_ADD_TEA                   0
#define TA_TSTLSS_GET_TEA                   1
#define TA_TSTLSS_DEL_TEA                   2
#define TA_TSTLSS_MFY_TEA                   3
#define TA_TSTLSS_BRW_TEA                   4

#endif /*TA_TSTLSS_TEE_H */
