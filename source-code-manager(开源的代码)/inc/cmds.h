#ifndef CMDS_H_
#define CMDS_H_
int cmd_version(int argc, char *argv[]);
int cmd_sha(int argc, char *argv[]);
int cmd_init(int argc, char *argv[]);
int cmd_branch(int argc, char *argv[]);
int cmd_status(int argc, char *argv[]);
int cmd_add(int argc, char *argv[]);
int cmd_ls(int argc, char *argv[]);
int cmd_commit(int argc, char *argv[]);
int cmd_info(int argc, char *argv[]);
int cmd_rm(int argc, char *argv[]);
int cmd_checkout(int argc, char *argv[]);
#endif
