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
#include "MantidNexus/nxstack.h"
#include "MantidNexus/napi.h"
#include "MantidNexus/napi_internal.h"
#include <stdlib.h>
#include <string.h>

/*-----------------------------------------------------------------------
 Data definitions
---------------------------------------------------------------------*/

typedef struct {
  pNexusFunction pDriver;
  NXlink closeID;
  std::string filename;
} fileStackEntry;

typedef struct __fileStack {
  int fileStackPointer;
  std::vector<fileStackEntry> fileStack;
  int pathPointer;
  std::vector<std::string> pathStack;
} fileStack;
/*---------------------------------------------------------------------*/
pFileStack makeFileStack() {
  pFileStack pNew = new fileStack;
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
/*----------------------------------------------------------------------*/
void pushFileStack(pFileStack self, pNexusFunction pDriv, const char *file) {
  self->fileStackPointer++;
  self->fileStack.emplace_back(pDriv, NXlink{"", NXentrytype::group}, std::string(file));
}
/*----------------------------------------------------------------------*/
void popFileStack(pFileStack self) {
  self->fileStackPointer--;
  if (self->fileStackPointer < -1) {
    self->fileStackPointer = -1;
  }
  if (!self->fileStack.empty()) {
    self->fileStack.pop_back();
  }
}
/*----------------------------------------------------------------------*/
pNexusFunction peekFileOnStack(pFileStack self) { return self->fileStack[self->fileStackPointer].pDriver; }
/*---------------------------------------------------------------------*/
char *peekFilenameOnStack(pFileStack self) { return self->fileStack[self->fileStackPointer].filename.data(); }
/*----------------------------------------------------------------------*/
void peekIDOnStack(pFileStack self, NXlink *id) { *id = self->fileStack[self->fileStackPointer].closeID; }
/*---------------------------------------------------------------------*/
void setCloseID(pFileStack self, const NXlink &id) { self->fileStack[self->fileStackPointer].closeID = id; }
/*----------------------------------------------------------------------*/
int fileStackDepth(pFileStack self) { return self->fileStackPointer; }
/*----------------------------------------------------------------------*/
void pushPath(pFileStack self, const char *name) {
  if (self->pathPointer >= 0 && name == self->pathStack[self->pathPointer]) {
    return;
  }
  self->pathPointer++;
  self->pathStack.emplace_back(name);
}
/*-----------------------------------------------------------------------*/
void popPath(pFileStack self) {
  self->pathPointer--;
  if (self->pathPointer < -1) {
    self->pathPointer = -1;
  }
  if (!self->pathStack.empty()) {
    self->pathStack.pop_back();
  }
}
/*-----------------------------------------------------------------------*/
int buildPath(pFileStack self, char *path, int pathlen) {
  std::string totalPath;
  if (self->pathStack.empty()) {
    totalPath = "/";
  }
  for (std::string const &subpath : self->pathStack) {
    totalPath += "/" + subpath;
  }
  strncpy(path, totalPath.c_str(), pathlen - 1);

  return 1;
}
