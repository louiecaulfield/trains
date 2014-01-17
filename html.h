#ifndef HTML_H
#define HTML_H

#include <tidy/tidy.h>

struct node_list 
{
	TidyNode node;
	struct node_list *next;
};

/*
 * node output functions
 */
int printNode(TidyNode tnod, int indent);
void dumpNode(TidyDoc tdoc, TidyNode tnod, int indent);
void getNodeText(TidyDoc tdoc, TidyNode tnod, char **data);

/*
 * node tests
 */
typedef int (*fn_test)(TidyNode node, const char * string);
int testNodeName(TidyNode node, const char * name);
int testNodeClass(TidyNode node, const char * class);

/*
 * node finders
 */
TidyAttr findAttribute(TidyNode node, const char * attrname);
const char * getAttributeValue(TidyNode node, const char * attribute_name);
TidyNode findNodeById(TidyNode node, const char * id);

struct node_list * findNodes(fn_test node_test, TidyNode node,
		const char *str_test, struct node_list *node_list_tail);
int findNodesByName (struct node_list ** list, TidyNode node, const char * name);
int findNodesByClass(struct node_list ** list, TidyNode node, const char * class);

/* 
 * struct node_list helpers
 */
int countNodeList(struct node_list * list);
void freeNodeList(struct node_list ** list);

#endif
