/**
 * This file contains functions necessary to perform XML-I/O for
 * NeXus with the mxml-library.
 *
 * Most notably it contains the callback function for reading and
 * writing  data
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

#include "MantidNexusCpp/nxconfig.h"

#ifdef WITH_MXML

#include "MantidNexusCpp/napi.h"
#include "MantidNexusCpp/napiconfig.h"
#include "MantidNexusCpp/nxdataset.h"
#include "MantidNexusCpp/nxio.h"
#include <assert.h>
#include <mxml.h>

/* fix for mxml-2.3 */
#ifndef MXML_WRAP
#define MXML_WRAP 79
#endif

#ifdef _MSC_VER
#define snprintf _snprintf
#endif /* _MSC_VER */

/* #define TESTMAIN 1 */
/*=================== type code handling ================================= */
typedef struct {
  char name[30];
  char format[30];
  int nx_type;
} type_code;

#define NTYPECODE 11
static type_code typecode[NTYPECODE];
/*-----------------------------------------------------------------------*/
void initializeNumberFormats() {
  type_code myCode;

  strcpy(myCode.name, "NX_FLOAT32");
  strcpy(myCode.format, "%12.4f");
  myCode.nx_type = NX_FLOAT32;
  typecode[0] = myCode;

  strcpy(myCode.name, "NX_FLOAT64");
  strcpy(myCode.format, "%16.5f");
  myCode.nx_type = NX_FLOAT64;
  typecode[1] = myCode;

  strcpy(myCode.name, "NX_INT8");
  strcpy(myCode.format, "%5d");
  myCode.nx_type = NX_INT8;
  typecode[2] = myCode;

  strcpy(myCode.name, "NX_UINT8");
  strcpy(myCode.format, "%5d");
  myCode.nx_type = NX_UINT8;
  typecode[3] = myCode;

  strcpy(myCode.name, "NX_INT16");
  strcpy(myCode.format, "%8d");
  myCode.nx_type = NX_INT16;
  typecode[4] = myCode;

  strcpy(myCode.name, "NX_UINT16");
  strcpy(myCode.format, "%8d");
  myCode.nx_type = NX_UINT16;
  typecode[5] = myCode;

  strcpy(myCode.name, "NX_INT32");
  strcpy(myCode.format, "%12d");
  myCode.nx_type = NX_INT32;
  typecode[6] = myCode;

  strcpy(myCode.name, "NX_UINT32");
  strcpy(myCode.format, "%12d");
  myCode.nx_type = NX_UINT32;
  typecode[7] = myCode;

  strcpy(myCode.name, "NX_INT64");
  strcpy(myCode.format, "%24lld");
  myCode.nx_type = NX_INT64;
  typecode[8] = myCode;

  strcpy(myCode.name, "NX_UINT64");
  strcpy(myCode.format, "%24llu");
  myCode.nx_type = NX_UINT64;
  typecode[9] = myCode;

  strcpy(myCode.name, "NX_CHAR");
  strcpy(myCode.format, "%c");
  myCode.nx_type = NX_CHAR;
  typecode[10] = myCode;
}
/*----------------------------------------------------------------------*/
void setNumberFormat(int nx_type, char *format) {
  int i;

  for (i = 0; i < NTYPECODE; i++) {
    if (typecode[i].nx_type == nx_type) {
      strncpy(typecode[i].format, format, 29);
    }
  }
}
/*------------------------------------------------------------------*/
static void getNumberFormat(int nx_type, char format[30]) {
  int i;

  for (i = 0; i < NTYPECODE; i++) {
    if (typecode[i].nx_type == nx_type) {
      strncpy(format, typecode[i].format, 29);
    }
  }
}
/*----------------------------------------------------------------*/
void getNumberText(int nx_type, char *typestring, int typeLen) {
  int i;

  for (i = 0; i < NTYPECODE; i++) {
    if (typecode[i].nx_type == nx_type) {
      strncpy(typestring, typecode[i].name, typeLen);
    }
  }
}
/*
 * 'mxml_add_char()' - Add a character to a buffer, expanding as needed.
 * copied here from mxml-file.c to achieve compatibility with mxml-2.1
 * standard
 */

