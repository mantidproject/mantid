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

  Added code to support the address stack for NXgetaddress,
        Mark Koennecke, October 2009
*/
#include "MantidNexus/nxstack.h"
#include "MantidNexus/napi.h"
#include "MantidNexus/napi_internal.h"
#include <stdlib.h>
#include <string.h>

/*----------------------------------------------------------------------*/
void nxstack::resetValues(pNexusFunction pDriv, std::string const &file) {
  this->pDriver = pDriv;
  this->filename = file;
}
/*----------------------------------------------------------------------*/
