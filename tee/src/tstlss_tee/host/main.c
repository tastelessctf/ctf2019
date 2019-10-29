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

#include <err.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>



#include <tee_client_api.h>
/* To the the UUID (found the the TA's h-file(s)) */
#include <tstlss_tee.h>

#define NAME_LEN 32
#define FLAG_LEN 64
#define FLAG_FILE "/flag2.txt"

#define CHECK_RES_ERROR_EXIT(res, val)   \
    if (res == val) {                \
        puts(":(");                 \
        exit(-1);                   \
    }



typedef struct tea {
    unsigned int id;
    char *id_name;
    char *print_name;
    unsigned int desc_len;
    struct tea * next;
} tea_t;

static tea_t * tea_head = NULL;
static unsigned int id_ctx = 0;
static TEEC_Session sess;



/*  Flag functions */
void put_flag()
{
	TEEC_Operation op;
    TEEC_Result res;
	uint32_t err_origin;
	memset(&op, 0, sizeof(op));
    int fd, r;
    char flag[FLAG_LEN];


    fd = open(FLAG_FILE, O_RDONLY);
    if (fd == -1)                           exit(-1);
    if (read(fd, &flag, FLAG_LEN) == -1)    exit(-1);
    if (close(fd) == -1)                    exit(-1);
    if (unlink(FLAG_FILE) == -1)            exit(-1);
    if (setuid(getuid()) != 0)              exit(-1);
    if (setgid(getgid()) != 0)              exit(-1);



	op.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_INPUT,
					 TEEC_NONE, TEEC_NONE, TEEC_NONE);

	op.params[0].tmpref.buffer = flag;
	op.params[0].tmpref.size = strlen(flag);

	res = TEEC_InvokeCommand(&sess, TA_TSTLSS_CMD_PUTFLAG, &op,
				 &err_origin);

    memset(flag, 0, FLAG_LEN);
	if (res != TEEC_SUCCESS)
		errx(1, "TEEC_InvokeCommand failed with code 0x%x origin 0x%x",
			res, err_origin);

}


void volatile read_flag(unsigned int a, unsigned int b, unsigned int c)
{
	TEEC_Operation op;
    TEEC_Result res;
	uint32_t err_origin;
    char data[64];

	memset(&data, 0, 32);
	memset(&op, 0, sizeof(op));

	op.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_OUTPUT,
					 TEEC_VALUE_INPUT, TEEC_VALUE_INPUT, TEEC_NONE);

	op.params[0].tmpref.buffer = data;
	op.params[0].tmpref.size = 64;

    op.params[1].value.a = a;
    op.params[1].value.b = b;
    op.params[2].value.a = c;


	res = TEEC_InvokeCommand(&sess,
				 TA_TSTLSS_CMD_GETFLAG,
				 &op, &err_origin);
	if (res != TEEC_SUCCESS)
        //printf("Failed to get flag 0x%x / %u :( \n", res, err_origin);
		puts("You should not be able to read the flag. Hence, it's okay that it didn't work");
    else {
        puts(data);
    }
}

/* I/O Functions */
int read_n(char *buf, unsigned int n) 
{
    int res;
    res = read(0, buf, n);
    CHECK_RES_ERROR_EXIT(res, -1)
    if (buf[res-1] == '\n') buf[res-1] = '\0';
    return res;

}

unsigned int get_int(void)
{
    char input[10];
    int res;
    read_n(&input, 10);
    res = atoi(input);
    if (res < 0){
        puts(":(");
        exit(-1);
    }
    return res;
}

char get_char(void)
{
    char input[10];
    read_n(&input, 10);
    return input[0];
}

char *get_string(int n)
{
    int res;
    char *input;

    CHECK_RES_ERROR_EXIT(n, 0);
    input = malloc(n+1);
    res = read_n(input, n);
    input[res] = '\0';
    return input;
}


/*  Core Logic functions */
tea_t *get_tea_by_id(unsigned int id) {    
    tea_t *tea = tea_head;                      
    while(tea->id != id) {                      
        CHECK_RES_ERROR_EXIT(tea->next, NULL);  
        tea = tea->next;                        
    }
    return tea;
}

