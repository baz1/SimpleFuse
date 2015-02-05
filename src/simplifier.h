#ifndef __SIMPLIFIER_H__
#define __SIMPLIFIER_H__

#define STR_LEN_MAX 255

#define PERSDATA ((PersistentData*) fuse_get_context()->private_data)

void makeSimplifiedFuseOperations();

fuse_operations *getSimplifiedFuseOperations();

#endif /* Not __SIMPLIFIER_H__ */
