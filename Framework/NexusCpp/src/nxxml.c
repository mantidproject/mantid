/*
 * This is the implementation file for the XML file driver
 * for NeXus
 *
 *   Copyright (C) 2006 Mark Koennecke
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

#include "MantidNexusCpp/nxconfig.h"

#ifdef WITH_MXML

#include "MantidNexusCpp/napi.h"
#include "MantidNexusCpp/napi_internal.h"
#include "MantidNexusCpp/nxdataset.h"
#include "MantidNexusCpp/nxio.h"
#include "MantidNexusCpp/nxxml.h"
#include <assert.h>
#include <mxml.h>
#include <stdint.h>
#include <stdio.h>

#ifdef _MSC_VER
#define snprintf _snprintf
#endif /* _MSC_VER */

extern void *NXpData;
extern int validNXName(const char *name, int allow_colon); /* from napi.c */
char *nxitrim(char *str);                                  /* from napi.c */

/*----------------------- our data structures --------------------------
  One might wonder why a node stack is still needed even if this API
  operates on top of a tree API. The reason for this are the links.
  Following a link on any NXopenpath, data means a jump through the
  whole tree. In order to correctly return from such adventures,
  a stack is needed. Moreover we need it in order to keep track of the
  state of search operations.

  The true NXroot node is always at stack[0]. The root in the data
  structure is the ?xml element. The latter one is needed to store
  the tree.
-----------------------------------------------------------------------*/
typedef struct {
  mxml_node_t *current;
  mxml_node_t *currentChild;
  int currentAttribute;
  int options; /**< additional information about the node */
} xmlStack;

/*
 * Freddie Akeroyd, 19/03/2008
 *
 * Add in support for table style data writing - this is
 * indicated internally via the XMLSTACK_OPTION_TABLE flag
 * and separates the dimensions and data into separate nodes contained
 * in DIMS_NODE_NAME and DATA_NODE_NAME. This is a first commit and
 * involves some code duplication that will need to be cleaned up later.
 * Also writing in table style is only enabled for 1D arrays as
 * I haven't done slab writing yet which the nexus test program uses
 * for writing 2D arrays.
 *
 * Table output is enabled by opening a file with (NXACC_CREATEXML | NXACC_TABLE)
 *
 * See http://trac.nexusformat.org/code/ticket/111 for further details
 */
#define XMLSTACK_OPTION_TABLE 0x1 /**< indicates table option in xmlStack */

/*---------------------------------------------------------------------*/
typedef struct {
  mxml_node_t *root;          /* root node */
  int readOnly;               /* read only flag */
  int tableStyle;             /**< whether to output data in XML table style */
  int stackPointer;           /* stack pointer */
  char filename[1024];        /* file name, for NXflush, NXclose */
  xmlStack stack[NXMAXSTACK]; /* stack */
} XMLNexus, *pXMLNexus;
/*===================== support functions ===============================*/
extern char *stptok(char *s, char *tok, size_t toklen, char *brk);
/*----------------------------------------------------------------------*/
static mxml_node_t *getLinkTarget(pXMLNexus xmlHandle, const char *target) {
  mxml_node_t *node = NULL;
  mxml_node_t *testNode = NULL;
  char path[132], *pPtr;

  pPtr = (char *)target + 1;
  node = xmlHandle->stack[0].current;
  while ((pPtr = stptok(pPtr, path, 131, "/")) != NULL) {
    /*
      search for group node
    */
    testNode = mxmlFindElement(node, node, NULL, "name", path, MXML_DESCEND_FIRST);
    if (testNode == NULL) {
      /*
        it can still be a data node
      */
      testNode = mxmlFindElement(node, node, path, NULL, NULL, MXML_DESCEND_FIRST);
    }
    if (testNode == NULL) {
      NXReportError("Cannot follow broken link");
      return NULL;
    } else {
      node = testNode;
    }
  }
  return node;
}
/*==================== file functions ===================================*/
static void errorCallbackForMxml(const char *txt) { NXReportError((char *)txt); }
/*-----------------------------------------------------------------------*/
NXstatus NXXopen(CONSTCHAR *filename, NXaccess am, NXhandle *pHandle) {
  pXMLNexus xmlHandle = NULL;
  FILE *fp = NULL;
  char *time_buffer = NULL;
  mxml_node_t *current;

  /*
    allocate data
  */
  xmlHandle = (pXMLNexus)malloc(sizeof(XMLNexus));
  if (!xmlHandle) {
    NXReportError("Out of memory allocating XML file handle");
    return NX_ERROR;
  }
  memset(xmlHandle, 0, sizeof(XMLNexus));

  /*
    initialize mxml XML parser
  */
  mxmlSetCustomHandlers(nexusLoadCallback, nexusWriteCallback);
  initializeNumberFormats();
  mxmlSetErrorCallback(errorCallbackForMxml);

  xmlHandle->tableStyle = ((am & NXACC_TABLE) ? 1 : 0);
  /*
    open file
  */
  strncpy(xmlHandle->filename, filename, 1023);
  switch (am & NXACCMASK_REMOVEFLAGS) {
  case NXACC_READ:
    xmlHandle->readOnly = 1;
  case NXACC_RDWR:
    fp = fopen(filename, "r");
    if (fp == NULL) {
      NXReportError("Failed to open file:");
      NXReportError((char *)filename);
      free(xmlHandle);
      return NX_ERROR;
    }
    xmlHandle->root = mxmlLoadFile(NULL, fp, nexusTypeCallback);
    xmlHandle->stack[0].current = mxmlFindElement(xmlHandle->root, xmlHandle->root, "NXroot", NULL, NULL, MXML_DESCEND);
    xmlHandle->stack[0].currentChild = NULL;
    xmlHandle->stack[0].currentAttribute = 0;
    xmlHandle->stack[0].options = 0;
    fclose(fp);
    break;
  case NXACC_CREATEXML:
    xmlHandle->root = mxmlNewElement(NULL, "?xml version=\"1.0\" encoding=\"UTF-8\"?");
    current = mxmlNewElement(xmlHandle->root, "NXroot");
    mxmlElementSetAttr(current, "NeXus_version", NEXUS_VERSION);
    mxmlElementSetAttr(current, "XML_version", "mxml");
    mxmlElementSetAttr(current, "NX_class", "NXroot");
    mxmlElementSetAttr(current, "file_name", filename);
    mxmlElementSetAttr(current, "xmlns", NEXUS_SCHEMA_NAMESPACE);
    mxmlElementSetAttr(current, "xmlns:xsi", "http://www.w3.org/2001/XMLSchema-instance");
    mxmlElementSetAttr(current, "xsi:schemaLocation", NEXUS_SCHEMA_NAMESPACE " " NEXUS_SCHEMA_URL);
    time_buffer = NXIformatNeXusTime();
    if (time_buffer != NULL) {
      mxmlElementSetAttr(current, "file_time", time_buffer);
      free(time_buffer);
    }
    xmlHandle->stack[0].current = current;
    xmlHandle->stack[0].currentChild = NULL;
    xmlHandle->stack[0].currentAttribute = 0;
    xmlHandle->stack[0].options = 0;
    break;
  default:
    NXReportError("Bad access parameter specified in NXXopen");
    return NX_ERROR;
  }
  if (xmlHandle->stack[0].current == NULL) {
    NXReportError("No NXroot element in XML-file, no NeXus-XML file");
    return NX_ERROR;
  }

  *pHandle = xmlHandle;
  return NX_OK;
}
/*----------------------------------------------------------------------*/
NXstatus NXXclose(NXhandle *fid) {
  pXMLNexus xmlHandle = NULL;
  FILE *fp = NULL;

  xmlHandle = (pXMLNexus)*fid;
  assert(xmlHandle);

  if (xmlHandle->readOnly == 0) {
    fp = fopen(xmlHandle->filename, "w");
    if (fp == NULL) {
      NXReportError("Failed to open NeXus XML file for writing");
      return NX_ERROR;
    }
    mxmlSaveFile(xmlHandle->root, fp, NXwhitespaceCallback);
    fclose(fp);
  }
  mxmlDelete(xmlHandle->root);
  free(xmlHandle);
  *fid = NULL;
  return NX_OK;
}
/*----------------------------------------------------------------------*/
NXstatus NXXflush(NXhandle *fid) {
  pXMLNexus xmlHandle = NULL;
  FILE *fp = NULL;

  xmlHandle = (pXMLNexus)*fid;
  assert(xmlHandle);

  if (xmlHandle->readOnly == 0) {
    fp = fopen(xmlHandle->filename, "w");
    if (fp == NULL) {
      NXReportError("Failed to open NeXus XML file for writing");
      return NX_ERROR;
    }
    mxmlSaveFile(xmlHandle->root, fp, NXwhitespaceCallback);
    fclose(fp);
  }
  return NX_OK;
}
/*=======================================================================
                   Group functions
=========================================================================*/
NXstatus NXXmakegroup(NXhandle fid, CONSTCHAR *name, CONSTCHAR *nxclass) {
  char buffer[256];
  pXMLNexus xmlHandle = NULL;
  mxml_node_t *newGroup = NULL;

  xmlHandle = (pXMLNexus)fid;
  assert(xmlHandle);

  if (!validNXName(name, 0)) {
    sprintf(buffer, "ERROR: invalid characters in group name \"%s\"", name);
    NXReportError(buffer);
    return NX_ERROR;
  }
  if (strlen(nxclass) == 0) /* xml node must have a name */
  {
    nxclass = "NXcollection";
  }

  if (isDataNode(xmlHandle->stack[xmlHandle->stackPointer].current)) {
    NXReportError("Close dataset before trying to create a group");
    return NX_ERROR;
  }

  newGroup = mxmlNewElement(xmlHandle->stack[xmlHandle->stackPointer].current, nxclass);
  if (!newGroup) {
    NXReportError("failed to allocate new group");
    return NX_ERROR;
  }
  mxmlElementSetAttr(newGroup, "name", name);
  return NX_OK;
}
/*----------------------------------------------------------------------*/
static mxml_node_t *searchGroupLinks(pXMLNexus xmlHandle, CONSTCHAR *name, CONSTCHAR *nxclass) {
  mxml_node_t *linkNode = NULL;
  mxml_node_t *current;
  mxml_node_t *test = NULL;
  const char *linkTarget;
  const char *linkName = NULL;

  current = xmlHandle->stack[xmlHandle->stackPointer].current;
  linkNode = current;
  while ((linkNode = mxmlFindElement(linkNode, current, "NAPIlink", NULL, NULL, MXML_DESCEND_FIRST)) != NULL) {
    linkTarget = mxmlElementGetAttr(linkNode, "target");
    test = getLinkTarget(xmlHandle, linkTarget);
    if (test != NULL) {
      if (strcmp(test->value.element.name, nxclass) == 0) {
        if (strcmp(mxmlElementGetAttr(test, "name"), name) == 0) {
          return test;
        }
      }
    }
    /*
      test for named links
    */
    linkName = mxmlElementGetAttr(linkNode, "name");
    if (test != NULL && linkName != NULL) {
      if (strcmp(test->value.element.name, nxclass) == 0) {
        if (strcmp(linkName, name) == 0) {
          return test;
        }
      }
    }
  }
  return NULL;
}
/*------------------------------------------------------------------------*/
NXstatus NXXopengroup(NXhandle fid, CONSTCHAR *name, CONSTCHAR *nxclass) {
  pXMLNexus xmlHandle = NULL;
  mxml_node_t *newGroup = NULL;
  char error[1024];

  xmlHandle = (pXMLNexus)fid;
  assert(xmlHandle);

  if (isDataNode(xmlHandle->stack[xmlHandle->stackPointer].current)) {
    NXReportError("Close dataset before trying to open a group");
    return NX_ERROR;
  }
  if (strlen(nxclass) == 0) /* xml node must have a name */
  {
    nxclass = "NXcollection";
  }
  newGroup =
      mxmlFindElement(xmlHandle->stack[xmlHandle->stackPointer].current,
                      xmlHandle->stack[xmlHandle->stackPointer].current, nxclass, "name", name, MXML_DESCEND_FIRST);
  if (newGroup == NULL) {
    newGroup = searchGroupLinks(xmlHandle, name, nxclass);
  }
  if (!newGroup) {
    snprintf(error, 1023, "Failed to open %s, %s", name, nxclass);
    NXReportError(error);
    return NX_ERROR;
  }
  xmlHandle->stackPointer++;
  xmlHandle->stack[xmlHandle->stackPointer].current = newGroup;
  xmlHandle->stack[xmlHandle->stackPointer].currentChild = NULL;
  xmlHandle->stack[xmlHandle->stackPointer].currentAttribute = 0;
  xmlHandle->stack[xmlHandle->stackPointer].options = 0;
  return NX_OK;
}
/*----------------------------------------------------------------------*/
NXstatus NXXclosegroup(NXhandle fid) {
  pXMLNexus xmlHandle = NULL;

  xmlHandle = (pXMLNexus)fid;
  assert(xmlHandle);

  if (isDataNode(xmlHandle->stack[xmlHandle->stackPointer].current)) {
    /*
      silently fix this
    */
    NXXclosedata(fid);
  }
  if (xmlHandle->stackPointer > 0) {
    xmlHandle->stackPointer--;
  }
  return NX_OK;
}
/*=========================================================================
         dataset functions
=========================================================================*/
NXstatus NXXcompmakedata64(NXhandle fid, CONSTCHAR *name, int datatype, int rank, int64_t dimensions[],
                           int compress_type, int64_t chunk_size[]) {
  /*
    compression does not relly make sense with XML
  */
  return NXXmakedata64(fid, name, datatype, rank, dimensions);
}
/*-----------------------------------------------------------------------*/
static char *buildTypeString(int datatype, int rank, int64_t dimensions[]) {
  char *typestring = NULL;
  char pNumber[20];
  int i;

  /*
    allocate data
  */
  typestring = (char *)malloc(132 * sizeof(char));
  if (!typestring) {
    NXReportError("Failed to allocate typestring");
    return NULL;
  }
  memset(typestring, 0, 132 * sizeof(char));

  getNumberText(datatype, typestring, 130);
  if (rank > 1 || datatype == NX_CHAR || dimensions[0] > 1) {
    strcat(typestring, "[");
    snprintf(pNumber, 19, "%lld", (long long)dimensions[0]);
    strncat(typestring, pNumber, 130 - strlen(typestring));
    for (i = 1; i < rank; i++) {
      snprintf(pNumber, 19, ",%lld", (long long)dimensions[i]);
      strncat(typestring, pNumber, 130 - strlen(typestring));
    }
    strcat(typestring, "]");
  }
  return typestring;
}

