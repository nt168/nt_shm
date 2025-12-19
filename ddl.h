/*
 * ddl.c
 *
 * Created on: Aug 26, 2024
 * Author: Snow
 */
#ifndef DDL_H
#define DDL_H
#include <stdio.h>
#include <string.h>

typedef enum{
	up=1,
	dn
}ddlctr;

typedef struct ddl {
	void *data;
	size_t dln;
	struct ddl* next;
	struct ddl* prev;
} ddl;

typedef struct ddlh {
	ddl* entr;
	ddl* tail;
	ddl* curr;
	ddl* pos;
	size_t num;
}ddlh;

void ddl_init(ddlh **dh);
void ddl_delete(ddlh** dh, ddl* di);
void ddl_insert_first(ddlh **dh, ddl* di);
void ddl_insert_end(ddlh **dh, ddl* di);
void ddl_insert_before(ddlh *dh, ddl* di);
void ddl_insert_after(ddlh *dh, ddl* di);
void ddl_positive_iterator(ddlh *dh);
void ddl_reverse_iterator(ddlh *dh);
void ddl_insert(ddlh **dh, void* data, size_t len, int flg);
void ddl_reverse(ddlh *dh);
void ddl_merge(ddlh* dh1, ddlh* dh2);
void ddl_destory(ddlh* dh);
void* ddl_control(ddlh* dh, ddlctr op);
#endif
