/**
 * This file contains functions necessary to perform XML-I/O for
 * NeXus with the mxml-library.
 *
 * Most notably it contains the callback function for reading data
 *
 *   Copyright (C) 2004 Mark Koennecke
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *  For further information, see <http://www.nexusformat.org>
 */

#ifndef __NXIO
#define __NXIO
#include <mxml.h>

#define TYPENAME "NAPItype"

#define DIMS_NODE_NAME "columns"
#define DATA_NODE_NAME "row"

mxml_type_t nexusTypeCallback(mxml_node_t *parent);
const char *NXwhitespaceCallback(mxml_node_t *node, int where);
int nexusLoadCallback(mxml_node_t *node, const char *buffer);
char *nexusWriteCallback(mxml_node_t *node);

void setNumberFormat(int dataType, char *formatString);
void initializeNumberFormats();
void getNumberText(int nx_type, char *typestring, int typeLen);
void destroyDataset(void *data);
int translateTypeCode(const char *code, const char *term);
int isDataNode(mxml_node_t *node);
void analyzeDim(const char *typeString, int *rank, int64_t *iDim, int *type);

#endif