/*------------------------------------------------------------------------*/
NXstatus NXXmakedatatable64(NXhandle fid, CONSTCHAR *name, int datatype, int rank, int64_t dimensions[]) {
  pXMLNexus xmlHandle = NULL;
  mxml_node_t *dataNode = NULL, *dataNodeRoot = NULL, *dimsNode = NULL, *dimsNodeRoot = NULL;
  mxml_node_t *newData = NULL;
  mxml_node_t *current;
  char *typestring;
  int i, ndata;
  char buffer[256];
  static int64_t one = 1;

  xmlHandle = (pXMLNexus)fid;
  assert(xmlHandle);
  if (!validNXName(name, 0)) {
    sprintf(buffer, "ERROR: invalid characters in dataset name \"%s\"", name);
    NXReportError(buffer);
    return NX_ERROR;
  }

  if (isDataNode(xmlHandle->stack[xmlHandle->stackPointer].current)) {
    NXReportError("Close dataset before trying to create a dataset");
    return NX_ERROR;
  }
  if (dimensions[0] < 0) {
    dimensions[0] = 1;
  }

  current = xmlHandle->stack[xmlHandle->stackPointer].current;

  dimsNodeRoot = mxmlFindElement(current, current, DIMS_NODE_NAME, NULL, NULL, MXML_DESCEND_FIRST);
  if (dimsNodeRoot == NULL) {
    dimsNodeRoot = mxmlNewElement(current, DIMS_NODE_NAME);
  }
  dimsNode = mxmlNewElement(dimsNodeRoot, name);
  mxmlNewOpaque(dimsNode, "");
  typestring = buildTypeString(datatype, rank, dimensions);
  if (typestring != NULL) {
    mxmlElementSetAttr(dimsNode, TYPENAME, typestring);
    free(typestring);
  } else {
    NXReportError("Failed to allocate typestring");
    return NX_ERROR;
  }
  ndata = 1;
  for (i = 0; i < rank; i++) {
    ndata *= dimensions[i];
  }
  dataNodeRoot = current;
  for (i = 0; i < ndata; i++) {
    dataNodeRoot = mxmlFindElement(dataNodeRoot, current, DATA_NODE_NAME, NULL, NULL,
                                   (i == 0 ? MXML_DESCEND_FIRST : MXML_NO_DESCEND));
    if (dataNodeRoot == NULL) {
      dataNodeRoot = mxmlNewElement(current, DATA_NODE_NAME);
    }
    dataNode = mxmlNewElement(dataNodeRoot, name);
    newData = (mxml_node_t *)malloc(sizeof(mxml_node_t));
    if (!newData) {
      NXReportError("Failed to allocate space for dataset");
      return NX_ERROR;
    }
    memset(newData, 0, sizeof(mxml_node_t));
    mxmlAdd(dataNode, MXML_ADD_AFTER, MXML_ADD_TO_PARENT, newData);
    newData->type = MXML_CUSTOM;
    /*        newData->value.custom.data = createNXDataset(rank,datatype,dimensions); */
    newData->value.custom.data = createNXDataset(1, datatype, &one);
    if (!newData->value.custom.data) {
      NXReportError("Failed to allocate space for dataset");
      return NX_ERROR;
    }
    newData->value.custom.destroy = destroyDataset;
  }
  return NX_OK;
}