static int                      /* O  - 0 on success, -1 on error */
myxml_add_char(int ch,          /* I  - Character to add */
               char **bufptr,   /* IO - Current position in buffer */
               char **buffer,   /* IO - Current buffer */
               size_t *bufsize) /* IO - Current buffer size */
{
  char *newbuffer; /* New buffer value */

  if (*bufptr >= (*buffer + *bufsize - 4)) {
    /*
     * Increase the size of the buffer...
     */

    if (*bufsize < 1024) {
      (*bufsize) *= 2;
    } else {
      (*bufsize) *= 3;
      (*bufsize) /= 2;
    }

    newbuffer = (char *)malloc(*bufsize * sizeof(char));
    if (!newbuffer) {
      free(*buffer);

      mxml_error("Unable to expand string buffer to %d bytes!", *bufsize);

      return (-1);
    }
    memset(newbuffer, 0, *bufsize * sizeof(char));
    memcpy(newbuffer, *buffer, *bufptr - *buffer);
    free(*buffer);

    *bufptr = newbuffer + (*bufptr - *buffer);
    *buffer = newbuffer;
  }

  if (ch < 128) {
    /*
     * Single byte ASCII...
     */

    *(*bufptr)++ = ch;
  } else if (ch < 2048) {
    /*
     * Two-byte UTF-8...
     */

    *(*bufptr)++ = 0xc0 | (ch >> 6);
    *(*bufptr)++ = 0x80 | (ch & 0x3f);
  } else if (ch < 65536) {
    /*
     * Three-byte UTF-8...
     */

    *(*bufptr)++ = 0xe0 | (ch >> 12);
    *(*bufptr)++ = 0x80 | ((ch >> 6) & 0x3f);
    *(*bufptr)++ = 0x80 | (ch & 0x3f);
  } else {
    /*
     * Four-byte UTF-8...
     */

    *(*bufptr)++ = 0xf0 | (ch >> 18);
    *(*bufptr)++ = 0x80 | ((ch >> 12) & 0x3f);
    *(*bufptr)++ = 0x80 | ((ch >> 6) & 0x3f);
    *(*bufptr)++ = 0x80 | (ch & 0x3f);
  }

  return (0);
}
/*------------------------------------------------------------------*/
extern char *stptok(char *s, char *tok, size_t toklen, char *brk);
/*=====================================================================
 actual stuff for implementing the callback functions
 =====================================================================*/

/*
 * if passed NX_CHAR, then returns dimension of -1 and the caller
 * needs to do a strlen() or equivalent
 */
void analyzeDim(const char *typeString, int *rank, int64_t *iDim, int *type) {
  char dimString[132];
  char dim[20];
  const char *dimStart, *dimEnd;
  char *dimTemp;
  int myRank;

  if (strchr(typeString, (int)'[') == NULL) {
    *rank = 1;
    switch (*type) {
    case NX_INT8:
    case NX_UINT8:
    case NX_INT16:
    case NX_UINT16:
    case NX_INT32:
    case NX_UINT32:
    case NX_INT64:
    case NX_UINT64:
    case NX_FLOAT32:
    case NX_FLOAT64:
      iDim[0] = 1;
      break;
    case NX_CHAR:
      iDim[0] = -1; /* length unknown, caller needs to determine later */
      break;
    default:
      mxml_error("ERROR: (analyzeDim) unknown type code %d for typeString %s", *type, typeString);
      break;
    }
  } else {
    /*
      we have to determine rank and the dims.
      Start by extracting the dimension string.
    */
    dimStart = strchr(typeString, (int)'[') + 1;
    dimEnd = strchr(typeString, (int)']');
    if (!dimStart || !dimEnd) {
      mxml_error("ERROR: malformed dimension string in %s", typeString);
      return;
    }
    if ((dimEnd - dimStart) > 131) {
      mxml_error("ERROR: run away dimension definition in %s", typeString);
      return;
    }
    memset(dimString, 0, 132);
    memcpy(dimString, dimStart, (dimEnd - dimStart) * sizeof(char));
    dimTemp = stptok(dimString, dim, 19, ",");
    myRank = 0;
    while (dimTemp != NULL) {
      iDim[myRank] = atoi(dim);
      dimTemp = stptok(dimTemp, dim, 19, ",");
      myRank++;
    }
    *rank = myRank;
  }
}
/*--------------------------------------------------------------------*/
int translateTypeCode(const char *code, const char *term) {
  int i, result = -1;
  char test_str[80];

  for (i = 0; i < NTYPECODE; i++) {
    snprintf(test_str, sizeof(test_str) - 1, "%s%s", typecode[i].name, term);
    if (strncmp(code, test_str, strlen(test_str)) == 0) {
      result = typecode[i].nx_type;
      break;
    }
  }
  return result;
}

