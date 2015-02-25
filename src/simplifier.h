/*
 * Copyright (c) 2015, Rémi Bazin <bazin.remi@gmail.com>
 * All rights reserved.
 * See LICENSE for licensing details.
 */

#ifndef __SIMPLIFIER_H__
#define __SIMPLIFIER_H__

#ifndef FUSE_USE_VERSION
#define FUSE_USE_VERSION 26
#endif

#include <fuse.h>

#define STR_LEN_MAX 255

struct PersistentData
{
    struct stat def_stat;
};

#define PERSDATA ((PersistentData*) fuse_get_context()->private_data)

void makeSimplifiedFuseOperations();

fuse_operations *getSimplifiedFuseOperations();

#endif /* Not __SIMPLIFIER_H__ */
