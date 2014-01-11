#ifndef HTML_H
#define HTML_H

void dumpNode(TidyDoc tdoc, TidyNode tnod, int indent);

TidyAttr findAttribute(TidyNode node, char * attrname);

TidyNode findNodeById(TidyNode node, char * id);

#endif
