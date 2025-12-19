#ifndef NT_PTCL_H
#define NT_PTCL_H
#include <stdint.h>
#include "nt_log.h"
/*net protocol
[head|len|checkcode|datapackage]
                        |
                        v
                        [type|send|receiver|proxy|data]
*/

/*ntptcli
[head|len|checkcode|datapackage]
 */
#define NTPTCLIHEAD "nt head data"
#define NTPTCLITAIL "nt tail data"
struct ntptcli
{
	char head[13];
	unsigned int len;
	char ckcode[41];
	char pakg[0];
};
/*ntptclii datapackage
[type|send|receiver|proxy|data]
*/
typedef enum
{
	NT_FILE = 0,
	NT_STRING,
	NT_CMD,
	NT_STATUS,
	NT_SYNC,
}nt_type;

struct ntptclii
{
	nt_type type;
	char data[0];
};

struct ntptclfile
{
	char sender[32];
	char receiver[32];
	char proxy[32];
	char date[32];
	unsigned int timestamp;
	char folder[1024];
	char name[256];
	unsigned int chknm;
	unsigned int soffset;
	unsigned int eoffset;
	char data[0];
};

struct ntptclcmd
{
	char sender[32];
	char receiver[32];
	char proxy[32];
	char date[32];
	unsigned int timestamp;
	char data[0];
};

struct ntptclstatus
{
	char sender[32];
	char receiver[32];
	char proxy[32];
	char date[32];
	unsigned int timestamp;
	char data[0];
};

struct ntptclsts
{
	char sender[32];
	char receiver[32];
	char proxy[32];
	char date[32];
	unsigned int timestamp;
	char data[0];
};

typedef enum
{
	NT_TASK = 0,
	NT_FETCH,
	NT_FINISH,
	NT_LASTONE,
}nt_sync;

struct ntptclsync
{
	char sender[32];
	char receiver[32];
	char proxy[32];
	char date[32];
	unsigned int timestamp;
	nt_sync tsync;
	char data[0];
};

void nt_ptcl_send(const char* addr, int port, const char* data, size_t len, nt_type ty);
void nt_ptcl_handle(const void* data, size_t rlen);
#if 0
struct ntptclstr
{
	char sender[32];
	char receiver[32];
	char proxy[32];
	char data[0];
};
#endif

typedef struct
{
    uint32_t state[5];
    uint32_t count[2];
    unsigned char buffer[64];
} SHA1_CTX;

void SHA1Transform(
    uint32_t state[5],
    const unsigned char buffer[64]
    );

void SHA1Init(
    SHA1_CTX * context
    );

void SHA1Update(
    SHA1_CTX * context,
    const unsigned char *data,
    uint32_t len
    );

void SHA1Final(
    unsigned char digest[20],
    SHA1_CTX * context
    );

void SHA1(
    char *hash_out,
    const char *str,
    int len);

#endif