NXstatus NXXmakedata64(NXhandle fid, CONSTCHAR *name, int datatype, int rank, int64_t dimensions[]) {
  pXMLNexus xmlHandle = NULL;
  mxml_node_t *dataNode = NULL;
  mxml_node_t *newData = NULL;
  mxml_node_t *current;
  char *typestring;
  char buffer[256];

  xmlHandle = (pXMLNexus)fid;
  assert(xmlHandle);
  if (!validNXName(name, 0)) {
    sprintf(buffer, "ERROR: invalid characters in dataset name \"%s\"", name);
    NXReportError(buffer);
    return NX_ERROR;
  }

  if (xmlHandle->tableStyle && datatype != NX_CHAR && dimensions[0] != NX_UNLIMITED && rank == 1) {
    return NXXmakedatatable64(fid, name, datatype, rank, dimensions);
  }

  if (isDataNode(xmlHandle->stack[xmlHandle->stackPointer].current)) {
    NXReportError("Close dataset before trying to create a dataset");
    return NX_ERROR;
  }
  if (dimensions[0] < 0) {
    dimensions[0] = 1;
  }

  current = xmlHandle->stack[xmlHandle->stackPointer].current;
  dataNode = mxmlNewElement(current, name);
  typestring = buildTypeString(datatype, rank, dimensions);
  if (typestring != NULL) {
    mxmlElementSetAttr(dataNode, TYPENAME, typestring);
    free(typestring);
  } else {
    NXReportError("Failed to allocate typestring");
    return NX_ERROR;
  }
  /*
    NX_CHAR maps to MXML_OPAQUE datasets
  */
  if (datatype == NX_CHAR) {
    newData = mxmlNewOpaque(dataNode, "");
    return NX_OK;
  } else {
    newData = (mxml_node_t *)malloc(sizeof(mxml_node_t));
    if (!newData) {
      NXReportError("Failed to allocate space for dataset");
      return NX_ERROR;
    }
    memset(newData, 0, sizeof(mxml_node_t));
    mxmlAdd(dataNode, MXML_ADD_AFTER, MXML_ADD_TO_PARENT, newData);
    newData->type = MXML_CUSTOM;
    newData->value.custom.data = createNXDataset(rank, datatype, dimensions);
    if (!newData->value.custom.data) {
      NXReportError("Failed to allocate space for dataset");
      return NX_ERROR;
    }
    newData->value.custom.destroy = destroyDataset;
  }
  return NX_OK;
}
/*----------------------------------------------------------------------*/
static mxml_node_t *searchSDSLinks(pXMLNexus xmlHandle, CONSTCHAR *name) {
  mxml_node_t *linkNode = NULL;
  mxml_node_t *current;
  mxml_node_t *test = NULL;
  const char *linkTarget;
  const char *linkName = NULL;

  current = xmlHandle->stack[xmlHandle->stackPointer].current;
  linkNode = current;
  while ((linkNode = mxmlFindElement(linkNode, current, "NAPIlink", NULL, NULL, MXML_DESCEND_FIRST)) != NULL) {
    linkTarget = mxmlElementGetAttr(linkNode, "target");
    test = getLinkTarget(xmlHandle, linkTarget);
    if (test != NULL) {
      if (strcmp(test->value.element.name, name) == 0) {
        return test;
      }
    }
    /*
      test for named links
    */
    linkName = mxmlElementGetAttr(linkNode, "name");
    if (test != NULL && linkName != NULL) {
      if (strcmp(linkName, name) == 0) {
        return test;
      }
    }
  }
  return NULL;
}
/*-----------------------------------------------------------------------*/
NXstatus NXXopendatatable(NXhandle fid, CONSTCHAR *name) {
  pXMLNexus xmlHandle = NULL;
  mxml_node_t *dataNode = NULL, *dimsNode = NULL;
  char error[1024];

  xmlHandle = (pXMLNexus)fid;
  assert(xmlHandle);

  if (isDataNode(xmlHandle->stack[xmlHandle->stackPointer].current)) {
    /*
      silently fix this
    */
    xmlHandle->stackPointer--;
    if (xmlHandle->stackPointer < 0) {
      xmlHandle->stackPointer = 0;
    }
  }

  dimsNode = mxmlFindElement(xmlHandle->stack[xmlHandle->stackPointer].current,
                             xmlHandle->stack[xmlHandle->stackPointer].current, DIMS_NODE_NAME, NULL, NULL,
                             MXML_DESCEND_FIRST);

  if (!dimsNode) {
    snprintf(error, 1023, "Failed to open dataset %s", name);
    NXReportError(error);
    return NX_ERROR;
  }

  dataNode = mxmlFindElement(dimsNode, dimsNode, name, NULL, NULL, MXML_DESCEND_FIRST);
  if (dataNode == NULL) {
    dataNode = searchSDSLinks(xmlHandle, name);
  }
  if (!dataNode) {
    snprintf(error, 1023, "Failed to open dataset %s", name);
    NXReportError(error);
    return NX_ERROR;
  }
  xmlHandle->stackPointer++;
  xmlHandle->stack[xmlHandle->stackPointer].current = dataNode;
  xmlHandle->stack[xmlHandle->stackPointer].currentChild = NULL;
  xmlHandle->stack[xmlHandle->stackPointer].currentAttribute = 0;
  xmlHandle->stack[xmlHandle->stackPointer].options = XMLSTACK_OPTION_TABLE;
  return NX_OK;
}

NXstatus NXXopendata(NXhandle fid, CONSTCHAR *name) {
  pXMLNexus xmlHandle = NULL;
  mxml_node_t *dataNode = NULL, *current = NULL;
  char error[1024];

  xmlHandle = (pXMLNexus)fid;
  assert(xmlHandle);

  /* is this a table style node ? */
  current = xmlHandle->stack[xmlHandle->stackPointer].current;
  dataNode = mxmlFindElement(current, current, DATA_NODE_NAME, NULL, NULL, MXML_DESCEND_FIRST);
  if (dataNode != NULL) {
    dataNode = mxmlFindElement(dataNode, dataNode, name, NULL, NULL, MXML_DESCEND_FIRST);
  }
  if (dataNode != NULL) {
    return NXXopendatatable(fid, name);
  }

  if (isDataNode(xmlHandle->stack[xmlHandle->stackPointer].current)) {
    /*
      silently fix this
    */
    xmlHandle->stackPointer--;
    if (xmlHandle->stackPointer < 0) {
      xmlHandle->stackPointer = 0;
    }
  }

  dataNode = mxmlFindElement(xmlHandle->stack[xmlHandle->stackPointer].current,
                             xmlHandle->stack[xmlHandle->stackPointer].current, name, NULL, NULL, MXML_DESCEND_FIRST);
  if (dataNode == NULL) {
    dataNode = searchSDSLinks(xmlHandle, name);
  }
  if (!dataNode) {
    snprintf(error, 1023, "Failed to open dataset %s", name);
    NXReportError(error);
    return NX_ERROR;
  }
  xmlHandle->stackPointer++;
  xmlHandle->stack[xmlHandle->stackPointer].current = dataNode;
  xmlHandle->stack[xmlHandle->stackPointer].currentChild = NULL;
  xmlHandle->stack[xmlHandle->stackPointer].currentAttribute = 0;
  xmlHandle->stack[xmlHandle->stackPointer].options = 0;
  return NX_OK;
}
/*----------------------------------------------------------------------*/

NXstatus NXXclosedata(NXhandle fid) {
  pXMLNexus xmlHandle = NULL;

  xmlHandle = (pXMLNexus)fid;
  assert(xmlHandle);

  if (isDataNode(xmlHandle->stack[xmlHandle->stackPointer].current)) {
    if (xmlHandle->stackPointer > 0) {
      xmlHandle->stackPointer--;
    }
    return NX_OK;
  }
  return NX_OK;
}
/*----------------------------------------------------------------------*/
static mxml_node_t *findData(mxml_node_t *node) {
  mxml_node_t *baby = node;

  while ((baby = mxmlWalkNext(baby, node, MXML_DESCEND_FIRST)) != NULL) {
    if (baby->type == MXML_OPAQUE || baby->type == MXML_CUSTOM) {
      return baby;
    }
  }
  return NULL;
}

