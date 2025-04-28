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

  Added functions to deal with the path stack for NXgetpath
  Mark Koennecke, October 2009

*/
#ifndef NEXUSFILESTACK
#define NEXUSFILESTACK

#include "MantidNexus/napi_internal.h"

typedef struct __fileStack *pFileStack;
#define MAXEXTERNALDEPTH 16

pFileStack makeFileStack();
void killFileStack(pFileStack self);

void pushFileStack(pFileStack self, pNexusFunction pDriv, const char *filename);
void popFileStack(pFileStack self);

pNexusFunction peekFileOnStack(pFileStack self);
char *peekFilenameOnStack(pFileStack self);
void peekIDOnStack(pFileStack self, NXlink *id);
void setCloseID(pFileStack self, const NXlink &id);

int fileStackDepth(pFileStack self);

void pushPath(pFileStack self, const char *name);
void popPath(pFileStack self);
int buildPath(pFileStack self, char *path, int pathlen);

#endif
