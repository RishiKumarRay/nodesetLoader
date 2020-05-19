#include "NodeContainer.h"
#include "Node.h"
#include <stdlib.h>

NodeContainer *NodeContainer_new(size_t initialSize, bool owner)
{
    NodeContainer *container =
        (NodeContainer *)calloc(1, sizeof(NodeContainer));
    if(!container)
    {
        return NULL;
    }
    container->nodes =
        (TNode **)calloc(initialSize, sizeof(void *) * initialSize);
    if(!container->nodes)
    {
        free(container);
        return NULL;
    }
    container->size = 0;
    container->capacity = initialSize;
    container->incrementSize = initialSize;
    container->owner = owner;
    return container;
}

void NodeContainer_add(NodeContainer *container, TNode *node)
{
    if (container->size == container->capacity)
    {
        container->nodes = (TNode **)realloc(
            container->nodes,
            (container->size + container->incrementSize) * sizeof(void *));
        if(!container->nodes)
        {
            return;
        }
        container->capacity += container->incrementSize;
    }
    container->nodes[container->size] = node;
    container->size++;
}

void NodeContainer_delete(NodeContainer *container)
{
    if (container->owner)
    {
        for (size_t i = 0; i < container->size; i++)
        {
            Node_delete(container->nodes[i]);
        }
    }
    free(container->nodes);
    free(container);
}