/* we only havv to deal with non-character data here */
NXstatus NXXputdatatable(NXhandle fid, const void *data) {
  pXMLNexus xmlHandle = NULL;
  mxml_node_t *userData = NULL;
  mxml_node_t *current = NULL;
  mxml_node_t *nodeRoot = NULL;
  mxml_node_t *dataNodeRoot = NULL;
  mxml_node_t *dataNode = NULL;
  const char *name;
  pNXDS dataset;
  int i, offset, length;
  xmlHandle = (pXMLNexus)fid;
  assert(xmlHandle);
  /* current points at the Idims node as done in NXXopendatatable */
  current = xmlHandle->stack[xmlHandle->stackPointer].current;
  name = current->value.element.name;
  /* we want to walk all Idata nodes and set name */
  nodeRoot = current->parent->parent;
  dataNodeRoot = nodeRoot;
  offset = 0;
  for (i = 0; dataNodeRoot != NULL; i++) {
    dataNodeRoot = mxmlFindElement(dataNodeRoot, nodeRoot, DATA_NODE_NAME, NULL, NULL,
                                   (i == 0 ? MXML_DESCEND_FIRST : MXML_NO_DESCEND));
    if (dataNodeRoot != NULL) {
      dataNode = mxmlFindElement(dataNodeRoot, dataNodeRoot, name, NULL, NULL, MXML_DESCEND_FIRST);
      if (dataNode != NULL) {
        userData = findData(dataNode);
        assert(userData != NULL);
        dataset = (pNXDS)userData->value.custom.data;
        assert(dataset);
        length = getNXDatasetByteLength(dataset);
        memcpy(dataset->u.ptr, (char *)data + offset, length);
        offset += length;
      }
    }
  }
  return NX_OK;
}

/*------------------------------------------------------------------------*/
NXstatus NXXputdata(NXhandle fid, const void *data) {
  pXMLNexus xmlHandle = NULL;
  mxml_node_t *userData = NULL;
  mxml_node_t *current = NULL;
  pNXDS dataset;
  int i, length, type, rank;
  int64_t dim[NX_MAXRANK];
  char *pPtr = NULL;

  xmlHandle = (pXMLNexus)fid;
  assert(xmlHandle);

  if (xmlHandle->stack[xmlHandle->stackPointer].options & XMLSTACK_OPTION_TABLE) {
    return NXXputdatatable(fid, data);
  }

  if (!isDataNode(xmlHandle->stack[xmlHandle->stackPointer].current)) {
    NXReportError("No dataset open");
    return NX_ERROR;
  }

  current = xmlHandle->stack[xmlHandle->stackPointer].current;
  userData = findData(current);
  assert(userData != NULL);
  if (userData->type == MXML_OPAQUE) {
    /*
      Text data. We have to make sure that the text is \0 terminated.
      Some language bindings do not ensure that this is the case.
    */
    if (NXXgetinfo64(fid, &rank, dim, &type) == NX_OK) {
      length = 1;
      for (i = 0; i < rank; i++) {
        length *= dim[i];
      }
      /* we seem to have trouble reading an empty node back (no userData), so make sure we have at least a single space
       * present even for empty strings */
      if (length == 0) {
        mxmlSetOpaque(userData, " ");
      } else {
        pPtr = (char *)malloc((length + 1) * sizeof(char));
        if (pPtr != NULL) {
          memcpy(pPtr, data, length);
          pPtr[length] = '\0';
          mxmlSetOpaque(userData, (const char *)pPtr);
          free(pPtr);
        }
      }
    } else {
      NXReportError("Unable to determine size of character dataset");
      return NX_ERROR;
    }
  } else {
    dataset = (pNXDS)userData->value.custom.data;
    assert(dataset);
    length = getNXDatasetByteLength(dataset);
    memcpy(dataset->u.ptr, data, length);
  }
  return NX_OK;
}

NXstatus NXXgetdatatable(NXhandle fid, void *data) {
  pXMLNexus xmlHandle = NULL;
  mxml_node_t *userData = NULL;
  mxml_node_t *current = NULL;
  mxml_node_t *nodeRoot = NULL;
  mxml_node_t *dataNodeRoot = NULL;
  mxml_node_t *dataNode = NULL;
  const char *name;
  pNXDS dataset;
  int i, offset, length;
  xmlHandle = (pXMLNexus)fid;
  assert(xmlHandle);

  /* current points at the Idims node as done in NXXopendatatable */
  current = xmlHandle->stack[xmlHandle->stackPointer].current;
  name = current->value.element.name;
  /* we want to walk all Idata nodes and set name */
  nodeRoot = current->parent->parent;
  dataNodeRoot = nodeRoot;
  offset = 0;
  for (i = 0; dataNodeRoot != NULL; i++) {
    dataNodeRoot = mxmlFindElement(dataNodeRoot, nodeRoot, DATA_NODE_NAME, NULL, NULL,
                                   (i == 0 ? MXML_DESCEND_FIRST : MXML_NO_DESCEND));
    if (dataNodeRoot != NULL) {
      dataNode = mxmlFindElement(dataNodeRoot, dataNodeRoot, name, NULL, NULL, MXML_DESCEND_FIRST);
      if (dataNode != NULL) {
        userData = findData(dataNode);
        assert(userData != NULL);
        dataset = (pNXDS)userData->value.custom.data;
        assert(dataset);
        length = getNXDatasetByteLength(dataset);
        memcpy((char *)data + offset, dataset->u.ptr, length);
        offset += length;
      }
    }
  }
  return NX_OK;
}

/*------------------------------------------------------------------------*/
NXstatus NXXgetdata(NXhandle fid, void *data) {
  pXMLNexus xmlHandle = NULL;
  mxml_node_t *userData = NULL;
  mxml_node_t *current = NULL;
  pNXDS dataset;
  int i, length, type, rank;
  int64_t dim[NX_MAXRANK];

  xmlHandle = (pXMLNexus)fid;
  assert(xmlHandle);

  if (xmlHandle->stack[xmlHandle->stackPointer].options & XMLSTACK_OPTION_TABLE) {
    return NXXgetdatatable(fid, data);
  }

  if (!isDataNode(xmlHandle->stack[xmlHandle->stackPointer].current)) {
    NXReportError("No dataset open");
    return NX_ERROR;
  }

  current = xmlHandle->stack[xmlHandle->stackPointer].current;
  userData = findData(current);
  assert(userData != NULL);
  if (userData->type == MXML_OPAQUE) {
    /*
      text data
    */
    if (NXXgetinfo64(fid, &rank, dim, &type) == NX_OK) {
      length = 1;
      for (i = 0; i < rank; i++) {
        length *= dim[i];
      }
      strncpy((char *)data, userData->value.opaque, length);
    } else {
      strcpy((char *)data, nxitrim(userData->value.opaque));
    }

  } else {
    dataset = (pNXDS)userData->value.custom.data;
    assert(dataset);
    length = getNXDatasetByteLength(dataset);
    memcpy(data, dataset->u.ptr, length);
  }
  return NX_OK;
}
/*------------------------------------------------------------------------*/
NXstatus NXXgetinfo64(NXhandle fid, int *rank, int64_t dimension[], int *iType) {
  pXMLNexus xmlHandle = NULL;
  mxml_node_t *userData = NULL;
  mxml_node_t *current = NULL;
  pNXDS dataset;
  int myRank, i;
  const char *attr = NULL;

  xmlHandle = (pXMLNexus)fid;
  assert(xmlHandle);

  if (!isDataNode(xmlHandle->stack[xmlHandle->stackPointer].current)) {
    NXReportError("No dataset open");
    return NX_ERROR;
  }

  current = xmlHandle->stack[xmlHandle->stackPointer].current;
  userData = findData(current);
  assert(userData != NULL);
  if (userData->type == MXML_OPAQUE) {
    /*
      text data
    */
    attr = mxmlElementGetAttr(current, TYPENAME);
    if (attr == NULL) {
      *rank = 1;
      *iType = NX_CHAR;
      dimension[0] = strlen(userData->value.opaque);
    } else {
      *iType = translateTypeCode(attr, "");
      analyzeDim(attr, rank, dimension, iType);
      if (dimension[0] == -1) /* 1D strings are NX_CHAR not NX_CHAR[] so length will not be correct */
      {
        dimension[0] = strlen(userData->value.opaque);
      }
    }
  } else {
    dataset = (pNXDS)userData->value.custom.data;
    assert(dataset);
    myRank = getNXDatasetRank(dataset);
    *rank = myRank;
    *iType = getNXDatasetType(dataset);
    for (i = 0; i < myRank; i++) {
      dimension[i] = getNXDatasetDim(dataset, i);
    }
  }
  return NX_OK;
}
/*---------------------------------------------------------------------
  clone the dataset and set the data pointer. This in order to use
  the addressing and type conversion implemented in nxdataset
---------------------------------------------------------------------*/
static pNXDS makeSlabData(pNXDS dataset, const void *data, const int64_t size[]) {
  pNXDS slabData = NULL;
  int rank, i;

  slabData = (pNXDS)malloc(sizeof(NXDS));
  if (slabData == NULL) {
    return NULL;
  }

  rank = getNXDatasetRank(dataset);
  slabData->rank = rank;
  slabData->dim = (int64_t *)malloc(rank * sizeof(int64_t));
  for (i = 0; i < rank; i++) {
    slabData->dim[i] = size[i];
  }
  slabData->type = getNXDatasetType(dataset);
  slabData->u.ptr = (void *)data;
  slabData->magic = dataset->magic;
  return slabData;
}
/*--------------------------------------------------------------------
  This goes by recursion
----------------------------------------------------------------------*/
static void putSlabData(pNXDS dataset, pNXDS slabData, int dim, const int64_t start[], int64_t sourcePos[],
                        int64_t targetPos[]) {
  int64_t i, rank, length;

  rank = getNXDatasetRank(slabData);
  length = getNXDatasetDim(slabData, dim);
  if (dim != rank - 1) {
    for (i = 0; i < length; i++) {
      targetPos[dim] = start[dim] + i;
      sourcePos[dim] = i;
      putSlabData(dataset, slabData, dim + 1, start, sourcePos, targetPos);
    }
  } else {
    for (i = 0; i < length; i++) {
      targetPos[dim] = start[dim] + i;
      sourcePos[dim] = i;
      putNXDatasetValue(dataset, targetPos, getNXDatasetValue(slabData, sourcePos));
    }
  }
}
/*----------------------------------------------------------------------
 This is in order to support unlimited dimensions along the first axis
 -----------------------------------------------------------------------*/
