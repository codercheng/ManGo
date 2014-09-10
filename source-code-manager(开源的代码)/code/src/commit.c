#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "scm.h"
#include "commit.h"
#include "common.h"


Commit Commit_Create(void)
{
	Commit c;
	c = (Commit)XMALLOC(sizeof(struct _Commit));
	sha_reset(c->parent0);
	sha_reset(c->parent1);
	sha_reset(c->tree);
	c->author = String_Create();
	c->message = String_Create();
	c->rawtime = 0;
	return c;
}

void Commit_Delete(Commit c)
{
	String_Delete(c->author);
	String_Delete(c->message);
	XFREE(c);
	return ;
}

void Commit_Reset(Commit c)
{
	sha_reset(c->parent0);
	sha_reset(c->parent1);
	sha_reset(c->tree);
	c->rawtime = 0;
	String_strcpy(c->author, "\0");
	String_strcpy(c->message, "\0");
	return;
}

bool Commit_SetParent(Commit c, const ShaBuffer parent0, const ShaBuffer parent1)
{
	memcpy(c->parent0, parent0, SHA_HASH_LENGTH);
	memcpy(c->parent1, parent1, SHA_HASH_LENGTH);
	return true;
}

bool Commit_SetTree(Commit c, const ShaBuffer tree)
{
	int len = strlen((char*)tree);
	if(len < SHA_HASH_LENGTH)
	{
		LOG_ERROR("Commit_SetTree() Sha length %d != %d", len, SHA_HASH_LENGTH);
		return false;
	}
	memcpy(c->tree, tree, SHA_HASH_LENGTH);
	return true;
}

bool Commit_SetAuthor(Commit c, const char *authorName, const char *authorEmailId)
{
	String_format(c->author, "%s <%s>", authorName, authorEmailId);
	return true;
}

bool Commit_SetMessage(Commit c, const  char *message)
{
	String_strcpy(c->message, message);
	return true;
}

#define INT_SIZE	sizeof(uint32_t)
bool Commit_WriteCommitFile(Commit c, ShaBuffer commitSha)
{
	uint32_t fd, temp, dummy;
	bool returnValue = false;
	String s = String_Create();

	c->rawtime = time(NULL);
	String_format(s, "%s:%s:%s:%u", c->tree, c->parent0, c->parent1, (uint32_t)c->rawtime);

	sha_buffer((const unsigned char *)s_getstr(s), String_strlen(s), commitSha);
	String_format(s,"%s/%s", SCM_COMMIT_FOLDER, commitSha);
	if(true == isItFile(s_getstr(s)))
	{
		LOG_ERROR("fatal Commit_WriteCommitFile() commit '%s' already exist", commitSha);
		abort();
	}
	fd = open(s_getstr(s), O_CREAT | O_TRUNC | O_WRONLY, SCM_OBJECT_FILE_PERMISSION);
	if(fd < 0)
	{
		LOG_ERROR("Commit_WriteCommitFile() open('%s') failed(%d)", s_getstr(s), errno);
		goto EXIT;
	}

	/*Write the object identification Code..*/
	temp = htonl(OBJECT_COMMIT_FILE);
	dummy = write(fd, &temp, INT_SIZE);

	/*time */
	temp = htonl(c->rawtime);
	dummy = write(fd, &temp, INT_SIZE);

	/*size of tree SHA*/
	temp = htonl(strlen((char*)c->tree)+1);
	dummy = write(fd, &temp, INT_SIZE);
	dummy = write(fd, c->tree, ntohl(temp));

	/*size of parent0*/
	temp = htonl(strlen((char*)c->parent0)+1);
	dummy = write(fd, &temp, INT_SIZE);
	dummy = write(fd, c->parent0, ntohl(temp));

	/*size of parent1*/
	temp = htonl(strlen((char*)c->parent1)+1);
	dummy = write(fd, &temp, INT_SIZE);
	dummy = write(fd, c->parent1, ntohl(temp));

	/*length of author*/
	temp = htonl(String_strlen(c->author)+1);
	dummy = write(fd, &temp, INT_SIZE);
	dummy = write(fd, s_getstr(c->author), ntohl(temp));

	/*length of message*/
	temp = htonl(String_strlen(c->message)+1);
	dummy = write(fd, &temp, INT_SIZE);
	dummy = write(fd, s_getstr(c->message), ntohl(temp));
	returnValue = true;
	close(fd);
EXIT:
	String_Delete(s);
	return returnValue;
}

bool Commit_ReadCommitFile(Commit c, const ShaBuffer commitSha)
{
	String s = String_Create();
	bool returnValue = false;
	uint32_t fd, len = strlen((char*)commitSha), temp, dummy;
	if(len != SHA_HASH_LENGTH)
	{
		LOG_ERROR("Commit_ReadCommitFile() shaLength %d != %d", len, SHA_HASH_LENGTH);
		goto EXIT;
	}

	String_format(s,"%s/%s", SCM_COMMIT_FOLDER, commitSha);
	if(false == isItFile(s_getstr(s)))
	{
		LOG_ERROR("fatal Commit_ReadCommitFile() commit '%s' doesn't exist", commitSha);
		goto EXIT;
	}
	fd = open(s_getstr(s), O_RDONLY);


	/*read the object identification Code and check it*/
	dummy = read(fd, &temp, INT_SIZE);
	temp = ntohl(temp);
	if(temp != OBJECT_COMMIT_FILE)
	{
		LOG_ERROR("Commit_ReadCommitFile: '%s' not a commit file", commitSha);
		close(fd);
		goto EXIT;
	}

	/*read the time*/
	dummy = read(fd, &temp, INT_SIZE);
	c->rawtime = ntohl(temp);

	/*read the tree Sha*/
	dummy = read(fd, &temp, INT_SIZE);
	temp = ntohl(temp);
	dummy = read(fd, c->tree, temp);

	/*read parent0*/
	dummy = read(fd, &temp, INT_SIZE);
	temp = ntohl(temp);
	dummy = read(fd, c->parent0, temp);

	/*read parent1*/
	dummy = read(fd, &temp, INT_SIZE);
	temp = ntohl(temp);
	dummy = read(fd, c->parent1, temp);

	/*read the author name and EmailId*/
	dummy = read(fd, &temp, INT_SIZE);
	temp = ntohl(temp);
	String_SetSize(c->author,temp+10);
	dummy = read(fd, (void*)s_getstr(c->author), temp);

	/*read the message*/
	dummy = read(fd, &temp, INT_SIZE);
	temp = ntohl(temp);
	String_SetSize(c->message,temp+10);
	dummy = read(fd, (void*)s_getstr(c->message), temp);

	returnValue = true;
	close(fd);
EXIT:
	String_Delete(s);
	return returnValue;
}