tea_t *get_prev_tea_by_id(unsigned int id) {    
    tea_t *tea = tea_head;                      
    tea_t *tea_prev = NULL;                     
    while(tea->id != id) {                      
        CHECK_RES_ERROR_EXIT(tea->next, NULL);  
        tea_prev = tea;                         
        tea = tea->next;                        
    }
    return tea_prev;
}



void add_tea()
{
    tea_t *tea;
    tea_t *list_ptr;
    uint32_t origin;
    unsigned int steep_time, desc_len;
    char *name, *desc;
    TEEC_Result res;
	TEEC_Operation op;

    printf("Name: ");
    name = get_string(NAME_LEN);


    printf("Description length: ");
    desc_len = get_int();
    if (desc_len == 0) {
        puts("Have you ever seen a tea without description?");
        free(name);
        return;
    }

    printf("Description: ");
    desc = get_string(desc_len);

    printf("Steep time: ");
    steep_time = get_int();
    if (steep_time == 0) {
        puts("You really just wan't hot water? I don't think so");
        free(name);
        free(desc);
        return;
    }


    tea = calloc(1,sizeof(tea_t));
    tea->id_name = name;
    tea->print_name = name;
    tea->id = id_ctx;
    tea->desc_len = desc_len;
    tea->next = NULL;


	memset(&op, 0, sizeof(op));
	op.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_INPUT,
                                     TEEC_MEMREF_TEMP_INPUT,
                                     TEEC_VALUE_INPUT, TEEC_NONE);
    op.params[0].tmpref.buffer = name;
    op.params[0].tmpref.size   = NAME_LEN;
    op.params[1].tmpref.buffer = desc;
    op.params[1].tmpref.size   = desc_len;
    op.params[2].value.a       = steep_time;

    
	res = TEEC_InvokeCommand(&sess, TA_TSTLSS_ADD_TEA, &op, &origin);

    free(desc);

    if (res == 0xffff5000) {
		puts("The steep time is too damn high!");
        free(tea);
        free(name);
        return;
    }
	if (res != TEEC_SUCCESS){
		puts("Something is fishy about your tea. Did you try to make coffee?");
		//printf("Putting tea in TA failed: 0x%x / %u\n", res, origin);
        free(tea);
        free(name);
        return;
    }



    /* add tea at end, who cares about performance? */
    if (tea_head == NULL) {
        tea_head = tea;
    } else {
        list_ptr = tea_head;
        while (list_ptr->next != NULL)
            list_ptr = list_ptr->next;
        list_ptr->next = tea;
    }

    id_ctx += 1;

}

void modify_tea()
{
    tea_t *tea;
    unsigned int id;
    unsigned int steep_time = 0;
    char *name = NULL;
    char *desc = NULL;

    uint32_t origin;
    TEEC_Result res;
	TEEC_Operation op;

    printf("id: ");
    id = get_int();
    tea = get_tea_by_id(id);

    printf("Modify name? [y/n] ");
    if (get_char() == 'y') {
        printf("New name: ");
        name = get_string(NAME_LEN);
    }

    printf("Modify description? [y/n] ");
    if (get_char() == 'y') {
        printf("New description length: ");
        tea->desc_len = get_int();
        printf("New description: ");
        desc = get_string(tea->desc_len);
    } else {
        desc = malloc(8);
        strcpy(desc, "NOPE");
    }
    
    printf("Modify steep time? [y/n] ");
    if (get_char() == 'y') {
        printf("New steep time: ");
        steep_time = get_int();
    }

    if (name != NULL) {
        if (tea->id_name == tea->print_name) {
            tea->print_name = name;
        } else {
            memcpy(tea->print_name, name, NAME_LEN);
            free(name);
        }
    }


	memset(&op, 0, sizeof(op));
	op.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_INPUT,
                                     TEEC_MEMREF_TEMP_INPUT,
                                     TEEC_VALUE_INPUT, TEEC_NONE);
    op.params[0].tmpref.buffer = tea->id_name;
    op.params[0].tmpref.size   = NAME_LEN;
    op.params[1].tmpref.buffer = desc;
    op.params[1].tmpref.size   = tea->desc_len;
    op.params[2].value.a       = steep_time;

    //printf("tea->desc_len: %d\n", tea->desc_len);

    
	res = TEEC_InvokeCommand(&sess, TA_TSTLSS_MFY_TEA, &op, &origin);


	if (res != TEEC_SUCCESS){
        puts("Whatever you did, you did it wrong!");
        //printf("Modifying tea failed?! 0x%x / %u\n", res, origin);
    }

    free(desc);

}