static int checkAndExtendDataset(mxml_node_t *node, pNXDS dataset, const int64_t start[], const int64_t size[]) {
  int64_t dim0, byteLength;
  void *oldData = NULL;
  char *typestring = NULL;

  dim0 = start[0] + size[0];
  if (dim0 > dataset->dim[0]) {
    byteLength = getNXDatasetByteLength(dataset);
    oldData = dataset->u.ptr;
    dataset->dim[0] = dim0;
    dataset->u.ptr = malloc(getNXDatasetByteLength(dataset));
    if (dataset->u.ptr == NULL) {
      return 0;
    }
    memset(dataset->u.ptr, 0, getNXDatasetByteLength(dataset));
    memcpy(dataset->u.ptr, oldData, byteLength);
    free(oldData);
    typestring = buildTypeString(dataset->type, dataset->rank, dataset->dim);
    if (typestring != NULL) {
      mxmlElementSetAttr(node, TYPENAME, typestring);
      free(typestring);
    } else {
      NXReportError("Failed to allocate typestring");
      return 0;
    }
  }
  return 1;
}

NXstatus NXXputslabtable(NXhandle fid, const void *data, const int64_t iStart[], const int64_t iSize[]) {
  return NX_OK;
}
/*----------------------------------------------------------------------*/
NXstatus NXXputslab64(NXhandle fid, const void *data, const int64_t iStart[], const int64_t iSize[]) {

  pXMLNexus xmlHandle = NULL;
  mxml_node_t *userData = NULL;
  mxml_node_t *current = NULL;
  pNXDS dataset, slabData;
  int64_t sourcePos[NX_MAXRANK], targetPos[NX_MAXRANK];
  int status;

  xmlHandle = (pXMLNexus)fid;
  assert(xmlHandle);

  if (xmlHandle->stack[xmlHandle->stackPointer].options & XMLSTACK_OPTION_TABLE) {
    return NXXputslabtable(fid, data, iStart, iSize);
  }

  if (!isDataNode(xmlHandle->stack[xmlHandle->stackPointer].current)) {
    NXReportError("No dataset open");
    return NX_ERROR;
  }

  current = xmlHandle->stack[xmlHandle->stackPointer].current;
  userData = findData(current);
  assert(userData != NULL);
  if (userData->type == MXML_OPAQUE) {
    NXReportError("This API does not support slabs on text data");
    return NX_ERROR;
  }
  dataset = (pNXDS)userData->value.custom.data;
  assert(dataset);

  status = checkAndExtendDataset(current, dataset, iStart, iSize);
  if (status == 0) {
    NXReportError("Out of memory extending dataset");
    return NX_ERROR;
  }

  slabData = makeSlabData(dataset, data, iSize);
  if (slabData == NULL) {
    NXReportError("Failed to allocate slab data");
    return NX_ERROR;
  }

  putSlabData(dataset, slabData, 0, iStart, sourcePos, targetPos);
  free(slabData->dim);
  free(slabData);

  return NX_OK;
}
/*--------------------------------------------------------------------
  This goes by recursion
----------------------------------------------------------------------*/
static void getSlabData(pNXDS dataset, pNXDS slabData, int dim, const int64_t start[], int64_t sourcePos[],
                        int64_t targetPos[]) {
  int64_t i, rank, length;

  rank = getNXDatasetRank(slabData);
  length = getNXDatasetDim(slabData, dim);
  if (dim != rank - 1) {
    for (i = 0; i < length; i++) {
      sourcePos[dim] = start[dim] + i;
      targetPos[dim] = i;
      getSlabData(dataset, slabData, dim + 1, start, sourcePos, targetPos);
    }
  } else {
    for (i = 0; i < length; i++) {
      sourcePos[dim] = start[dim] + i;
      targetPos[dim] = i;
      putNXDatasetValue(slabData, targetPos, getNXDatasetValue(dataset, sourcePos));
    }
  }
}
/*----------------------------------------------------------------------*/
NXstatus NXXgetslab64(NXhandle fid, void *data, const int64_t iStart[], const int64_t iSize[]) {
  pXMLNexus xmlHandle = NULL;
  mxml_node_t *userData = NULL;
  mxml_node_t *current = NULL;
  pNXDS dataset, slabData;
  int64_t sourcePos[NX_MAXRANK], targetPos[NX_MAXRANK];

  xmlHandle = (pXMLNexus)fid;
  assert(xmlHandle);

  if (!isDataNode(xmlHandle->stack[xmlHandle->stackPointer].current)) {
    NXReportError("No dataset open");
    return NX_ERROR;
  }

  current = xmlHandle->stack[xmlHandle->stackPointer].current;
  userData = findData(current);
  assert(userData != NULL);
  if (userData->type == MXML_OPAQUE) {
    NXReportError("This API does not support slabs on text data");
    return NX_ERROR;
  }
  dataset = (pNXDS)userData->value.custom.data;
  assert(dataset);
  slabData = makeSlabData(dataset, data, iSize);
  if (slabData == NULL) {
    NXReportError("Failed to allocate slab data");
    return NX_ERROR;
  }
  getSlabData(dataset, slabData, 0, iStart, sourcePos, targetPos);
  free(slabData->dim);
  free(slabData);

  return NX_OK;
}
/*----------------------------------------------------------------------*/
static NXstatus NXXsetnumberformat(NXhandle fid, int type, char *format) {
  pXMLNexus xmlHandle = NULL;
  mxml_node_t *current = NULL;
  mxml_node_t *userData = NULL;
  pNXDS dataset;

  xmlHandle = (pXMLNexus)fid;
  assert(xmlHandle);

  if (isDataNode(xmlHandle->stack[xmlHandle->stackPointer].current)) {
    current = xmlHandle->stack[xmlHandle->stackPointer].current;
    userData = findData(current);
    assert(userData != NULL);
    if (userData->type == MXML_OPAQUE) {
      return NX_OK;
    }
    dataset = (pNXDS)userData->value.custom.data;
    assert(dataset);
    if (dataset->format != NULL) {
      free(dataset->format);
    }
    dataset->format = strdup(format);
  } else {
    setNumberFormat(type, format);
  }
  return NX_OK;
}
/*============================ Attributes ============================*/
static char *formatAttributeData(const void *data, int datalen, int iType) {
  int intData = 0;
  long iValue = -99999;
  double dValue = -1e38;
  char type[20];
  char *number;

  if (iType == NX_CHAR) {
    /* data may not be NULL terminated */
    number = (char *)malloc((datalen + 1) * sizeof(char));
    memcpy(number, data, datalen * sizeof(char));
    number[datalen] = '\0';
    return number;
  }

  number = (char *)malloc(132 * sizeof(char));
  if (!number) {
    NXReportError("Failed to allocate attribute number buffer");
    return NULL;
  }

  if (datalen > 1) {
    return NULL;
  }
  type[0] = '\0';
  switch (iType) {
  case NX_INT32:
    iValue = ((int *)data)[0];
    intData = 1;
    strcpy(type, "NX_INT32:");
    break;
  case NX_UINT32:
    iValue = ((unsigned int *)data)[0];
    intData = 1;
    strcpy(type, "NX_UINT32:");
    break;
  case NX_INT16:
    iValue = ((short *)data)[0];
    intData = 1;
    strcpy(type, "NX_INT16:");
    break;
  case NX_UINT16:
    iValue = ((unsigned short *)data)[0];
    intData = 1;
    strcpy(type, "NX_UINT16:");
    break;
  case NX_INT8:
    iValue = (int)((char *)data)[0];
    intData = 1;
    strcpy(type, "NX_INT8:");
    break;
  case NX_UINT8:
    intData = 1;
    iValue = (int)((unsigned char *)data)[0];
    strcpy(type, "NX_UINT8:");
    break;
  case NX_FLOAT32:
    dValue = ((float *)data)[0];
    strcpy(type, "NX_FLOAT32:");
    intData = 0;
    break;
  case NX_FLOAT64:
    dValue = ((double *)data)[0];
    strcpy(type, "NX_FLOAT64:");
    intData = 0;
    break;
  }
  if (intData) {
    snprintf(number, 79, "%s%ld", type, iValue);
  } else {
    snprintf(number, 79, "%s%f", type, dValue);
  }
  return number;
}
/*---------------------------------------------------------------------*/
NXstatus NXXputattr(NXhandle fid, CONSTCHAR *name, const void *data, int datalen, int iType) {
  char buffer[256];
  pXMLNexus xmlHandle = NULL;
  mxml_node_t *current = NULL;
  char *numberData = NULL;

  xmlHandle = (pXMLNexus)fid;
  assert(xmlHandle);
  if (!validNXName(name, 1)) {
    sprintf(buffer, "ERROR: invalid characters in attribute name \"%s\"", name);
    NXReportError(buffer);
    return NX_ERROR;
  }

  current = xmlHandle->stack[xmlHandle->stackPointer].current;
  if (isDataNode(xmlHandle->stack[xmlHandle->stackPointer].current)) {
    if (strcmp(name, TYPENAME) == 0) {
      NXReportError("type is a reserved attribute name, rejected");
      return NX_ERROR;
    }
  }

  numberData = formatAttributeData(data, datalen, iType);
  if (numberData == NULL) {
    NXReportError("This API does not support non number arrays");
    return NX_ERROR;
  } else {
    mxmlElementSetAttr(current, name, numberData);
    free(numberData);
  }
  return NX_OK;
}
/*--------------------------------------------------------------------------*/
NXstatus NXXgetattr(NXhandle fid, const char *name, void *data, int *datalen, int *iType) {
  pXMLNexus xmlHandle = NULL;
  mxml_node_t *current = NULL;
  const char *attribute = NULL;
  char error[1024];
  const char *attData = NULL;
  int nx_type;

  xmlHandle = (pXMLNexus)fid;
  assert(xmlHandle);

  current = xmlHandle->stack[xmlHandle->stackPointer].current;

  attribute = mxmlElementGetAttr(current, name);
  if (!attribute) {
    snprintf(error, 1023, "Attribute %s not found", name);
    NXReportError(error);
    return NX_ERROR;
  }
  nx_type = translateTypeCode((char *)attribute, ":");
  if (nx_type < 0) {
    /*
      no type code == text attribute
    */
    nx_type = NX_CHAR;
  } else {
    /*
      We need to find the number after the type code. However, there is
      the complication of the datatype type attribute ...
    */
    if (strcmp(name, TYPENAME) == 0) {
      nx_type = NX_CHAR;
    } else {
      attData = strchr(attribute, (int)':');
      if (attData == NULL) {
        snprintf(error, 1023, "ERROR: bad attribute string, : missing for attribute \"%s\"", name);
        NXReportError(error);
        return NX_ERROR;
      }
      attData++;
    }
  }
  *iType = nx_type;
  switch (nx_type) {
  case NX_CHAR:
    /* enforce NULL termination regardless of length of datalen */
    strncpy((char *)data, attribute, *datalen - 1);
    ((char *)data)[*datalen - 1] = '\0';
    /* *datalen = strlen(attribute); */
    *datalen = strlen((char *)data);
    *iType = NX_CHAR;
    break;
  case NX_INT32:
    ((int *)data)[0] = atoi(attData);
    *datalen = 1;
    break;
  case NX_UINT32:
    ((unsigned int *)data)[0] = atoi(attData);
    *datalen = 1;
    break;
  case NX_INT16:
    ((short *)data)[0] = atoi(attData);
    *datalen = 1;
    break;
  case NX_UINT16:
    ((unsigned short *)data)[0] = atoi(attData);
    *datalen = 1;
    break;
  case NX_INT8:
    ((char *)data)[0] = atoi(attData);
    *datalen = 1;
    break;
  case NX_UINT8:
    ((unsigned char *)data)[0] = atoi(attData);
    *datalen = 1;
    break;
  case NX_FLOAT32:
    ((float *)data)[0] = atof(attData);
    *datalen = 1;
    break;
  case NX_FLOAT64:
    ((double *)data)[0] = atof(attData);
    *datalen = 1;
    break;
  }

  return NX_OK;
}

