#include <tidy/tidy.h>
#include <tidy/buffio.h>
#include <regex.h>
#include "include/dbg.h"
#include "html.h"

int printNode(TidyNode tnod, int indent)
{
	TidyAttr attr;
	ctmbstr name = tidyNodeGetName(tnod);
	if(!name) return -1; //Fail if we don't have a name

	printf("%*.*s%s ", indent, indent, "<", name);
	// walk attr list
	for(attr = tidyAttrFirst(tnod); attr; attr=tidyAttrNext(attr)) {
		printf("%s", tidyAttrName(attr));
		tidyAttrValue(attr)?printf("=\"%s\" ",
			tidyAttrValue(attr)):printf(" ");
	}
	printf(">\n");
	return 0;
}
void dumpNode(TidyDoc tdoc, TidyNode tnod, int indent)
{
	TidyNode child;
	for(child = tidyGetChild(tnod); child; child = tidyGetNext(child))
	{
		if(printNode(child, indent)) { //Returns -1 if not a name, so contents... 
			TidyBuffer buf;
			tidyBufInit(&buf);
			tidyNodeGetText(tdoc, child, &buf);
			printf("%*.*s\n", indent, indent, buf.bp?(char *)buf.bp:"");
			tidyBufFree(&buf);
		}
		dumpNode(tdoc, child, indent + 4);
	}
}

TidyAttr findAttribute(TidyNode node, const char * attrname)
{
	TidyAttr attr;
	for(attr = tidyAttrFirst(node); attr; attr=tidyAttrNext(attr)) {
		//FIXME: use strncmp instead?
		if(!strcmp(attrname, tidyAttrName(attr))) {
			return attr;
		}
	}
	return NULL;
}

TidyNode findNodeById(TidyNode node, const char * id)
{
	TidyNode child, match = NULL;
	TidyAttr attr;
	for(child = tidyGetChild(node); child; child = tidyGetNext(child)) {
		if( (attr = findAttribute(child, "id") ) ) {
			const char * attr_value;
			attr_value = tidyAttrValue(attr);
			if(attr_value && !strcmp(id, attr_value)) {
				//Match!
				return child;
			}
		}
		if ( (match = findNodeById(child, id)) ) {
			return match;
		}
	}
	return NULL;
}

int testNodeName(TidyNode node, const char * name)
{
	const char *name_node = tidyNodeGetName(node);
	if(name_node && !strcmp(name_node, name)) {
		return 1;
	}
	return 0;
}
//Use regex to account for multiple classes
int testNodeClass(TidyNode node, const char * class)
{
	TidyAttr attr;
	const char * attr_value;

	attr = findAttribute(node, "class");
	if(!attr) return 0;  //No class attribute	

	attr_value = tidyAttrValue(attr);
	if(attr_value) {
		char *regex_patt;
		int res;
		regex_t regex;

		asprintf(&regex_patt, "%s%s%s", "(^| )", class, "( |$)");
		check_mem(regex_patt);
		regcomp(&regex, regex_patt, REG_EXTENDED | REG_NOSUB);
		free(regex_patt);

		res = regexec(&regex, attr_value, 0, NULL, 0);
		regfree(&regex);
		return res?0:1;
	}
error:
	return 0;
}

struct node_list * findNodes(fn_test node_test, TidyNode node, 
		const char *str_test, struct node_list *node_list_tail)
{
	TidyNode child;
	struct node_list *node_list_head = NULL, *r;
	for(child = tidyGetChild(node); child; child = tidyGetNext(child)) {
		if(node_test(child, str_test) ){
			struct node_list *node_list_match = 
				(struct node_list*)malloc(sizeof(struct node_list));	
			check_mem(node_list_match);	
			node_list_match->node = child;
			node_list_match->next = NULL;
			if(node_list_tail) {
				node_list_tail->next = node_list_match;
				node_list_tail = node_list_tail->next;
			} else { //first match ever
				node_list_head = node_list_tail = node_list_match;
			}
		}
		//Iterate over this child's children
		if( (r=findNodes(node_test, child, str_test, node_list_tail)) ) {
			node_list_head = r;
		}
	}
	return node_list_head;
error:
	return NULL;
}

int findNodesByName (struct node_list ** list, TidyNode node, const char * name)
{
	*list = findNodes(&testNodeName, node, name, NULL);
	return countNodeList(*list);
}

int findNodesByClass(struct node_list ** list, TidyNode node, const char * class)
{
	*list = findNodes(&testNodeClass, node, class, NULL);
	return countNodeList(*list);
}

int countNodeList(struct node_list * list)
{
	int i = 0;
	for(struct node_list *node_cur = list; 
		node_cur; node_cur = node_cur->next)
		i++;	
	return i;
}
void freeNodeList(struct node_list * list)
{
	if(!list) return;
	if(list->next)
		freeNodeList(list->next);
	free(list);
}
