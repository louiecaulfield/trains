#include <tidy/tidy.h>
#include <tidy/buffio.h>
#include "include/dbg.h"

void dumpNode(TidyDoc tdoc, TidyNode tnod, int indent)
{
	TidyNode child;
	for(child = tidyGetChild(tnod); child; child = tidyGetNext(child))
	{
		ctmbstr name = tidyNodeGetName(child);
		if(name)
		{
			//if it has a name, it's a tag
			TidyAttr attr;
			printf("%*.*s%s ", indent, indent, "<", name);
			// walk attr list
			for(attr = tidyAttrFirst(child); attr; attr=tidyAttrNext(attr)) {
				printf("%s", tidyAttrName(attr));
				tidyAttrValue(attr)?printf("=\"%s\" ",
					tidyAttrValue(attr)):printf(" ");
			}
			printf(">\n");
		} else {
			TidyBuffer buf;
			tidyBufInit(&buf);
			tidyNodeGetText(tdoc, child, &buf);
			printf("%*.*s\n", indent, indent, buf.bp?(char *)buf.bp:"");
			tidyBufFree(&buf);
		}
		dumpNode(tdoc, child, indent + 4);
	}
}

TidyAttr findAttribute(TidyNode node, char * attrname)
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

TidyNode findNodeById(TidyNode node, char * id)
{
	TidyNode child, match = NULL;
	TidyAttr attr;
	for(child = tidyGetChild(node); child; child = tidyGetNext(child)) {
		if( (attr = findAttribute(child, "id") ) ) {
			debug("attr = %p, match = %p", attr, match);
			if(!strcmp(id, tidyAttrValue(attr))) {
				//Match!
				debug("found a match!");
				return child;
			}
		}
		if ( (match = findNodeById(child, id)) ) {
			debug("Match found!");
			return match;
		}
	}
	return NULL;
}