/* find the next node, ignoring Idata */
static mxml_node_t *find_node(mxml_node_t *node, int next) {
  int done = 0;
  mxml_node_t *parent_next = NULL; /* parent to use if we are in an Idims  search */
  if (node == NULL) {
    return NULL;
  }
  if ((node->parent != NULL) && !strcmp(node->parent->value.element.name, DIMS_NODE_NAME)) {
    parent_next = node->parent->next;
  } else {
    parent_next = NULL;
  }
  if (next) {
    if (node->next != NULL) {
      node = node->next;
    } else {
      node = parent_next;
    }
  }
  while (node != NULL && !done) {
    if ((node->parent != NULL) && !strcmp(node->parent->value.element.name, DIMS_NODE_NAME)) {
      parent_next = node->parent->next;
    } else {
      parent_next = NULL;
    }
    if ((node->type != MXML_ELEMENT) || !strcmp(node->value.element.name, DATA_NODE_NAME)) {
      if (node->next != NULL) {
        node = node->next;
      } else {
        node = parent_next;
      }
      continue;
    }
    if (!strcmp(node->value.element.name, DIMS_NODE_NAME)) {
      node = node->child;
      continue;
    }
    done = 1;
  }
  return node;
}

/*====================== search functions =================================*/
NXstatus NXXgetnextentry(NXhandle fid, NXname name, NXname nxclass, int *datatype) {
  pXMLNexus xmlHandle = NULL;
  mxml_node_t *next = NULL, *userData, *node = NULL;
  int stackPtr;
  const char *target = NULL, *attname = NULL;
  pNXDS dataset;
  char pBueffel[256];
  const char *linkName = NULL;

  xmlHandle = (pXMLNexus)fid;
  assert(xmlHandle);

  if (isDataNode(xmlHandle->stack[xmlHandle->stackPointer].current)) {
    /*
      be nice to user: silently fix this problem
    */
    NXXclosedata(fid);
  }

  stackPtr = xmlHandle->stackPointer;
  if (xmlHandle->stack[stackPtr].currentChild == NULL) {
    /*
      initialization of search
    */
    node = find_node(xmlHandle->stack[stackPtr].current->child, 0);
  } else {
    /*
      proceed
    */
    node = find_node(xmlHandle->stack[stackPtr].currentChild, 1);
  }
  xmlHandle->stack[stackPtr].currentChild = node;
  next = node;
  if (next == NULL) {
    return NX_EOD;
  }
  if (strcmp(next->value.element.name, "NAPIlink") == 0) {
    target = mxmlElementGetAttr(next, "target");
    linkName = mxmlElementGetAttr(next, "name");
    if (target == NULL) {
      NXReportError("Corrupted file, NAPIlink without target");
      return NX_ERROR;
    }
    next = getLinkTarget(xmlHandle, target);
    if (next == NULL) {
      NXReportError("Corrupted file, broken link");
      return NX_ERROR;
    }
  }

  if (isDataNode(next)) {
    strcpy(name, next->value.element.name);
    strcpy(nxclass, "SDS");
    userData = findData(next);
    if (userData == NULL) {
      snprintf(pBueffel, 255, "Corrupted file, userData for %s not found", name);
      NXReportError(pBueffel);
      return NX_ERROR;
    }
    if (userData->type == MXML_OPAQUE) {
      *datatype = NX_CHAR;
    } else {
      dataset = (pNXDS)userData->value.custom.data;
      assert(dataset);
      *datatype = getNXDatasetType(dataset);
    }
  } else {
    strcpy(nxclass, next->value.element.name);
    attname = mxmlElementGetAttr(next, "name");
    strcpy(name, attname);
  }
  /*
    this is for named links
  */
  if (linkName != NULL) {
    strcpy(name, linkName);
  }
  return NX_OK;
}
/*----------------------------------------------------------------------*/
extern NXstatus NXXinitgroupdir(NXhandle fid) {
  pXMLNexus xmlHandle = NULL;
  int stackPtr;

  xmlHandle = (pXMLNexus)fid;
  assert(xmlHandle);

  if (isDataNode(xmlHandle->stack[xmlHandle->stackPointer].current)) {
    NXReportError("Cannot search datasets");
    return NX_ERROR;
  }

  stackPtr = xmlHandle->stackPointer;
  xmlHandle->stack[stackPtr].currentChild = NULL;
  return NX_OK;
}
/*-------------------------------------------------------------------------*/
NXstatus NXXgetnextattr(NXhandle fid, NXname pName, int *iLength, int *iType) {
  pXMLNexus xmlHandle = NULL;
  mxml_node_t *current = NULL;
  int stackPtr, currentAtt, nx_type;
  char *attVal;

  xmlHandle = (pXMLNexus)fid;
  assert(xmlHandle);

  stackPtr = xmlHandle->stackPointer;

  current = xmlHandle->stack[stackPtr].current;
  currentAtt = xmlHandle->stack[stackPtr].currentAttribute;
  if (currentAtt >= current->value.element.num_attrs) {
    xmlHandle->stack[stackPtr].currentAttribute = 0;
    return NX_EOD;
  }

  /*
    hide group name attribute
  */
  if (strcmp(current->value.element.attrs[currentAtt].name, "name") == 0 && !isDataNode(current)) {
    xmlHandle->stack[stackPtr].currentAttribute++;
    return NXXgetnextattr(fid, pName, iLength, iType);
  }

  /*
    hide type attribute
  */
  if (strcmp(current->value.element.attrs[currentAtt].name, TYPENAME) == 0 && isDataNode(current)) {
    xmlHandle->stack[stackPtr].currentAttribute++;
    return NXXgetnextattr(fid, pName, iLength, iType);
  }

  strcpy(pName, current->value.element.attrs[currentAtt].name);
  attVal = current->value.element.attrs[currentAtt].value;
  nx_type = translateTypeCode((char *)attVal, ":");
  if (nx_type < 0 || strcmp(pName, TYPENAME) == 0) {
    /*
      no type == NX_CHAR
    */
    *iLength = strlen(attVal);
    *iType = NX_CHAR;
  } else {
    *iLength = 1;
    *iType = nx_type;
  }

  xmlHandle->stack[stackPtr].currentAttribute++;
  return NX_OK;
}
/*-------------------------------------------------------------------------*/
extern NXstatus NXXinitattrdir(NXhandle fid) {
  pXMLNexus xmlHandle = NULL;
  int stackPtr;

  xmlHandle = (pXMLNexus)fid;
  assert(xmlHandle);

  stackPtr = xmlHandle->stackPointer;
  xmlHandle->stack[stackPtr].currentAttribute = 0;
  return NX_OK;
}
/*-------------------------------------------------------------------------*/
NXstatus NXXgetgroupinfo(NXhandle fid, int *iN, NXname pName, NXname pClass) {
  pXMLNexus xmlHandle = NULL;
  mxml_node_t *node = NULL, *child = NULL;
  mxml_node_t *current = NULL;
  const char *nameAtt = NULL;
  int childCount;

  xmlHandle = (pXMLNexus)fid;
  assert(xmlHandle);

  if (isDataNode(xmlHandle->stack[xmlHandle->stackPointer].current)) {
    NXReportError("No group open");
    return NX_ERROR;
  }
  current = xmlHandle->stack[xmlHandle->stackPointer].current;

  nameAtt = mxmlElementGetAttr(current, "name");
  if (nameAtt != NULL) {
    strcpy(pName, nameAtt);
  }
  strcpy(pClass, current->value.element.name);

  /* count all child nodes, but need to ignore DATA_NODE_NAME and
   * descend into DIMS_NODE_NAME
   */
  childCount = 0;
  node = current->child;
  while (node != NULL) {
    if (!strcmp(node->value.element.name, DATA_NODE_NAME)) {
      ; /* names also exist in DIMS_NODE_NAME so do nothing here */
    } else if (!strcmp(node->value.element.name, DIMS_NODE_NAME)) {
      child = node->child;
      while (child != NULL) {
        /* not sure why this check is needed, but you double count otherwise */
        if (child->type == MXML_ELEMENT) {
          childCount++;
        }
        child = child->next;
      }
    } else {
      childCount++;
    }
    node = node->next;
  }
  *iN = childCount;
  return NX_OK;
}
/*----------------------------------------------------------------------*/
NXstatus NXXgetattrinfo(NXhandle fid, int *iN) {
  pXMLNexus xmlHandle = NULL;
  mxml_node_t *current = NULL;
  int stackPtr, skip;

  xmlHandle = (pXMLNexus)fid;
  assert(xmlHandle);

  stackPtr = xmlHandle->stackPointer;

  current = xmlHandle->stack[stackPtr].current;

  /*
    hide type and group name attributes
  */
  skip = 0;
  if (isDataNode(current)) {
    /* data nodes may have type */
    if (mxmlElementGetAttr(current, TYPENAME) != NULL)
      skip = 1;
  } else {
    /* group nodes (except root) have name */
    if (mxmlElementGetAttr(current, "name") != NULL)
      skip = 1;
  }
  *iN = current->value.element.num_attrs - skip;
  return NX_OK;
}
/*================= Linking functions =================================*/
static int countPathChars(mxml_node_t *path[], int stackPtr) {
  int count = 1;
  const char *name = NULL;

  while (stackPtr >= 0) {
    if (isDataNode(path[stackPtr])) {
      count += strlen(path[stackPtr]->value.element.name);
    } else {
      name = mxmlElementGetAttr(path[stackPtr], "name");
      if (name != NULL) {
        count += strlen(name);
      }
    }
    stackPtr--;
    count += 1;
  }
  return count;
}
/*-------------------------------------------------------------------*/
static char *buildPathString(mxml_node_t *path[], int stackPtr) {
  int count = 0;
  const char *name = NULL;
  char *pathString = NULL;

  count = countPathChars(path, stackPtr);
  pathString = (char *)malloc((count + 10) * sizeof(char));
  if (pathString == NULL) {
    return NULL;
  }
  memset(pathString, 0, (count + 10) * sizeof(char));

  while (stackPtr >= 0) {
    if (isDataNode(path[stackPtr])) {
      strcat(pathString, "/");
      strcat(pathString, path[stackPtr]->value.element.name);
    } else {
      name = mxmlElementGetAttr(path[stackPtr], "name");
      if (name != NULL) {
        strcat(pathString, "/");
        strcat(pathString, name);
      }
    }
    stackPtr--;
  }
  return pathString;
}
/*--------------------------------------------------------------------*/
static char *findLinkPath(mxml_node_t *node) {
  mxml_node_t **path = NULL;
  int stackPtr;
  mxml_node_t *current = NULL;
  char *result = NULL;

  path = (mxml_node_t **)malloc(NXMAXSTACK * sizeof(mxml_node_t *));
  if (path == NULL) {
    NXReportError("ERROR: out of memory following link path");
    return NULL;
  }
  memset(path, 0, NXMAXSTACK * sizeof(mxml_node_t *));

  /*
    first path:  walk up the tree untill NXroot is found
  */
  current = node;
  stackPtr = 0;
  while (current != NULL && strcmp(current->value.element.name, "NXroot") != 0) {
    path[stackPtr] = current;
    stackPtr++;
    current = current->parent;
  }
  stackPtr--;

  /*
    path now contains the nodes to the root node in reverse order.
    From this build the path string
  */
  result = buildPathString(path, stackPtr);
  free(path);
  return result;
}
/*--------------------------------------------------------------------*/
NXstatus NXXgetdataID(NXhandle fid, NXlink *sRes) {
  pXMLNexus xmlHandle = NULL;
  mxml_node_t *current = NULL;
  char *linkPath = NULL;

  xmlHandle = (pXMLNexus)fid;
  assert(xmlHandle);

  if (!isDataNode(xmlHandle->stack[xmlHandle->stackPointer].current)) {
    return NX_ERROR;
  }
  current = xmlHandle->stack[xmlHandle->stackPointer].current;

  linkPath = findLinkPath(current);
  if (!linkPath) {
    NXReportError("Failed to allocate link path string");
    return NX_ERROR;
  }
  strncpy(sRes->targetPath, linkPath, 1023);
  free(linkPath);
  return NX_OK;
}
/*--------------------------------------------------------------------*/
NXstatus NXXgetgroupID(NXhandle fid, NXlink *sRes) {
  pXMLNexus xmlHandle = NULL;
  mxml_node_t *current = NULL;
  char *linkPath = NULL;

  xmlHandle = (pXMLNexus)fid;
  assert(xmlHandle);

  if (isDataNode(xmlHandle->stack[xmlHandle->stackPointer].current)) {
    NXReportError("No group open");
    return NX_ERROR;
  }
  current = xmlHandle->stack[xmlHandle->stackPointer].current;

  if (xmlHandle->stackPointer == 0) {
    return NX_ERROR;
  }

  linkPath = findLinkPath(current);
  if (!linkPath) {
    NXReportError("Failed to allocate link path string");
    return NX_ERROR;
  }
  strncpy(sRes->targetPath, linkPath, 1023);
  free(linkPath);
  return NX_OK;
}

