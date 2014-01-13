#ifndef HTML_H
#define HTML_H

#include <tidy/tidy.h>
struct node_list 
{
	TidyNode node;
	struct node_list *next;
};

int printNode(TidyNode tnod, int indent);

void dumpNode(TidyDoc tdoc, TidyNode tnod, int indent);

typedef int (*fn_test)(TidyNode node, const char * string);

TidyAttr findAttribute(TidyNode node, const char * attrname);

TidyNode findNodeById(TidyNode node, const char * id);

void freeNodeList(struct node_list * list);

struct node_list * findNodes(fn_test node_test, TidyNode node,
		const char *str_test, struct node_list *node_list_tail);
struct node_list * findNodesByName (TidyNode node, const char * name);
struct node_list * findNodesByClass(TidyNode node, const char * class);

#endif
