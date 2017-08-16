/*
 * QEMU Guest Agent POSIX-specific utils
 *
 * Copyright IBM Corp. 2017
 *
 * Authors:
 *  zhaozhongguo
 *
 * This work is licensed under the terms of the GNU GPL, version 2 or later.
 * See the COPYING file in the top-level directory.
 */
#ifndef __UTILS_COMMON_H
#define __UTILS_COMMON_H

#define DEFAULT_DELAY 1


//malloc for stat list
#define STAT_LIST_ALLOCATE(type, length, out_pList) \
    do {\
        if (length <= 0) \
        { \
            out_pList = NULL; \
            break; \
        } \
        \
        struct type##_list* t_list = (struct type##_list*)malloc(sizeof(struct type##_list)); \
        if (NULL == t_list) \
        { \
            out_pList = NULL; \
            break; \
        } \
        memset((char*)t_list, 0, sizeof(struct type##_list)); \
        \
        int size = sizeof(struct type) * length; \
        t_list->list = (struct type*) malloc(size); \
        if (NULL != t_list->list) \
        { \
            memset((char*)t_list->list, 0, size); \
            t_list->capacity = length; \
        } \
        else \
        { \
            free(t_list); \
            out_pList = NULL; \
            break; \
        } \
        \
        out_pList = t_list; \
    } while (0)


//free for stat list
#define STAT_LIST_FREE(pList) \
    do { \
        if (NULL != pList) \
        { \
            if (NULL != pList->list) \
            { \
                free(pList->list); \
            } \
            \
            free(pList); \
        } \
    } while (0)


#endif