/*-----------------------------------------------------------------------*/
NXstatus NXXprintlink(NXhandle fid, NXlink *sLink) {
  pXMLNexus xmlHandle = NULL;
  xmlHandle = (pXMLNexus)fid;
  assert(xmlHandle);

  printf("XML link: target=\"%s\"\n", sLink->targetPath);
  return NX_OK;
}

/*-----------------------------------------------------------------------*/
NXstatus NXXmakelink(NXhandle fid, NXlink *sLink) {
  pXMLNexus xmlHandle = NULL;
  mxml_node_t *current = NULL, *linkNode = NULL;
  mxml_node_t *linkedNode = NULL;

  xmlHandle = (pXMLNexus)fid;
  assert(xmlHandle);

  if (isDataNode(xmlHandle->stack[xmlHandle->stackPointer].current)) {
    NXReportError("No group to link to open");
    return NX_ERROR;
  }
  current = xmlHandle->stack[xmlHandle->stackPointer].current;
  linkNode = mxmlNewElement(current, "NAPIlink");
  if (!linkNode) {
    NXReportError("Failed to allocate new link element");
    return NX_ERROR;
  }
  mxmlElementSetAttr(linkNode, "target", sLink->targetPath);
  linkedNode = getLinkTarget(xmlHandle, sLink->targetPath);
  if (linkedNode != NULL) {
    mxmlElementSetAttr(linkedNode, "target", sLink->targetPath);
  }
  return NX_OK;
}

