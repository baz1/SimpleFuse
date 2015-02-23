#ifndef __SIMPLIFIER_H__
#define __SIMPLIFIER_H__

#ifndef FUSE_USE_VERSION
#define FUSE_USE_VERSION 26
#endif

#include <fuse.h>

#define STR_LEN_MAX 255

struct PersistentData
{
    stat def_stat;
};

#define PERSDATA ((PersistentData*) fuse_get_context()->private_data)

void makeSimplifiedFuseOperations();

fuse_operations *getSimplifiedFuseOperations();

#endif /* Not __SIMPLIFIER_H__ */
