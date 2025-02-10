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
#pragma once

// get nxlink
#include "MantidLegacyNexus/NeXusFile_fwd.h"
#include "MantidLegacyNexus/napi_internal.h"

typedef struct __fileHDF4Stack *pFileHDF4Stack;

#define MAXEXTERNALDEPTH 16

pFileHDF4Stack makeFileStack();
void killFileStack(pFileHDF4Stack self);
int getFileStackSize();

void pushFileStack(pFileHDF4Stack self, pHDF4Function pDriv, const char *filename);
void popFileStack(pFileHDF4Stack self);

pHDF4Function peekFileOnStack(pFileHDF4Stack self);
char *peekFilenameOnStack(pFileHDF4Stack self);
void peekIDOnStack(pFileHDF4Stack self, NXlink *id);
void setCloseID(pFileHDF4Stack self, const NXlink &id);

int fileStackDepth(pFileHDF4Stack self);

void pushPath(pFileHDF4Stack self, const char *name);
void popPath(pFileHDF4Stack self);
int buildPath(pFileHDF4Stack self, char *path, int pathlen);
