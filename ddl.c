/*
 * ddl.c
 *
 * Created on: Aug 26, 2024
 * Author: Snow
 */

#include "ddl.h"
#include <stdlib.h>
void ddl_init(ddlh **dh)
{
	(*dh) = malloc(sizeof(ddlh));
	memset((*dh), 0, sizeof(ddlh));
	(*dh)->curr = NULL;
	(*dh)->entr = NULL;
	(*dh)->num = 0;
	(*dh)->pos = NULL;
	(*dh)->tail = NULL;
}

//void ddl_insert(ddlh **dh, void* data, size_t len, int flg)
//{
//	ddl* di = NULL;
//	di = malloc(sizeof(ddl));
//	memset(di, 0, sizeof(ddl));
//	di->data = NULL;
//	di->data = malloc(len);
//	memcpy(di->data, data, len);
////insert end
//	if(flg == 1){
//		ddl_insert_end(dh, di);
//	}else if(flg == 0){
//		ddl_insert_first(dh, di);
//	}else{
//	}
//}

void ddl_insert(ddlh **dh, void* data, size_t len, int flg) {
    ddl* di = NULL;
    di = malloc(sizeof(ddl));
    if (di == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        return;
    }
    memset(di, 0, sizeof(ddl));

    // 分配存储 data 的内存
    di->data = malloc(len);
    if (di->data == NULL) {
        fprintf(stderr, "Memory allocation for data failed\n");
        free(di);
        return;
    }

    // 确保 data 中的数据没有越界，安全地进行数据拷贝
    // 可以使用 len 来指定实际分配的大小，但需要确认 data 的结构和大小
    memcpy(di->data, data, len);

    // 插入逻辑
    if (flg == 1) {
        ddl_insert_end(dh, di);
    } else if (flg == 0) {
        ddl_insert_first(dh, di);
    } else {
        // 其他插入逻辑
    }
}

void ddl_insert_first(ddlh **dh, ddl* di)
{
	ddl* dp = NULL;
	if((*dh)->entr == NULL){
		(*dh)->entr = di;
		(*dh)->curr = (*dh)->entr;
		(*dh)->pos = (*dh)->entr;
		(*dh)->tail = (*dh)->entr;
		(*dh)->num = 1;
	}else{
		dp = (*dh)->entr;//(*dh)->curr;
		(*dh)->entr = di;
		di->next = dp;
		dp->prev = di;
//		(*dh)->curr = di;

//		dp->prev = (*dh)->curr;
//		(*dh)->tail = dp;
	}
}

void ddl_insert_end(ddlh **dh, ddl* di)
{
	ddl* dp = NULL;
	if((*dh)->entr == NULL){
		(*dh)->entr = di;
		(*dh)->curr = (*dh)->entr;
		(*dh)->pos = (*dh)->entr;
		(*dh)->tail = (*dh)->entr;
		(*dh)->num = 1;
	}else{
		dp = (*dh)->curr;
		dp->next = di;
		(*dh)->curr = di;
		(*dh)->num++;

		di->prev = dp;

		(*dh)->tail = di;
	}
}

void ddl_positive_iterator(ddlh *dh)
{
	ddl* dp = NULL;
	dp = dh->entr;
	while(dp){
		printf("%s\n", (char*)(dp->data));
		dp = dp->next;
	}
}

void ddl_reverse_iterator(ddlh *dh)
{
	ddl* dp = NULL;
	dp = dh->tail;
	while(dp){
		printf("%s\n", (char*)(dp->data));
		dp = dp->prev;
	}
}

void ddl_destory(ddlh *dh)
{
	ddl* dp = NULL;
	ddl* tdp = NULL;
	dp = dh->entr;
	tdp = dp;
	while(dp){
		tdp = dp->next;
		free(dp->data);
		free(dp);
		dp = tdp;
	}
	free(dh);
}

void* ddl_control(ddlh* dh, ddlctr op)
{
	void* ret = NULL;
	if(op == up){
		if(dh->pos->prev != NULL){
			ret = dh->pos->prev;
			dh->pos = dh->pos->prev;
		}else{
			ret = dh->pos;
		}
	}else if(op == dn){
		if(dh->pos == NULL){
			return NULL;
		}
		if(dh->pos->next != NULL){
			ret = dh->pos->next;
			dh->pos = dh->pos->next;
		}else{
			ret = dh->pos;
		}
	}else{
		return NULL;
	}
	return ret;
}

void ddl_delete(ddlh** dh, ddl* di)
{
	ddl* dp = NULL;
	ddl* tp = NULL;
	tp = (*dh)->entr;

	if(tp == NULL || di == NULL){
		return;
	}

	if(tp == di){
		free(di->data);
		free(di);
		(*dh)->curr = NULL;
		(*dh)->pos = NULL;
		(*dh)->entr = NULL;
		(*dh)->tail = NULL;
		(*dh)->num = 0;
		return;
	}

	dp = tp->next;
	while(dp){
		if(dp == di)
		{
			(*dh)->num--;
			if(di == (*dh)->curr){
				(*dh)->curr = dp->next;
			}

			if(di == (*dh)->pos){
				(*dh)->pos = tp;
			}

			tp->next = dp->next;
			if(dp->next != NULL){
				dp->next->prev = tp;
			}else{
				(*dh)->tail = tp;
			}

			free(dp->data);
			free(dp);
			break;
		}
		tp = dp;
		dp = dp->next;
	}
}
