/*
  This is some code to handle a stack of NeXus files. This is used to implement
  external linking within the NeXus-API

  Copyright (C) 1997-2006 Mark Koennecke

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

  For further information, see <http://www.nexusformat.org>

  Added code to support the path stack for NXgetpath,
        Mark Koennecke, October 2009
*/
#include "MantidLegacyNexus/nxstack.h"
#include "MantidLegacyNexus/napi.h"
#include "MantidLegacyNexus/napi_internal.h"
#include <stdlib.h>
#include <string.h>

/*-----------------------------------------------------------------------
 Data definitions
---------------------------------------------------------------------*/

typedef struct {
  pLgcyFunction pDriver;
  NXlink closeID;
  char filename[1024];
} fileLgcyStackEntry;

typedef struct __fileLgcyStack {
  int fileStackPointer;
  fileLgcyStackEntry fileStack[MAXEXTERNALDEPTH];
  int pathPointer;
  char pathStack[NXMAXSTACK][NX_MAXNAMELEN];
} fileLgcyStack;

/*---------------------------------------------------------------------*/
pFileLgcyStack makeFileStack() {
  pFileLgcyStack pNew = NULL;

  pNew = static_cast<pFileLgcyStack>(malloc(sizeof(fileLgcyStack)));
  if (pNew == NULL) {
    return NULL;
  }
  memset(pNew, 0, sizeof(fileLgcyStack));
  pNew->fileStackPointer = -1;
  pNew->pathPointer = -1;
  return pNew;
}
/*---------------------------------------------------------------------*/
void killFileStack(pFileLgcyStack self) {
  if (self != NULL) {
    free(self);
  }
}
/*---------------------------------------------------------------------*/
int getFileStackSize() { return sizeof(fileLgcyStack); }
/*----------------------------------------------------------------------*/
void pushFileStack(pFileLgcyStack self, pLgcyFunction pDriv, const char *file) {
  size_t length;

  self->fileStackPointer++;
  self->fileStack[self->fileStackPointer].pDriver = pDriv;
  memset(&self->fileStack[self->fileStackPointer].closeID, 0, sizeof(NXlink));
  length = strlen(file);
  if (length >= 1024) {
    length = 1023;
  }
  memcpy(&self->fileStack[self->fileStackPointer].filename, file, length);
}
/*----------------------------------------------------------------------*/
void popFileStack(pFileLgcyStack self) {
  self->fileStackPointer--;
  if (self->fileStackPointer < -1) {
    self->fileStackPointer = -1;
  }
}
/*----------------------------------------------------------------------*/
pLgcyFunction peekFileOnStack(pFileLgcyStack self) { return self->fileStack[self->fileStackPointer].pDriver; }
/*---------------------------------------------------------------------*/
char *peekFilenameOnStack(pFileLgcyStack self) { return self->fileStack[self->fileStackPointer].filename; }
/*----------------------------------------------------------------------*/
void peekIDOnStack(pFileLgcyStack self, NXlink *id) {
  memcpy(id, &self->fileStack[self->fileStackPointer].closeID, sizeof(NXlink));
}
/*---------------------------------------------------------------------*/
void setCloseID(pFileLgcyStack self, const NXlink &id) {
  memcpy(&self->fileStack[self->fileStackPointer].closeID, &id, sizeof(NXlink));
}
/*----------------------------------------------------------------------*/
int fileStackDepth(pFileLgcyStack self) { return self->fileStackPointer; }
/*----------------------------------------------------------------------*/
void pushPath(pFileLgcyStack self, const char *name) {
  self->pathPointer++;
  strncpy(self->pathStack[self->pathPointer], name, NX_MAXNAMELEN - 1);
}
/*-----------------------------------------------------------------------*/
void popPath(pFileLgcyStack self) {
  self->pathPointer--;
  if (self->pathPointer < -1) {
    self->pathPointer = -1;
  }
}
/*-----------------------------------------------------------------------*/
int buildPath(pFileLgcyStack self, char *path, int pathlen) {
  int i;
  size_t totalPathLength;
  char *totalPath;

  for (i = 0, totalPathLength = 5; i <= self->pathPointer; i++) {
    totalPathLength += strlen(self->pathStack[i]) + 1;
  }
  totalPath = static_cast<char *>(malloc(totalPathLength * sizeof(char)));
  if (totalPath == NULL) {
    return 0;
  }
  memset(totalPath, 0, totalPathLength * sizeof(char));
  for (i = 0; i <= self->pathPointer; i++) {
    strcat(totalPath, "/");
    strcat(totalPath, self->pathStack[i]);
  }

  strncpy(path, totalPath, pathlen - 1);
  free(totalPath);
  return 1;
}
