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
const char * getAttributeValue(TidyNode node, const char * attribute_name);

TidyNode findNodeById(TidyNode node, const char * id);

int countNodeList(struct node_list * list);
void freeNodeList(struct node_list * list);

struct node_list * findNodes(fn_test node_test, TidyNode node,
		const char *str_test, struct node_list *node_list_tail);
int findNodesByName (struct node_list ** list, TidyNode node, const char * name);
int findNodesByClass(struct node_list ** list, TidyNode node, const char * class);

#endif
