/**
 * @file
 * ACPI list.
 */

/*
 * Copyright (c) 2006-2016 SylixOS Group.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 * Author: Jiao.JinXing <jiaojinxing@acoinfo.com>
 *
 */

#ifndef _ACPI_LIST_H_
#define _ACPI_LIST_H_

/*
 * The definition of a key
 * in generic data-structures
 * this can be values or data
 */
typedef union _DataKey {
    /* Integer Value */
    int Value;

    /* Data Pointer */
    void *Pointer;

    /* String */
    char *String;
} DataKey_t;

/*
 * This enumeration denotes
 * the kind of key that is
 * to be interpreted by the
 * data-structure
 */
typedef enum _KeyType {
    KeyInteger,
    KeyPointer,
    KeyString
} KeyType_t;

/*
 * The list node structure
 * this is a generic list item
 * that holds an ident (key)
 * and data
 */
typedef struct _ListNode {
	/* Key(s) */
	DataKey_t Key;
	DataKey_t SortKey;

	/* Payload */
	void *Data;

	/* Link(s) */
	struct _ListNode *Link;
	struct _ListNode *Prev;
} ListNode_t;

/*
 * This is the list structure
 * it holds basic information
 * about the list
 */
typedef struct _List {
	/* Key type */
	KeyType_t KeyType;

	/* Head and Tail */
	ListNode_t *Headp, *Tailp;

	/* Attributes */
	int Attributes;

	/* Length */
	int Length;

	/* Perhaps we use a lock */
	spinlock_t Lock;
} List_t;

/*
 * List Definitions
 */
#define LIST_NORMAL				0x0
#define LIST_SAFE				0x1

/*
 * Sorted list? Normally
 * the list is unsorted 
 * but supports different sorts
 */
#define LIST_SORT_ONINSERT		0x2
#define LIST_SORT_ONCALL		(0x4 | LIST_SORT_ONINSERT)

/*
 * Foreach Macro(s)
 * They help keeping the code 
 * clean and readable when coding
 * loops
 */
#define foreach(i, List) ListNode_t *i; for (i = List->Headp; i != NULL; i = i->Link)
#define _foreach(i, List)               for (i = List->Headp; i != NULL; i = i->Link)
#define _foreach_nolink(i, List)        for (i = List->Headp; i != NULL; )

/*
 * Instantiates a new list
 * with the given attribs and keytype
 */
extern List_t *ListCreate(KeyType_t KeyType, int Attributes);

/*
 * Destroys the list and
 * frees all resources associated
 * does also free all list elements
 * and keys
 */
extern void ListDestroy(List_t *List);

/*
 * Returns the length of the
 * given list
 */
extern int ListLength(List_t *List);

/*
 * Instantiates a new list node
 * that can be appended to the list 
 * by ListAppend. If using an unsorted list
 * set the sortkey == key
 */
extern ListNode_t *ListCreateNode(DataKey_t Key, DataKey_t SortKey, void *Data);

/*
 * Cleans up a list node and frees
 * all resources it had
 */
extern void ListDestroyNode(List_t *List, ListNode_t *Node);

/*
 * Insert the node into a specific position
 * in the list, if position is invalid it is
 * inserted at the back. This function is not
 * available for sorted lists, it will simply 
 * call ListInsert instead
 */
extern void ListInsertAt(List_t *List, ListNode_t *Node, int Position);

/*
 * Inserts the node into the front of
 * the list. This should be used for sorted
 * lists, but is available for unsorted lists
 * aswell
 */
extern void ListInsert(List_t *List, ListNode_t *Node);

/*
 * Inserts the node into the the back
 * of the list. This function is not
 * available for sorted lists, it will
 * simply redirect to ListInsert
 */
extern void ListAppend(List_t *List, ListNode_t *Node);

/*
 * List pop functions, the either
 * remove an element from the back or 
 * the front of the given list and return
 * the node
 */
extern ListNode_t *ListPopFront(List_t *List);
extern ListNode_t *ListPopBack(List_t *List);

/*
 * These are the index-retriever functions
 * they return the given index by either 
 * Key, data or node, return -1 if not found
 */
extern int ListGetIndexByData(List_t *List, void *Data);
extern int ListGetIndexByKey(List_t *List, DataKey_t Key);
extern int ListGetIndexByNode(List_t *List, ListNode_t *Node);

/*
 * These are the node-retriever functions
 * they return the list-node by either key
 * data or index
 */
extern ListNode_t *ListGetNodeByKey(List_t *List, DataKey_t Key, int n);

/*
 * These are the data-retriever functions
 * they return the list-node by either key
 * node or index
 */
extern void *ListGetDataByKey(List_t *List, DataKey_t Key, int n);

/*
 * These functions execute a given function
 * on all relevant nodes (see names)
 */
extern void ListExecuteOnKey(List_t *List, void(*Function)(void*, int, void*), DataKey_t Key, void *UserData);
extern void ListExecuteAll(List_t *List, void(*Function)(void*, int, void*), void *UserData);

/*
 * This functions unlinks a node
 * and returns the next node for
 * usage
 */
extern ListNode_t *ListUnlinkNode(List_t *List, ListNode_t *Node);

/*
 * These are the deletion functions
 * and remove based on either node 
 * index or key
 */
extern void ListRemoveByNode(List_t *List, ListNode_t* Node);
extern void ListRemoveByIndex(List_t *List, int Index);
extern int ListRemoveByKey(List_t *List, DataKey_t Key);

#endif /* _ACPI_LIST_H_ */