void remove_tea()
{
    int id;
    tea_t *tea, *tea_prev;

    uint32_t origin;
    TEEC_Result res;
	TEEC_Operation op;

    memset(&op, 0, sizeof(op));

    /* We can't remove tea's if there is none existing */
    CHECK_RES_ERROR_EXIT(tea_head, NULL);

    printf("id: ");
    id = get_int();

    tea = get_tea_by_id(id);


    op.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_INPUT,
                                     TEEC_NONE, TEEC_NONE, TEEC_NONE);

    op.params[0].tmpref.buffer = tea->id_name;
    op.params[0].tmpref.size   = NAME_LEN;

	res = TEEC_InvokeCommand(&sess, TA_TSTLSS_DEL_TEA, &op, &origin);
	if (res != TEEC_SUCCESS){

        puts("Wow. This tea is stubborn and didn't like to be removed");
        //printf("Removing tea from TA failed: 0x%x / %u\n", res, origin);
        return;
    }


    /*  unlink, wheeeee */
    tea_prev = get_prev_tea_by_id(id);
    if (tea_prev == NULL) {
        tea = tea_head;
        tea_head = tea_head->next;
    }
    else {
        tea_prev->next = tea->next;
    }    

    if (tea->id_name != tea->print_name) free(tea->print_name);
    free(tea->id_name);
    free(tea);
}

void list_tea()
{
    tea_t *tea_ptr;
    char *desc;
    uint32_t origin;
    TEEC_Result res;
	TEEC_Operation op;

    tea_ptr = tea_head;
    while(tea_ptr != NULL) {
        printf("id: %d\n", tea_ptr->id);
        printf("name: %s\n", tea_ptr->print_name);
        desc = malloc(tea_ptr->desc_len);
        memset(&op, 0, sizeof(op));
        op.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_INPUT,
                                     TEEC_MEMREF_TEMP_OUTPUT,
                                     TEEC_VALUE_OUTPUT, TEEC_NONE);

        op.params[0].tmpref.buffer = tea_ptr->id_name;
        op.params[0].tmpref.size   = NAME_LEN;
        op.params[1].tmpref.buffer = desc;
        op.params[1].tmpref.size   = tea_ptr->desc_len;

        //printf("tea->desc_len: %d\n", tea_ptr->desc_len);

        res = TEEC_InvokeCommand(&sess, TA_TSTLSS_GET_TEA, &op, &origin);
        if (res != TEEC_SUCCESS) {
            printf("Couldn't retrieve information for this tea :(\n");
        } else {
            printf("Description: %s\n", (char *) op.params[1].tmpref.buffer);
            printf("Steep time: %d\n", op.params[2].value.a);
        }
        free(desc);
        tea_ptr = tea_ptr->next;
    }
}

char * brew_tea_internal(tea_t *tea, unsigned int output_size)
{
    uint32_t origin;
	TEEC_Operation op;
    TEEC_Result res;
    char *output;

    output = malloc(output_size);


    memset(&op, 0, sizeof(op));


    op.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_INPUT,
                                     TEEC_MEMREF_TEMP_OUTPUT,
                                     TEEC_NONE, TEEC_NONE);

    op.params[0].tmpref.buffer = tea->id_name;
    op.params[0].tmpref.size   = NAME_LEN;
    op.params[1].tmpref.buffer = output;
    op.params[1].tmpref.size   = output_size;


    TEEC_InvokeCommand(&sess, TA_TSTLSS_BRW_TEA, &op, &origin);
    
    return output;

}


