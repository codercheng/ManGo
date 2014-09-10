#ifndef _SCM_H_
#define _SCM_H_

#include <sys/stat.h>

#define SCM_VERSION_NUMBER	"0.04"
#define SCM_DEFAULT_BRANCH	"master"

#define SCM_FOLDER			".scm"
#define SCM_BRANCH_FOLDER	".scm/branch"
#define SCM_OBJECTS_FOLDER	".scm/obj"

#define SCM_BLOB_FOLDER		".scm/obj/blob"
#define SCM_COMMIT_FOLDER	".scm/obj/commit"
#define SCM_TREE_FOLDER		".scm/obj/tree"
#define SCM_TEMP_FOLDER		".scm/tmp"

#define SCM_BRANCH_CACHE_FOLDER	"cache"

#define SCM_HEAD_FILE		".scm/HEAD"
#define SCM_INDEX_FILENAME	"index"
#define SCM_COMMIT_FILENAME "commit"

#define SCM_FOLDER_PERMISSION		S_IRWXU | S_IROTH | S_IRGRP
#define SCM_HEAD_FILE_PERMISSION	S_IRWXU | S_IROTH | S_IRGRP
#define SCM_OBJECT_FILE_PERMISSION	S_IRUSR | S_IROTH | S_IRGRP

#endif
