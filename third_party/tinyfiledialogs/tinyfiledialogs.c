/* SPDX-License-Identifier: Zlib
Copyright (c) 2014 - 2023 Guillaume Vareille
Simplified version for Linux using zenity/kdialog
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "tinyfiledialogs.h"

static char lBuff[4096];

static int detectPresence(char const * aExecutable) {
    char lTestedString[256];
    FILE *lIn;
    snprintf(lTestedString, sizeof(lTestedString), "which %s 2>/dev/null", aExecutable);
    lIn = popen(lTestedString, "r");
    if (!lIn) return 0;
    if (fgets(lBuff, sizeof(lBuff), lIn) == NULL) {
        pclose(lIn);
        return 0;
    }
    pclose(lIn);
    return (lBuff[0] != '\0');
}

static int zenityPresent(void) {
    static int lZenityPresent = -1;
    if (lZenityPresent < 0) {
        lZenityPresent = detectPresence("zenity");
    }
    return lZenityPresent;
}

static int kdialogPresent(void) {
    static int lKdialogPresent = -1;
    if (lKdialogPresent < 0) {
        lKdialogPresent = detectPresence("kdialog");
    }
    return lKdialogPresent;
}

char * tinyfd_openFileDialog(
    char const * aTitle,
    char const * aDefaultPathAndFile,
    int aNumOfFilterPatterns,
    char const * const * aFilterPatterns,
    char const * aSingleFilterDescription,
    int aAllowMultipleSelects)
{
    static char lDialogString[4096];
    static char lResultBuff[4096];
    FILE *lIn;
    char *p;
    int i;

    lResultBuff[0] = '\0';

    if (zenityPresent()) {
        strcpy(lDialogString, "zenity --file-selection");
        
        if (aTitle && strlen(aTitle)) {
            strcat(lDialogString, " --title=\"");
            strcat(lDialogString, aTitle);
            strcat(lDialogString, "\"");
        }
        
        if (aDefaultPathAndFile && strlen(aDefaultPathAndFile)) {
            strcat(lDialogString, " --filename=\"");
            strcat(lDialogString, aDefaultPathAndFile);
            strcat(lDialogString, "\"");
        }
        
        if (aNumOfFilterPatterns > 0 && aFilterPatterns) {
            strcat(lDialogString, " --file-filter=\"");
            if (aSingleFilterDescription && strlen(aSingleFilterDescription)) {
                strcat(lDialogString, aSingleFilterDescription);
                strcat(lDialogString, " |");
            }
            for (i = 0; i < aNumOfFilterPatterns; i++) {
                strcat(lDialogString, " ");
                strcat(lDialogString, aFilterPatterns[i]);
            }
            strcat(lDialogString, "\" --file-filter=\"All files | *\"");
        }
        
        if (aAllowMultipleSelects) {
            strcat(lDialogString, " --multiple --separator=\"|\"");
        }
        
        strcat(lDialogString, " 2>/dev/null");
    }
    else if (kdialogPresent()) {
        strcpy(lDialogString, "kdialog --getopenfilename");
        
        if (aDefaultPathAndFile && strlen(aDefaultPathAndFile)) {
            strcat(lDialogString, " \"");
            strcat(lDialogString, aDefaultPathAndFile);
            strcat(lDialogString, "\"");
        } else {
            strcat(lDialogString, " .");
        }
        
        if (aNumOfFilterPatterns > 0 && aFilterPatterns) {
            strcat(lDialogString, " \"");
            for (i = 0; i < aNumOfFilterPatterns; i++) {
                if (i > 0) strcat(lDialogString, " ");
                strcat(lDialogString, aFilterPatterns[i]);
            }
            if (aSingleFilterDescription && strlen(aSingleFilterDescription)) {
                strcat(lDialogString, " | ");
                strcat(lDialogString, aSingleFilterDescription);
            }
            strcat(lDialogString, "\"");
        }
        
        if (aTitle && strlen(aTitle)) {
            strcat(lDialogString, " --title \"");
            strcat(lDialogString, aTitle);
            strcat(lDialogString, "\"");
        }
        
        strcat(lDialogString, " 2>/dev/null");
    }
    else {
        return NULL;
    }

    lIn = popen(lDialogString, "r");
    if (!lIn) return NULL;
    
    if (fgets(lResultBuff, sizeof(lResultBuff), lIn) == NULL) {
        pclose(lIn);
        return NULL;
    }
    pclose(lIn);
    
    p = strchr(lResultBuff, '\n');
    if (p) *p = '\0';
    
    if (strlen(lResultBuff) == 0) return NULL;
    
    return lResultBuff;
}

char * tinyfd_saveFileDialog(
    char const * aTitle,
    char const * aDefaultPathAndFile,
    int aNumOfFilterPatterns,
    char const * const * aFilterPatterns,
    char const * aSingleFilterDescription)
{
    static char lDialogString[4096];
    static char lResultBuff[4096];
    FILE *lIn;
    char *p;
    int i;

    lResultBuff[0] = '\0';

    if (zenityPresent()) {
        strcpy(lDialogString, "zenity --file-selection --save --confirm-overwrite");
        
        if (aTitle && strlen(aTitle)) {
            strcat(lDialogString, " --title=\"");
            strcat(lDialogString, aTitle);
            strcat(lDialogString, "\"");
        }
        
        if (aDefaultPathAndFile && strlen(aDefaultPathAndFile)) {
            strcat(lDialogString, " --filename=\"");
            strcat(lDialogString, aDefaultPathAndFile);
            strcat(lDialogString, "\"");
        }
        
        if (aNumOfFilterPatterns > 0 && aFilterPatterns) {
            strcat(lDialogString, " --file-filter=\"");
            if (aSingleFilterDescription && strlen(aSingleFilterDescription)) {
                strcat(lDialogString, aSingleFilterDescription);
                strcat(lDialogString, " |");
            }
            for (i = 0; i < aNumOfFilterPatterns; i++) {
                strcat(lDialogString, " ");
                strcat(lDialogString, aFilterPatterns[i]);
            }
            strcat(lDialogString, "\"");
        }
        
        strcat(lDialogString, " 2>/dev/null");
    }
    else if (kdialogPresent()) {
        strcpy(lDialogString, "kdialog --getsavefilename");
        
        if (aDefaultPathAndFile && strlen(aDefaultPathAndFile)) {
            strcat(lDialogString, " \"");
            strcat(lDialogString, aDefaultPathAndFile);
            strcat(lDialogString, "\"");
        } else {
            strcat(lDialogString, " .");
        }
        
        if (aTitle && strlen(aTitle)) {
            strcat(lDialogString, " --title \"");
            strcat(lDialogString, aTitle);
            strcat(lDialogString, "\"");
        }
        
        strcat(lDialogString, " 2>/dev/null");
    }
    else {
        return NULL;
    }

    lIn = popen(lDialogString, "r");
    if (!lIn) return NULL;
    
    if (fgets(lResultBuff, sizeof(lResultBuff), lIn) == NULL) {
        pclose(lIn);
        return NULL;
    }
    pclose(lIn);
    
    p = strchr(lResultBuff, '\n');
    if (p) *p = '\0';
    
    if (strlen(lResultBuff) == 0) return NULL;
    
    return lResultBuff;
}

char * tinyfd_selectFolderDialog(
    char const * aTitle,
    char const * aDefaultPath)
{
    static char lDialogString[4096];
    static char lResultBuff[4096];
    FILE *lIn;
    char *p;

    lResultBuff[0] = '\0';

    if (zenityPresent()) {
        strcpy(lDialogString, "zenity --file-selection --directory");
        
        if (aTitle && strlen(aTitle)) {
            strcat(lDialogString, " --title=\"");
            strcat(lDialogString, aTitle);
            strcat(lDialogString, "\"");
        }
        
        if (aDefaultPath && strlen(aDefaultPath)) {
            strcat(lDialogString, " --filename=\"");
            strcat(lDialogString, aDefaultPath);
            strcat(lDialogString, "\"");
        }
        
        strcat(lDialogString, " 2>/dev/null");
    }
    else if (kdialogPresent()) {
        strcpy(lDialogString, "kdialog --getexistingdirectory");
        
        if (aDefaultPath && strlen(aDefaultPath)) {
            strcat(lDialogString, " \"");
            strcat(lDialogString, aDefaultPath);
            strcat(lDialogString, "\"");
        } else {
            strcat(lDialogString, " .");
        }
        
        if (aTitle && strlen(aTitle)) {
            strcat(lDialogString, " --title \"");
            strcat(lDialogString, aTitle);
            strcat(lDialogString, "\"");
        }
        
        strcat(lDialogString, " 2>/dev/null");
    }
    else {
        return NULL;
    }

    lIn = popen(lDialogString, "r");
    if (!lIn) return NULL;
    
    if (fgets(lResultBuff, sizeof(lResultBuff), lIn) == NULL) {
        pclose(lIn);
        return NULL;
    }
    pclose(lIn);
    
    p = strchr(lResultBuff, '\n');
    if (p) *p = '\0';
    
    if (strlen(lResultBuff) == 0) return NULL;
    
    return lResultBuff;
}

int tinyfd_messageBox(
    char const * aTitle,
    char const * aMessage,
    char const * aDialogType,
    char const * aIconType,
    int aDefaultButton)
{
    static char lDialogString[4096];
    FILE *lIn;
    int lResult = 0;

    if (zenityPresent()) {
        strcpy(lDialogString, "zenity");
        
        if (aDialogType && !strcmp(aDialogType, "okcancel")) {
            strcat(lDialogString, " --question --ok-label=OK --cancel-label=Cancel");
        } else if (aDialogType && !strcmp(aDialogType, "yesno")) {
            strcat(lDialogString, " --question");
        } else if (aDialogType && !strcmp(aDialogType, "yesnocancel")) {
            strcat(lDialogString, " --question");
        } else {
            if (aIconType && !strcmp(aIconType, "error")) {
                strcat(lDialogString, " --error");
            } else if (aIconType && !strcmp(aIconType, "warning")) {
                strcat(lDialogString, " --warning");
            } else {
                strcat(lDialogString, " --info");
            }
        }
        
        if (aTitle && strlen(aTitle)) {
            strcat(lDialogString, " --title=\"");
            strcat(lDialogString, aTitle);
            strcat(lDialogString, "\"");
        }
        
        if (aMessage && strlen(aMessage)) {
            strcat(lDialogString, " --text=\"");
            strcat(lDialogString, aMessage);
            strcat(lDialogString, "\"");
        }
        
        strcat(lDialogString, " 2>/dev/null; echo $?");
    }
    else {
        return aDefaultButton;
    }

    lIn = popen(lDialogString, "r");
    if (!lIn) return aDefaultButton;
    
    if (fgets(lBuff, sizeof(lBuff), lIn) != NULL) {
        lResult = (atoi(lBuff) == 0) ? 1 : 0;
    }
    pclose(lIn);
    
    return lResult;
}
