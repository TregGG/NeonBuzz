/* SPDX-License-Identifier: Zlib
Copyright (c) 2014 - 2023 Guillaume Vareille http://zetcode.com
*/

#ifndef TINYFILEDIALOGS_H
#define TINYFILEDIALOGS_H

#ifdef	__cplusplus
extern "C" {
#endif

/* returns NULL on cancel */
char * tinyfd_openFileDialog(
    char const * aTitle,        /* NULL or "" */
    char const * aDefaultPathAndFile, /* NULL or "" */
    int aNumOfFilterPatterns,   /* 0 */
    char const * const * aFilterPatterns, /* NULL | {"*.jpg","*.png"} */
    char const * aSingleFilterDescription, /* NULL | "image files" */
    int aAllowMultipleSelects); /* 0 or 1 */

char * tinyfd_saveFileDialog(
    char const * aTitle,        /* NULL or "" */
    char const * aDefaultPathAndFile, /* NULL or "" */
    int aNumOfFilterPatterns,   /* 0 */
    char const * const * aFilterPatterns, /* NULL | {"*.jpg","*.png"} */
    char const * aSingleFilterDescription); /* NULL | "image files" */

char * tinyfd_selectFolderDialog(
    char const * aTitle,        /* NULL or "" */
    char const * aDefaultPath); /* NULL or "" */

/* returns 0 for cancel/no, 1 for ok/yes, 2 for no in yesnocancel */
int tinyfd_messageBox(
    char const * aTitle,        /* NULL or "" */
    char const * aMessage,      /* NULL or "" may contain \n \t */
    char const * aDialogType,   /* "ok" "okcancel" "yesno" "yesnocancel" */
    char const * aIconType,     /* "info" "warning" "error" "question" */
    int aDefaultButton);        /* 0 for cancel/no, 1 for ok/yes, 2 for no in yesnocancel */

#ifdef	__cplusplus
}
#endif

#endif /* TINYFILEDIALOGS_H */