/*-----------------------------------------------------------------------*/
NXstatus NXXmakenamedlink(NXhandle fid, CONSTCHAR *name, NXlink *sLink) {
  pXMLNexus xmlHandle = NULL;
  mxml_node_t *current = NULL, *linkNode = NULL;
  mxml_node_t *linkedNode = NULL;
  char buffer[256];

  xmlHandle = (pXMLNexus)fid;
  assert(xmlHandle);
  if (!validNXName(name, 0)) {
    sprintf(buffer, "ERROR: invalid characters in link name \"%s\"", name);
    NXReportError(buffer);
    return NX_ERROR;
  }

  if (isDataNode(xmlHandle->stack[xmlHandle->stackPointer].current)) {
    NXReportError("No group to link to open");
    return NX_ERROR;
  }
  current = xmlHandle->stack[xmlHandle->stackPointer].current;
  linkNode = mxmlNewElement(current, "NAPIlink");
  if (!linkNode) {
    NXReportError("Failed to allocate new link element");
    return NX_ERROR;
  }
  mxmlElementSetAttr(linkNode, "target", sLink->targetPath);
  mxmlElementSetAttr(linkNode, "name", name);
  linkedNode = getLinkTarget(xmlHandle, sLink->targetPath);
  if (linkedNode != NULL) {
    mxmlElementSetAttr(linkedNode, "target", sLink->targetPath);
  }
  return NX_OK;
}
/*----------------------------------------------------------------------*/
NXstatus NXXsameID(NXhandle fileid, NXlink *pFirstID, NXlink *pSecondID) {
  if (strcmp(pFirstID->targetPath, pSecondID->targetPath) == 0) {
    return NX_OK;
  } else {
    return NX_ERROR;
  }
}

/*--------------------------------------------------------------------*/

int NXXcompress(NXhandle fid, int comp) {
  /* that will throw an exception in the Java API, errors have to be fatal */
  /* NXReportError("NXcompress is deprecated, IGNORED"); */
  return NX_OK;
}

/*--------------------------------------------------------------------*/
NXstatus NXXputattra(NXhandle handle, CONSTCHAR *name, const void *data, const int rank, const int dim[],
                     const int iType) {
  if (rank > 1) {
    NXReportError("This is an XML file, there is only rudimentary support for attribute arrays wirh rank <=1");
    return NX_ERROR;
  }

  return NXXputattr(handle, name, data, dim[0], iType);
}

/*--------------------------------------------------------------------*/
NXstatus NXXgetnextattra(NXhandle handle, NXname pName, int *rank, int dim[], int *iType) {
  NXstatus ret = NXXgetnextattr(handle, pName, dim, iType);
  if (ret != NX_OK)
    return ret;
  (*rank) = 1;
  if (dim[0] <= 1)
    (*rank) = 0;
  return NX_OK;
}

/*--------------------------------------------------------------------*/
NXstatus NXXgetattra(NXhandle handle, const char *name, void *data) {
  NXReportError("This is an XML file, attribute array API is not supported here");
  return NX_ERROR;
}

/*--------------------------------------------------------------------*/
NXstatus NXXgetattrainfo(NXhandle handle, NXname pName, int *rank, int dim[], int *iType) {
  NXReportError("This is an XML file, attribute array API is not supported here");
  return NX_ERROR;
}

/*----------------------------------------------------------------------*/
void NXXassignFunctions(pNexusFunction fHandle) {
  fHandle->nxclose = NXXclose;
  fHandle->nxreopen = NULL;
  fHandle->nxflush = NXXflush;
  fHandle->nxmakegroup = NXXmakegroup;
  fHandle->nxopengroup = NXXopengroup;
  fHandle->nxclosegroup = NXXclosegroup;
  fHandle->nxmakedata64 = NXXmakedata64;
  fHandle->nxcompmakedata64 = NXXcompmakedata64;
  fHandle->nxcompress = NXXcompress;
  fHandle->nxopendata = NXXopendata;
  fHandle->nxclosedata = NXXclosedata;
  fHandle->nxputdata = NXXputdata;
  fHandle->nxputattr = NXXputattr;
  fHandle->nxputslab64 = NXXputslab64;
  fHandle->nxgetdataID = NXXgetdataID;
  fHandle->nxmakelink = NXXmakelink;
  fHandle->nxmakenamedlink = NXXmakenamedlink;
  fHandle->nxgetdata = NXXgetdata;
  fHandle->nxgetinfo64 = NXXgetinfo64;
  fHandle->nxgetnextentry = NXXgetnextentry;
  fHandle->nxgetslab64 = NXXgetslab64;
  fHandle->nxgetnextattr = NXXgetnextattr;
  fHandle->nxgetattr = NXXgetattr;
  fHandle->nxgetattrinfo = NXXgetattrinfo;
  fHandle->nxgetgroupID = NXXgetgroupID;
  fHandle->nxgetgroupinfo = NXXgetgroupinfo;
  fHandle->nxsameID = NXXsameID;
  fHandle->nxinitgroupdir = NXXinitgroupdir;
  fHandle->nxinitattrdir = NXXinitattrdir;
  fHandle->nxsetnumberformat = NXXsetnumberformat;
  fHandle->nxprintlink = NXXprintlink;
  fHandle->nxnativeexternallink = NULL;
  fHandle->nxputattra = NXXputattra;
  fHandle->nxgetnextattra = NXXgetnextattra;
  fHandle->nxgetattra = NXXgetattra;
  fHandle->nxgetattrainfo = NXXgetattrainfo;
}

#endif /*NXXML*/
