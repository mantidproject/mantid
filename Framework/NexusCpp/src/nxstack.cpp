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
#include "MantidNexusCpp/nxstack.h"
#include "MantidNexusCpp/napi.h"
#include "MantidNexusCpp/napi_internal.h"
#include <stdlib.h>
#include <string.h>

/*-----------------------------------------------------------------------
 Data definitions
---------------------------------------------------------------------*/

typedef struct {
  pNexusFunction pDriver;
  NXlink closeID;
  char filename[1024];
} fileStackEntry;

typedef struct __fileStack {
  int fileStackPointer;
  fileStackEntry fileStack[MAXEXTERNALDEPTH];
  int pathPointer;
  char pathStack[NXMAXSTACK][NX_MAXNAMELEN];
} fileStack;
/*---------------------------------------------------------------------*/
pFileStack makeFileStack() {
  pFileStack pNew = NULL;

  pNew = static_cast<pFileStack>(malloc(sizeof(fileStack)));
  if (pNew == NULL) {
    return NULL;
  }
  memset(pNew, 0, sizeof(fileStack));
  pNew->fileStackPointer = -1;
  pNew->pathPointer = -1;
  return pNew;
}
/*---------------------------------------------------------------------*/
void killFileStack(pFileStack self) {
  if (self != NULL) {
    free(self);
  }
}
/*---------------------------------------------------------------------*/
int getFileStackSize() { return sizeof(fileStack); }
/*----------------------------------------------------------------------*/
void pushFileStack(pFileStack self, pNexusFunction pDriv, const char *file) {
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
void popFileStack(pFileStack self) {
  self->fileStackPointer--;
  if (self->fileStackPointer < -1) {
    self->fileStackPointer = -1;
  }
}
/*----------------------------------------------------------------------*/
pNexusFunction peekFileOnStack(pFileStack self) { return self->fileStack[self->fileStackPointer].pDriver; }
/*---------------------------------------------------------------------*/
char *peekFilenameOnStack(pFileStack self) { return self->fileStack[self->fileStackPointer].filename; }
/*----------------------------------------------------------------------*/
void peekIDOnStack(pFileStack self, NXlink *id) {
  memcpy(id, &self->fileStack[self->fileStackPointer].closeID, sizeof(NXlink));
}
/*---------------------------------------------------------------------*/
void setCloseID(pFileStack self, const NXlink &id) {
  memcpy(&self->fileStack[self->fileStackPointer].closeID, &id, sizeof(NXlink));
}
/*----------------------------------------------------------------------*/
int fileStackDepth(pFileStack self) { return self->fileStackPointer; }
/*----------------------------------------------------------------------*/
void pushPath(pFileStack self, const char *name) {
  self->pathPointer++;
  strncpy(self->pathStack[self->pathPointer], name, NX_MAXNAMELEN - 1);
}
/*-----------------------------------------------------------------------*/
void popPath(pFileStack self) {
  self->pathPointer--;
  if (self->pathPointer < -1) {
    self->pathPointer = -1;
  }
}
/*-----------------------------------------------------------------------*/
int buildPath(pFileStack self, char *path, int pathlen) {
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