/*
 * This is used to locate an Idims node from the new style table data layout
 */
static mxml_node_t *findDimsNode(mxml_node_t *node) {
  mxml_node_t *tnode = NULL;
  const char *name = node->value.element.name;
  if ((node->parent != NULL) && !strcmp(node->parent->value.element.name, DATA_NODE_NAME)) {
    tnode = mxmlFindElement(node->parent->parent, node->parent->parent, DIMS_NODE_NAME, NULL, NULL, MXML_DESCEND_FIRST);
    if (tnode != NULL) {
      tnode = mxmlFindElement(tnode, tnode, name, NULL, NULL, MXML_DESCEND_FIRST);
    }
  }
  return tnode;
}

/*---------------------------------------------------------------------*/
/*return 1 if in table mode , 0 if not */
static void analyzeDataType(mxml_node_t *parent, int *rank, int *type, int64_t *iDim) {
  const char *typeString;
  mxml_node_t *tnode;
  int nx_type = -1;
  int table_mode = 0;

  *rank = 1;
  *type = NX_CHAR;
  iDim[0] = -1;

  /*
    get the type attribute. No attribute means: plain text
  */
  tnode = findDimsNode(parent);
  if (tnode != NULL) {
    table_mode = 1;
    parent = tnode;
  }
  typeString = mxmlElementGetAttr(parent, TYPENAME);
  if (typeString == NULL) {
    return;
  }

  nx_type = translateTypeCode((char *)typeString, "");

  /*
    assign type
  */
  if (nx_type == -1) {
    mxml_error("ERROR: %s is an invalid NeXus type, I try to continue but may fail", typeString);
    *type = NX_CHAR;
    return;
  }

  *type = nx_type;

  analyzeDim(typeString, rank, iDim, type);
  if (table_mode) {
    *rank = 1;
    iDim[0] = 1;
  }
  return;
}
/*-------------------------------------------------------------------*/
void destroyDataset(void *data) {
  if (data != NULL) {
    dropNXDataset((pNXDS)data);
  }
}
/*-------------------------------------------------------------------*/
static char *getNextNumber(char *pStart, char pNumber[80]) {
  int charCount = 0;
  pNumber[0] = '\0';

  /* advance to first digit */
  while (isspace(*pStart) && *pStart != '\0') {
    pStart++;
  }
  if (*pStart == '\0') {
    return NULL;
  }

  /* copy */
  while (!isspace(*pStart) && *pStart != '\0' && charCount < 78) {
    pNumber[charCount] = *pStart;
    pStart++;
    charCount++;
  }
  pNumber[charCount] = '\0';
  return pStart;
}
/*--------------------------------------------------------------------*/
mxml_type_t nexusTypeCallback(mxml_node_t *parent) {
  const char *typeString;

  if (strstr(parent->value.element.name, "?xml") != NULL || !strncmp(parent->value.element.name, "NX", 2) ||
      !strcmp(parent->value.element.name, DATA_NODE_NAME) || !strcmp(parent->value.element.name, DIMS_NODE_NAME)) {
    return MXML_ELEMENT;
  } else {
    /* data nodes do not habe TYPENAME in table style but are always CUSTOM */
    if (parent->parent != NULL && !strcmp(parent->parent->value.element.name, DATA_NODE_NAME)) {
      return MXML_CUSTOM;
    }
    if (parent->parent != NULL && !strcmp(parent->parent->value.element.name, DIMS_NODE_NAME)) {
      return MXML_OPAQUE;
    }
    typeString = mxmlElementGetAttr(parent, TYPENAME);
    if (typeString == NULL) {
      /*
        MXML_TEXT seems more appropriate here. But mxml hacks text into
        single words which is not what NeXus wants.
      */
      return MXML_OPAQUE;
    } else {
      if (strstr(typeString, "NX_CHAR") != NULL) {
        return MXML_OPAQUE;
      } else {
        return MXML_CUSTOM;
      }
    }
  }
}
/*----------------------------------------------------------------------*/
int nexusLoadCallback(mxml_node_t *node, const char *buffer) {
  mxml_node_t *parent = NULL;
  int rank, type;
  int64_t iDim[NX_MAXRANK];
  char pNumber[80], *pStart;
  long address, maxAddress;
  pNXDS dataset = NULL;

  parent = node->parent;
  analyzeDataType(parent, &rank, &type, iDim);
  if (iDim[0] == -1 || !strcmp(parent->parent->value.element.name, DIMS_NODE_NAME)) {
    iDim[0] = strlen(buffer);
    node->value.custom.data = strdup(buffer);
    node->value.custom.destroy = free;
    return 0;
  } else {
    node->value.custom.data = createNXDataset(rank, type, iDim);
    dataset = (pNXDS)node->value.custom.data;
    if (dataset == NULL) {
      mxml_error("Failed to allocate custom dataset");
      return 1;
    }
    node->value.custom.destroy = destroyDataset;
  }

  /*
    load data
  */
  pStart = (char *)buffer;
  maxAddress = getNXDatasetLength(dataset);
  address = 0;
  while ((pStart = getNextNumber(pStart, pNumber)) != NULL && address < maxAddress) {
    putNXDatasetValueAt(dataset, address, atof(pNumber));
    address++;
  }

  return 0;
}
/*---------------------------------------------------------------------*/
static void stringIntoBuffer(char **buffer, char **bufPtr, size_t *bufSize, char *string) {
  size_t i;

  for (i = 0; i < strlen(string); i++) {
    myxml_add_char(string[i], bufPtr, buffer, bufSize);
  }
}
/*--------------------------------------------------------------------*/
static void formatNumber(double value, char *txt, int txtLen, char *format, int type) {
  switch (type) {
  case NX_INT8:
  case NX_UINT8:
  case NX_INT16:
  case NX_UINT16:
  case NX_INT32:
  case NX_UINT32:
    snprintf(txt, txtLen, format, (int)value);
    break;
  case NX_INT64:
    snprintf(txt, txtLen, format, (int64_t)value);
    break;
  case NX_UINT64:
    snprintf(txt, txtLen, format, (uint64_t)value);
    break;
  case NX_FLOAT32:
  case NX_FLOAT64:
    snprintf(txt, txtLen, format, value);
    break;
  default:
    /*assert(0);  something is very wrong here */
    printf("Problem\n");
    break;
  }
}
/*--------------------------------------------------------------------*/
static int countDepth(mxml_node_t *node) {
  int count = 0;
  mxml_node_t *cur;

  cur = node;
  while (cur != NULL) {
    count++;
    cur = cur->parent;
  }
  count--;
  return count;
}
/*---------------------------------------------------------------------*/
char *nexusWriteCallback(mxml_node_t *node) {
  int type, col;
  char pNumber[80], indent[80], format[30];
  char *buffer, *bufPtr;
  pNXDS dataset;
  int currentLen, table_style = 0;
  size_t i, bufsize, length;
  int is_definition = 0;
  /* this is set by nxconvert when making a definiton */
  is_definition = (getenv("NX_IS_DEFINITION") != NULL);

  if (!strcmp(node->parent->parent->value.element.name, DATA_NODE_NAME)) {
    table_style = 1;
  }
  /*
    allocate output buffer
  */
  buffer = (char *)malloc(1024 * sizeof(char));
  if (buffer == NULL) {
    mxml_error("Unable to allocate buffer");
    return NULL;
  }
  memset(buffer, 0, 1024);
  bufPtr = buffer;
  bufsize = 1024;

  dataset = (pNXDS)node->value.custom.data;

  /*
    prepare indentation level
  */
  col = countDepth(node) * 2;
  memset(indent, 0, 80);
  for (i = 0; i < col; i++) {
    indent[i] = ' ';
  }

  /*
    get dataset info
  */
  type = getNXDatasetType(dataset);
  if (is_definition) {
    length = 1;
  } else {
    length = getNXDatasetLength(dataset);
  }
  if (dataset->format != NULL) {
    strcpy(format, dataset->format);
  } else {
    getNumberFormat(type, format);
  }

  /*
    actually get the data out
  */
  if (table_style) {
    for (i = 0; i < length; i++) {
      formatNumber(getNXDatasetValueAt(dataset, i), pNumber, 79, format, type);
      stringIntoBuffer(&buffer, &bufPtr, &bufsize, pNumber);
    }
  } else {
    currentLen = col;
    myxml_add_char('\n', &bufPtr, &buffer, &bufsize);
    stringIntoBuffer(&buffer, &bufPtr, &bufsize, indent);
    for (i = 0; i < length; i++) {
      formatNumber(getNXDatasetValueAt(dataset, i), pNumber, 79, format, type);
      if (currentLen + strlen(pNumber) > MXML_WRAP) {
        /*
          wrap line
        */
        myxml_add_char('\n', &bufPtr, &buffer, &bufsize);
        stringIntoBuffer(&buffer, &bufPtr, &bufsize, indent);
        currentLen = col;
      }
      stringIntoBuffer(&buffer, &bufPtr, &bufsize, pNumber);
      myxml_add_char(' ', &bufPtr, &buffer, &bufsize);
      currentLen += strlen(pNumber) + 1;
    }
  }
  myxml_add_char('\0', &bufPtr, &buffer, &bufsize);
  return (char *)buffer;
}
/*------------------------------------------------------------------*/
int isDataNode(mxml_node_t *node) {
  if (mxmlElementGetAttr(node, "name") != NULL) {
    return 0;
  }
  if (strcmp(node->value.element.name, "NXroot") == 0) {
    return 0;
  }
  if (strcmp(node->value.element.name, DIMS_NODE_NAME) == 0) {
    return 0;
  }
  if (strcmp(node->value.element.name, DATA_NODE_NAME) == 0) {
    return 0;
  }
  if (strcmp(node->value.element.name, "NAPIlink") == 0) {
    return 0;
  }
  return 1;
}
/*--------------------------------------------------------------------*/
static int isTextData(mxml_node_t *node) {
  const char *attr = NULL;

  if (!isDataNode(node)) {
    return 0;
  }
  /*
    test datasets
  */
  attr = mxmlElementGetAttr(node, TYPENAME);
  if (attr == NULL) {
    return 1;
  }
  if (strstr(attr, "NX_CHAR") != NULL) {
    return 1;
  } else {
    return 0;
  }
}
/*---------------------------------------------------------------------*/

