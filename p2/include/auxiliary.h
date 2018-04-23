/*
 * OPERATING SYSTEMS DESING - 16/17
 *
 * @file 	auxiliary.h
 * @brief 	Headers for the auxiliary functions required by filesystem.c.
 * @date	01/03/2017
 */

int ialloc();
int ifree(int i);
int balloc();
int bfree(int b);
int namei(char *name);
int bmap(int i, int offset);
int myceil(double x);
uint32_t CRCheck(int type, int i);