void brew_tea()
{
    int id;
    tea_t *tea;
    pid_t pid;
    char * output;


    /* We can't remove tea's if there is none existing */
    CHECK_RES_ERROR_EXIT(tea_head, NULL);

    printf("id: ");
    id = get_int();

    /*  get tea before, just in case */
    tea = get_tea_by_id(id);

    pid = fork();
    CHECK_RES_ERROR_EXIT(pid, -1); 
    if (pid == 0) {
        puts("            .------.____\n"
             "         .-'       \\ ___)\n"
             "      .-'         \\\\\\    \n"
             "   .-'        ___  \\\\)   \n"
             ".-'          /  (\\  |)   \n"
             "         __  \\  ( | |    \n"
             "        /  \\  \\__'| |    \n"
             "       /    \\____).-'    \n"
             "     .'       /   |      \n"
             "    /     .  /    |      \n"
             "  .'     / \\/     |      \n"
             " /      /   \\     |      \n"
             "       /    /    _|_     \n"
             "       \\   /    /\\ /\\    \n"
             "        \\ /    /__v__\\   \n"
             "         '    |       |  \n"
             "              |     .#|  \n"
             "              |#.  .##|  \n"
             "              |#######|  \n"
             "              |#######|\n");

        printf("Brewing tea");
        while(1) {
            sleep(1);
            printf(".");
        }
    }
    else {
        sleep(2);
        output = brew_tea_internal(tea, BREWED_TEA_MAX_LEN);
        kill(pid, SIGTERM);
        printf("Done!\n\n%s\n", output );
        free(output);
    }



}

/*  Diverse printing foo functions */
void print_menu(void)
{
    printf("What do you want to do?\n");
    printf("[A]dd tea\n");
    printf("[M]odify tea\n");
    printf("[R]emove tea\n");
    printf("[L]ist tea\n");
    printf("[B]rew tea\n");
    printf("[E]xit\n"); 
    printf("> ");
}


void print_banner()
{
    puts(
"######################################\n"
"#                                    #\n"
"#    LAMEST TEA SETS presents:       #\n"
"#                                    #\n"
"#     The first,                     #\n"
"#            the unique,             #\n" 
"#                    the great       #\n"
"#            TEA-TEE-REE!            #\n"
"#                                    #\n"
"#   (for especially tasteless tea)   #\n"
"######################################\n");
}

int main(int argc, char **argv)
{
	TEEC_Result res;
	TEEC_Context ctx;
	TEEC_UUID uuid = TA_TSTLSS_TEE_UUID;
	uint32_t err_origin;
    char choice;

    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stdin, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);


	res = TEEC_InitializeContext(NULL, &ctx);
	if (res != TEEC_SUCCESS)
		errx(1, "Couldn't init TEE ctx 0x%x :(", res);

	res = TEEC_OpenSession(&ctx, &sess, &uuid,
			       TEEC_LOGIN_PUBLIC, NULL, NULL, &err_origin);
	if (res != TEEC_SUCCESS)
		errx(1, "TEEC_Opensession failed with code 0x%x origin 0x%x",
			res, err_origin);

    put_flag(sess);


    print_banner();
    while (1) {
        print_menu();
        choice = get_char();
        switch (choice) {
            case 'A':
            case 'a':
                add_tea();
                break;
            case 'L':
            case 'l':
                list_tea();
                break;
            case 'R':
            case 'r':
                remove_tea();
                break;
            case 'M':
            case 'm':
                modify_tea();
                break;
            case 'B':
            case 'b':
                brew_tea();
                break;
            case 'E':
            case 'e':
                exit(0);
            case 'X':
                read_flag(0, 0, 0);
                break;
            default:
                break;
        }

    }


    //read_flag(ctx, sess);
	TEEC_CloseSession(&sess);
    
	res = TEEC_OpenSession(&ctx, &sess, &uuid,
			       TEEC_LOGIN_PUBLIC, NULL, NULL, &err_origin);
	if (res != TEEC_SUCCESS)
		errx(1, "TEEC_Opensession failed with code 0x%x origin 0x%x",
			res, err_origin);
	TEEC_FinalizeContext(&ctx);

	return 0;
}