/*
 * note: not reentrant or thead safe; returns pointer to static storage
 */
const char *NXwhitespaceCallback(mxml_node_t *node, int where) {
  static char *indent = NULL;
  int len;

  if (strstr(node->value.element.name, "?xml") != NULL) {
    return NULL;
  }
  if (node->parent != NULL && !strcmp(node->parent->value.element.name, DATA_NODE_NAME)) {
    return NULL;
  }
  if (where == MXML_WS_BEFORE_CLOSE && !strcmp(node->value.element.name, DATA_NODE_NAME)) {
    return NULL;
  }

  if (isTextData(node)) {
    if (where == MXML_WS_BEFORE_OPEN) {
      len = countDepth(node) * 2 + 2;
      if (indent != NULL) {
        free(indent);
        indent = NULL;
      }
      indent = (char *)malloc(len * sizeof(char));
      if (indent != NULL) {
        memset(indent, ' ', len);
        indent[0] = '\n';
        indent[len - 1] = '\0';
        return (const char *)indent;
      }
    }
    return NULL;
  }

  if (where == MXML_WS_BEFORE_OPEN || where == MXML_WS_BEFORE_CLOSE) {
    len = countDepth(node) * 2 + 2;
    if (indent != NULL) {
      free(indent);
      indent = NULL;
    }
    indent = (char *)malloc(len * sizeof(char));
    if (indent != NULL) {
      memset(indent, ' ', len);
      indent[0] = '\n';
      indent[len - 1] = '\0';
      return (const char *)indent;
    }
  }
  return NULL;
}
/*-----------------------------------------------------------------------*/
#ifdef TESTMAIN
#include <stdio.h>

int main(int argc, char *argv[]) {
  mxml_node_t *root = NULL;
  FILE *f;

  mxmlSetCustomHandlers(nexusLoadCallback, nexusWriteCallback);
  initializeNumberFormats();

  /*
    read test
  */
  f = fopen("dmc.xml", "r");
  root = mxmlLoadFile(NULL, f, nexusTypeCallback);
  fclose(f);

  /*
    write test
  */
  setNumberFormat(NX_INT32, "%8d");
  setNumberFormat(NX_FLOAT32, "%8.2f");
  f = fopen("dmc2.xml", "w");
  mxmlSaveFile(root, f, NXwhitespaceCallback);
  fclose(f);
}
#endif

#endif /*NXXML*/
