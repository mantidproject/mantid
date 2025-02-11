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
  pHDF4Function pDriver;
  NXlink closeID;
  char filename[1024];
} fileHDF4StackEntry;

typedef struct __fileHDF4Stack {
  int fileStackPointer;
  fileHDF4StackEntry fileStack[MAXEXTERNALDEPTH];
  int pathPointer;
  char pathStack[NXMAXSTACK][NX_MAXNAMELEN];
} fileHDF4Stack;

/*---------------------------------------------------------------------*/
pFileHDF4Stack makeFileStack() {
  pFileHDF4Stack pNew = NULL;

  pNew = static_cast<pFileHDF4Stack>(malloc(sizeof(fileHDF4Stack)));
  if (pNew == NULL) {
    return NULL;
  }
  memset(pNew, 0, sizeof(fileHDF4Stack));
  pNew->fileStackPointer = -1;
  pNew->pathPointer = -1;
  return pNew;
}
/*---------------------------------------------------------------------*/
void killFileStack(pFileHDF4Stack self) {
  if (self != NULL) {
    free(self);
  }
}
/*---------------------------------------------------------------------*/
int getFileStackSize() { return sizeof(fileHDF4Stack); }
/*----------------------------------------------------------------------*/
void pushFileStack(pFileHDF4Stack self, pHDF4Function pDriv, const char *file) {
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
void popFileStack(pFileHDF4Stack self) {
  self->fileStackPointer--;
  if (self->fileStackPointer < -1) {
    self->fileStackPointer = -1;
  }
}
/*----------------------------------------------------------------------*/
pHDF4Function peekFileOnStack(pFileHDF4Stack self) { return self->fileStack[self->fileStackPointer].pDriver; }
/*---------------------------------------------------------------------*/
char *peekFilenameOnStack(pFileHDF4Stack self) { return self->fileStack[self->fileStackPointer].filename; }
/*----------------------------------------------------------------------*/
void peekIDOnStack(pFileHDF4Stack self, NXlink *id) {
  memcpy(id, &self->fileStack[self->fileStackPointer].closeID, sizeof(NXlink));
}
/*---------------------------------------------------------------------*/
void setCloseID(pFileHDF4Stack self, const NXlink &id) {
  memcpy(&self->fileStack[self->fileStackPointer].closeID, &id, sizeof(NXlink));
}
/*----------------------------------------------------------------------*/
int fileStackDepth(pFileHDF4Stack self) { return self->fileStackPointer; }
/*----------------------------------------------------------------------*/
void pushPath(pFileHDF4Stack self, const char *name) {
  self->pathPointer++;
  strncpy(self->pathStack[self->pathPointer], name, NX_MAXNAMELEN - 1);
}
/*-----------------------------------------------------------------------*/
void popPath(pFileHDF4Stack self) {
  self->pathPointer--;
  if (self->pathPointer < -1) {
    self->pathPointer = -1;
  }
}
/*-----------------------------------------------------------------------*/
int buildPath(pFileHDF4Stack self, char *path, int pathlen) {
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
