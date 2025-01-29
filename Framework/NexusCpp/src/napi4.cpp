/*---------------------------------------------------------------------------
  NeXus - Neutron & X-ray Common Data Format

  Application Program Interface (HDF4) Routines

  Copyright (C) 1997-2006 Mark Koennecke, Przemek Klosowski

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

  $Id$

----------------------------------------------------------------------------*/

#include "MantidNexusCpp/napiconfig.h"

#ifdef WITH_HDF4

#include <assert.h>
#include <map>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// clang-format off
#include "MantidNexusCpp/napi.h"
#include "MantidNexusCpp/napi_internal.h"
#include "MantidNexusCpp/napi4.h"
// clang-format on

extern void *NXpData;

typedef struct __NexusFile {
  struct iStack {
    int32 *iRefDir;
    int32 *iTagDir;
    int32 iVref;
    int32 __iStack_pad;
    int iNDir;
    int iCurDir;
  } iStack[NXMAXSTACK];
  struct iStack iAtt;
  int32 iVID;
  int32 iSID;
  int32 iCurrentVG;
  int32 iCurrentSDS;
  int iNXID;
  int iStackPtr;
  char iAccess[2];
} NexusFile, *pNexusFile;

/*-------------------------------------------------------------------*/

std::map<NXnumtype, int> const nxToHDF4Map{
    {NXnumtype::CHAR, DFNT_CHAR8},    {NXnumtype::INT8, DFNT_INT8},       {NXnumtype::UINT8, DFNT_UINT8},
    {NXnumtype::INT16, DFNT_INT16},   {NXnumtype::UINT16, DFNT_UINT16},   {NXnumtype::INT32, DFNT_INT32},
    {NXnumtype::UINT32, DFNT_UINT32}, {NXnumtype::FLOAT32, DFNT_FLOAT32}, {NXnumtype::FLOAT64, DFNT_FLOAT64}};

static int nxToHDF4Type(NXnumtype type) {
  auto const iter = nxToHDF4Map.find(type);
  if (iter != nxToHDF4Map.cend()) {
    return iter->second;
  } else {
    NXReportError("ERROR: nxToHDF4Type: unknown type");
    return -1;
  }
}

/*-------------------------------------------------------------------*/

static pNexusFile NXIassert(NXhandle fid) {
  pNexusFile pRes;

  assert(fid != NULL);
  pRes = static_cast<pNexusFile>(fid);
  assert(pRes->iNXID == NXSIGNATURE);
  return pRes;
}
/*----------------------------------------------------------------------*/
static int findNapiClass(pNexusFile pFile, int groupRef, NXname nxclass) {
  NXname classText;
  int32 tags[2], attID, linkID, groupID;

  groupID = Vattach(pFile->iVID, groupRef, "r");
  Vgetclass(groupID, classText);
  if (strcmp(classText, "NAPIlink") != 0) {
    /* normal group */
    strcpy(nxclass, classText);
    Vdetach(groupID);
    return groupRef;
  } else {
    /* code for linked renamed groups */
    attID = Vfindattr(groupID, "NAPIlink");
    if (attID >= 0) {
      Vgetattr(groupID, attID, tags);
      linkID = Vattach(pFile->iVID, tags[1], "r");
      NXname linkClass;
      Vgetclass(linkID, linkClass);
      Vdetach(groupID);
      Vdetach(linkID);
      strcpy(nxclass, linkClass);
      return tags[1];
    } else {
      /* this allows for finding the NAPIlink group in NXmakenamedlink */
      strcpy(nxclass, classText);
      Vdetach(groupID);
      return groupRef;
    }
  }
}
/* --------------------------------------------------------------------- */

static int32 NXIFindVgroup(pNexusFile pFile, CONSTCHAR *name, CONSTCHAR *nxclass) {
  int const NX_EOD = static_cast<int>(NXstatus::EOD);
  int32 iNew, iRef, iTag;
  int iN, i;
  int32 *pArray = NULL;
  NXname pText;

  assert(pFile != NULL);

  if (pFile->iCurrentVG == 0) { /* root level */
    /* get the number and ID's of all lone Vgroups in the file */
    iN = Vlone(pFile->iVID, NULL, 0);
    if (iN == 0) {
      return NX_EOD;
    }
    pArray = static_cast<int32 *>(malloc(static_cast<size_t>(iN) * sizeof(int32)));
    if (!pArray) {
      NXReportError("ERROR: out of memory in NXIFindVgroup");
      return NX_EOD;
    }
    Vlone(pFile->iVID, pArray, iN);

    /* loop and check */
    for (i = 0; i < iN; i++) {
      iNew = Vattach(pFile->iVID, pArray[i], "r");
      Vgetname(iNew, pText);
      Vdetach(iNew);
      if (strcmp(pText, name) == 0) {
        pArray[i] = findNapiClass(pFile, pArray[i], pText);
        if (strcmp(pText, nxclass) == 0) {
          /* found ! */
          iNew = pArray[i];
          free(pArray);
          return iNew;
        }
      }
    }
    /* nothing found */
    free(pArray);
    return NX_EOD;
  } else { /* case in Vgroup */
    iN = Vntagrefs(pFile->iCurrentVG);
    for (i = 0; i < iN; i++) {
      Vgettagref(pFile->iCurrentVG, i, &iTag, &iRef);
      if (iTag == DFTAG_VG) {
        iNew = Vattach(pFile->iVID, iRef, "r");
        Vgetname(iNew, pText);
        Vdetach(iNew);
        if (strcmp(pText, name) == 0) {
          iRef = findNapiClass(pFile, iRef, pText);
          if (strcmp(pText, nxclass) == 0) {
            return iRef;
          }
        }
      }
    } /* end for */
  } /* end else */
  /* not found */
  return NX_EOD;
}

/*----------------------------------------------------------------------*/

static int32 NXIFindSDS(NXhandle fid, CONSTCHAR *name) {
  int const NX_EOD = static_cast<int>(NXstatus::EOD);
  pNexusFile self;
  int32 iNew, iRet, iTag, iRef;
  int32 i, iN, iA, iD1, iD2;
  NXname pNam;
  int32 iDim[H4_MAX_VAR_DIMS];

  self = NXIassert(fid);

  /* root level search */
  if (self->iCurrentVG == 0) {
    i = SDfileinfo(self->iSID, &iN, &iA);
    if (i < 0) {
      NXReportError("ERROR: failure to read file information");
      return NX_EOD;
    }
    for (i = 0; i < iN; i++) {
      iNew = SDselect(self->iSID, i);
      SDgetinfo(iNew, pNam, &iA, iDim, &iD1, &iD2);
      if (strcmp(pNam, name) == 0) {
        iRet = SDidtoref(iNew);
        SDendaccess(iNew);
        return iRet;
      } else {
        SDendaccess(iNew);
      }
    }
    /* not found */
    return NX_EOD;
  }
  /* end root level */
  else { /* search in a Vgroup */
    iN = Vntagrefs(self->iCurrentVG);
    for (i = 0; i < iN; i++) {
      Vgettagref(self->iCurrentVG, i, &iTag, &iRef);
      /* we are now writing using DFTAG_NDG, but need others for backward compatability */
      if ((iTag == DFTAG_SDG) || (iTag == DFTAG_NDG) || (iTag == DFTAG_SDS)) {
        iNew = SDreftoindex(self->iSID, iRef);
        iNew = SDselect(self->iSID, iNew);
        SDgetinfo(iNew, pNam, &iA, iDim, &iD1, &iD2);
        if (strcmp(pNam, name) == 0) {
          SDendaccess(iNew);
          return iRef;
        }
        SDendaccess(iNew);
      }
    } /* end for */
  } /* end Vgroup */
  /* we get here, only if nothing found */
  return NX_EOD;
}

/*----------------------------------------------------------------------*/

static NXstatus NXIInitDir(pNexusFile self) {
  int32 iTag, iRef;
  int iStackPtr;

  /*
   * Note: the +1 to various malloc() operations is to avoid a
   *  malloc(0), which is an error on some operating systems
   */
  iStackPtr = self->iStackPtr;
  if (self->iCurrentVG == 0 && self->iStack[iStackPtr].iRefDir == NULL) { /* root level */
    /* get the number and ID's of all lone Vgroups in the file */
    self->iStack[iStackPtr].iNDir = Vlone(self->iVID, NULL, 0);
    self->iStack[iStackPtr].iRefDir =
        static_cast<int32 *>(malloc(static_cast<size_t>(self->iStack[iStackPtr].iNDir) * sizeof(int32) + 1));
    if (!self->iStack[iStackPtr].iRefDir) {
      NXReportError("ERROR: out of memory in NXIInitDir");
      return NXstatus::EOD;
    }
    Vlone(self->iVID, self->iStack[self->iStackPtr].iRefDir, self->iStack[self->iStackPtr].iNDir);
  } else {
    /* Vgroup level */
    self->iStack[iStackPtr].iNDir = Vntagrefs(self->iCurrentVG);
    self->iStack[iStackPtr].iRefDir =
        static_cast<int32 *>(malloc(static_cast<size_t>(self->iStack[iStackPtr].iNDir) * sizeof(int32) + 1));
    self->iStack[iStackPtr].iTagDir =
        static_cast<int32 *>(malloc(static_cast<size_t>(self->iStack[iStackPtr].iNDir) * sizeof(int32) + 1));
    if ((!self->iStack[iStackPtr].iRefDir) || (!self->iStack[iStackPtr].iTagDir)) {
      NXReportError("ERROR: out of memory in NXIInitDir");
      return NXstatus::EOD;
    }
    for (int i = 0; i < self->iStack[self->iStackPtr].iNDir; i++) {
      Vgettagref(self->iCurrentVG, i, &iTag, &iRef);
      self->iStack[iStackPtr].iRefDir[i] = iRef;
      self->iStack[iStackPtr].iTagDir[i] = iTag;
    }
  }
  self->iStack[iStackPtr].iCurDir = 0;
  return NXstatus::OKAY;
}

/*----------------------------------------------------------------------*/

static void NXIKillDir(pNexusFile self) {
  if (self->iStack[self->iStackPtr].iRefDir) {
    free(self->iStack[self->iStackPtr].iRefDir);
    self->iStack[self->iStackPtr].iRefDir = NULL;
  }
  if (self->iStack[self->iStackPtr].iTagDir) {
    free(self->iStack[self->iStackPtr].iTagDir);
    self->iStack[self->iStackPtr].iTagDir = NULL;
  }
  self->iStack[self->iStackPtr].iCurDir = 0;
  self->iStack[self->iStackPtr].iNDir = 0;
}

/*-------------------------------------------------------------------------*/

static NXstatus NXIInitAttDir(pNexusFile pFile) {
  int iRet;
  int32 iData, iAtt, iRank, iType;
  int32 iDim[H4_MAX_VAR_DIMS];

  pFile->iAtt.iCurDir = 0;
  if (pFile->iCurrentSDS != 0) { /* SDS level */
    NXname pNam;
    iRet = SDgetinfo(pFile->iCurrentSDS, pNam, &iRank, iDim, &iType, &iAtt);
  } else {
    if (pFile->iCurrentVG == 0) {
      /* global level */
      iRet = SDfileinfo(pFile->iSID, &iData, &iAtt);
    } else {
      /* group attribute */
      iRet = Vnattrs(pFile->iCurrentVG);
      iAtt = iRet;
    }
  }
  if (iRet < 0) {
    NXReportError("ERROR: HDF cannot read attribute numbers");
    pFile->iAtt.iNDir = 0;
    return NXstatus::ERROR;
  }
  pFile->iAtt.iNDir = iAtt;
  return NXstatus::OKAY;
}

/* --------------------------------------------------------------------- */

static void NXIKillAttDir(pNexusFile self) {
  if (self->iAtt.iRefDir) {
    free(self->iAtt.iRefDir);
    self->iAtt.iRefDir = NULL;
  }
  if (self->iAtt.iTagDir) {
    free(self->iAtt.iTagDir);
    self->iAtt.iTagDir = NULL;
  }
  self->iAtt.iCurDir = 0;
  self->iAtt.iNDir = 0;
}
/*------------------------------------------------------------------*/
static void NXIbuildPath(pNexusFile pFile, char *buffer, int bufLen) {
  int i;
  int32 groupID, iA, iD1, iD2, iDim[H4_MAX_VAR_DIMS];
  NXname pText;

  buffer[0] = '\0';
  for (i = 1; i <= pFile->iStackPtr; i++) {
    strncat(buffer, "/", bufLen - strlen(buffer));
    groupID = Vattach(pFile->iVID, pFile->iStack[i].iVref, "r");
    if (groupID != -1) {
      if (Vgetname(groupID, pText) != -1) {
        strncat(buffer, pText, bufLen - strlen(buffer));
      } else {
        NXReportError("ERROR: NXIbuildPath cannot get vgroup name");
      }
      Vdetach(groupID);
    } else {
      NXReportError("ERROR: NXIbuildPath cannot attach to vgroup");
    }
  }
  if (pFile->iCurrentSDS != 0) {
    if (SDgetinfo(pFile->iCurrentSDS, pText, &iA, iDim, &iD1, &iD2) != -1) {
      strncat(buffer, "/", bufLen - strlen(buffer));
      strncat(buffer, pText, bufLen - strlen(buffer));
    } else {
      NXReportError("ERROR: NXIbuildPath cannot read SDS");
    }
  }
}
/* ----------------------------------------------------------------------

                        Definition of NeXus API

 ---------------------------------------------------------------------*/

NXstatus NX4open(CONSTCHAR *filename, NXaccess am, NXhandle *pHandle) {
  pNexusFile pNew = NULL;
  char pBuffer[512];
  char *time_puffer = NULL;
  char HDF_VERSION[64];
  uint32 lmajor, lminor, lrelease;
  int32 am1 = 0;

  *pHandle = NULL;

  /* mask off any options for now */
  am = (NXaccess)(am & NXACCMASK_REMOVEFLAGS);
  /* map Nexus NXaccess types to HDF4 types */
  if (am == NXACC_CREATE) {
    am1 = DFACC_CREATE;
  } else if (am == NXACC_CREATE4) {
    am1 = DFACC_CREATE;
  } else if (am == NXACC_READ) {
    am1 = DFACC_READ;
  } else if (am == NXACC_RDWR) {
    am1 = DFACC_RDWR;
  }
  /* get memory */
  pNew = static_cast<pNexusFile>(malloc(sizeof(NexusFile)));
  if (!pNew) {
    NXReportError("ERROR: no memory to create File datastructure");
    return NXstatus::ERROR;
  }
  memset(pNew, 0, sizeof(NexusFile));

#if WRITE_OLD_IDENT /* not used at moment */
                    /*
                     * write something that can be used by OLE
                     */

  if (am == NXACC_CREATE || am == NXACC_CREATE4) {
    if ((file_id = Hopen(filename, am1, 0)) == -1) {
      sprintf(pBuffer, "ERROR: cannot open file_a: %s", filename);
      NXReportError(pBuffer);
      free(pNew);
      return NXstatus::ERROR;
    }
    an_id = ANstart(file_id);
    ann_id = ANcreatef(an_id, AN_FILE_LABEL); /* AN_FILE_DESC */
    ANwriteann(ann_id, "NeXus", 5);
    ANendaccess(ann_id);
    ANend(an_id);
    if (Hclose(file_id) == -1) {
      sprintf(pBuffer, "ERROR: cannot close file: %s", filename);
      NXReportError(pBuffer);
      free(pNew);
      return NXstatus::ERROR;
    }
    am = NXACC_RDWR;
  }
#endif /* WRITE_OLD_IDENT */

  /* start SDS interface */
  pNew->iSID = SDstart(filename, am1);
  if (pNew->iSID <= 0) {
    sprintf(pBuffer, "ERROR: cannot open file_b: %s", filename);
    NXReportError(pBuffer);
    free(pNew);
    return NXstatus::ERROR;
  }
  /*
   * need to create global attributes         file_name file_time NeXus_version
   * at some point for new files
   */
  if (am == NXACC_CREATE || am == NXACC_CREATE4) {
    /* set the NeXus_version attribute*/
    if (SDsetattr(pNew->iSID, "NeXus_version", DFNT_CHAR8, static_cast<int32>(strlen(NEXUS_VERSION)), NEXUS_VERSION) <
        0) {
      NXReportError("ERROR: HDF failed to store NeXus_version attribute ");
      free(pNew);
      return NXstatus::ERROR;
    }

    /* set the HDF4 version attribute */
    Hgetlibversion(&lmajor, &lminor, &lrelease, HDF_VERSION);
    if (SDsetattr(pNew->iSID, "HDF_version", DFNT_CHAR8, static_cast<int32>(strlen(HDF_VERSION)), HDF_VERSION) < 0) {
      NXReportError("ERROR: HDF failed to store HDF_version attribute ");
      free(pNew);
      return NXstatus::ERROR;
    }

    /* set the filename attribute */
    if (SDsetattr(pNew->iSID, "file_name", DFNT_CHAR8, static_cast<int32>(strlen(filename)),
                  static_cast<const char *>(filename)) < 0) {
      NXReportError("ERROR: HDF failed to store file_name attribute ");
      free(pNew);
      return NXstatus::ERROR;
    }

    /* set the file_time attribute */
    time_puffer = NXIformatNeXusTime();
    if (time_puffer != NULL) {
      if (SDsetattr(pNew->iSID, "file_time", DFNT_CHAR8, static_cast<int32>(strlen(time_puffer)), time_puffer) < 0) {
        NXReportError("ERROR: HDF failed to store file_time attribute ");
        free(pNew);
        free(time_puffer);
        return NXstatus::ERROR;
      }
      free(time_puffer);
    }

    if (SDsetattr(pNew->iSID, "NX_class", DFNT_CHAR8, 7, "NXroot") < 0) {
      NXReportError("ERROR: HDF failed to store NX_class attribute ");
      free(pNew);
      return NXstatus::ERROR;
    }
  }

  /*
   * Otherwise we try to create the file two times which makes HDF
   * Throw up on us.
   */
  // cppcheck-suppress duplicateCondition
  if (am == NXACC_CREATE || am == NXACC_CREATE4) {
    am = NXACC_RDWR;
    am1 = DFACC_RDWR;
  }

  /* Set Vgroup access mode */
  if (am == NXACC_READ) {
    strcpy(pNew->iAccess, "r");
  } else {
    strcpy(pNew->iAccess, "w");
  }

  /* start Vgroup API */

  pNew->iVID = Hopen(filename, am1, 100);
  if (pNew->iVID <= 0) {
    sprintf(pBuffer, "ERROR: cannot open file_c: %s", filename);
    NXReportError(pBuffer);
    free(pNew);
    return NXstatus::ERROR;
  }
  Vstart(pNew->iVID);
  pNew->iNXID = NXSIGNATURE;
  pNew->iStack[0].iVref = 0; /* root! */

  *pHandle = static_cast<NXhandle>(pNew);
  return NXstatus::OKAY;
}

/*-----------------------------------------------------------------------*/

NXstatus NX4close(NXhandle *fid) {
  pNexusFile pFile = NULL;
  int iRet;

  pFile = NXIassert(*fid);
  iRet = 0;
  /* close links into vGroups or SDS */
  if (pFile->iCurrentVG != 0) {
    Vdetach(pFile->iCurrentVG);
  }
  if (pFile->iCurrentSDS != 0) {
    iRet = SDendaccess(pFile->iCurrentSDS);
  }
  if (iRet < 0) {
    NXReportError("ERROR: ending access to SDS");
  }
  /* close the SDS and Vgroup API's */
  Vend(pFile->iVID);
  iRet = SDend(pFile->iSID);
  if (iRet < 0) {
    NXReportError("ERROR: HDF cannot close SDS interface");
  }
  iRet = Hclose(pFile->iVID);
  if (iRet < 0) {
    NXReportError("ERROR: HDF cannot close HDF file");
  }
  /* release memory */
  NXIKillDir(pFile);
  free(pFile);
  *fid = NULL;
  return NXstatus::OKAY;
}

/*-----------------------------------------------------------------------*/

NXstatus NX4makegroup(NXhandle fid, CONSTCHAR *name, CONSTCHAR *nxclass) {
  pNexusFile pFile;
  int32 iNew, iRet;
  char pBuffer[256];

  pFile = NXIassert(fid);
  /*
   * Make sure that a group with the same name and nxclass does not
   * already exist.
   */
  if ((iRet = NXIFindVgroup(pFile, static_cast<const char *>(name), nxclass)) >= 0) {
    sprintf(pBuffer, "ERROR: Vgroup %s, class %s already exists", name, nxclass);
    NXReportError(pBuffer);
    return NXstatus::ERROR;
  }

  /* create and configure the group */
  iNew = Vattach(pFile->iVID, -1, "w");
  if (iNew < 0) {
    sprintf(pBuffer, "ERROR: HDF could not create Vgroup %s, class %s", name, nxclass);
    NXReportError(pBuffer);
    return NXstatus::ERROR;
  }
  Vsetname(iNew, name);
  Vsetclass(iNew, nxclass);

  /* Insert it into the hierarchy, when appropriate */
  iRet = 0;
  if (pFile->iCurrentVG != 0) {
    iRet = Vinsert(pFile->iCurrentVG, iNew);
  }
  Vdetach(iNew);
  if (iRet < 0) {
    NXReportError("ERROR: HDF failed to insert Vgroup");
    return NXstatus::ERROR;
  }
  return NXstatus::OKAY;
}

/*------------------------------------------------------------------------*/
NXstatus NX4opengroup(NXhandle fid, CONSTCHAR *name, CONSTCHAR *nxclass) {
  pNexusFile pFile;
  int32 iRef;

  pFile = NXIassert(fid);

  iRef = NXIFindVgroup(pFile, static_cast<const char *>(name), nxclass);
  if (iRef < 0) {
    char pBuffer[256];
    sprintf(pBuffer, "ERROR: Vgroup \"%s\", class \"%s\" NOT found", name, nxclass);
    NXReportError(pBuffer);
    return NXstatus::ERROR;
  }
  /* are we at root level ? */
  if (pFile->iCurrentVG == 0) {
    pFile->iCurrentVG = Vattach(pFile->iVID, iRef, pFile->iAccess);
    pFile->iStackPtr++;
    pFile->iStack[pFile->iStackPtr].iVref = iRef;
  } else {
    Vdetach(pFile->iCurrentVG);
    pFile->iStackPtr++;
    pFile->iStack[pFile->iStackPtr].iVref = iRef;
    pFile->iCurrentVG = Vattach(pFile->iVID, pFile->iStack[pFile->iStackPtr].iVref, pFile->iAccess);
  }
  NXIKillDir(pFile);
  return NXstatus::OKAY;
}
/* ------------------------------------------------------------------- */

NXstatus NX4closegroup(NXhandle fid) {
  pNexusFile pFile;

  pFile = NXIassert(fid);

  /* first catch the trivial case: we are at root and cannot get
     deeper into a negative directory hierarchy (anti-directory)
   */
  if (pFile->iCurrentVG == 0) {
    NXIKillDir(pFile);
    return NXstatus::OKAY;
  } else { /* Sighhh. Some work to do */
    /* close the current VG and decrement stack */
    Vdetach(pFile->iCurrentVG);
    NXIKillDir(pFile);
    pFile->iStackPtr--;
    if (pFile->iStackPtr <= 0) { /* we hit root */
      pFile->iStackPtr = 0;
      pFile->iCurrentVG = 0;
    } else {
      /* attach to the lower Vgroup */
      pFile->iCurrentVG = Vattach(pFile->iVID, pFile->iStack[pFile->iStackPtr].iVref, pFile->iAccess);
    }
  }
  return NXstatus::OKAY;
}

/* --------------------------------------------------------------------- */

NXstatus NX4makedata64(NXhandle fid, CONSTCHAR *name, NXnumtype datatype, int rank, int64_t dimensions[]) {
  pNexusFile pFile;
  int32 iNew;
  char pBuffer[256];
  int i, iRet, type;
  int32 myDim[H4_MAX_VAR_DIMS];

  pFile = NXIassert(fid);

  if (dimensions[0] == NX_UNLIMITED) {
    dimensions[0] = SD_UNLIMITED;
  }

  if ((iNew = NXIFindSDS(fid, name)) >= 0) {
    sprintf(pBuffer, "ERROR: SDS %s already exists at this level", name);
    NXReportError(pBuffer);
    return NXstatus::ERROR;
  }

  type = nxToHDF4Type(datatype);

  if (rank <= 0) {
    sprintf(pBuffer, "ERROR: invalid rank specified for SDS %s", name);
    NXReportError(pBuffer);
    return NXstatus::ERROR;
  }

  /*
    Check dimensions for consistency. The first dimension may be 0
    thus denoting an unlimited dimension.
  */
  for (i = 1; i < rank; i++) {
    if (dimensions[i] <= 0) {
      sprintf(pBuffer, "ERROR: invalid dimension %d, value %lld given for SDS %s", i, (long long)dimensions[i], name);
      NXReportError(pBuffer);
      return NXstatus::ERROR;
    }
  }

  /* cast the dimensions array properly for non 32-bit ints */
  for (i = 0; i < rank; i++) {
    myDim[i] = (int32)dimensions[i];
  }

  /* behave nicely, if there is still an SDS open */
  if (pFile->iCurrentSDS != 0) {
    SDendaccess(pFile->iCurrentSDS);
    pFile->iCurrentSDS = 0;
  }

  /* Do not allow creation of SDS's at the root level */
  if (pFile->iCurrentVG == 0) {
    sprintf(pBuffer, "ERROR: SDS creation at root level is not permitted");
    NXReportError(pBuffer);
    return NXstatus::ERROR;
  }

  /* dataset creation */
  iNew =
      SDcreate(pFile->iSID, static_cast<const char *>(name), static_cast<int32>(type), static_cast<int32>(rank), myDim);
  if (iNew < 0) {
    sprintf(pBuffer, "ERROR: cannot create SDS %s, check arguments", name);
    NXReportError(pBuffer);
    return NXstatus::ERROR;
  }
  /* link into Vgroup, if in one */
  if (pFile->iCurrentVG != 0) {
    Vaddtagref(pFile->iCurrentVG, DFTAG_NDG, SDidtoref(iNew));
  }
  iRet = SDendaccess(iNew);
  if (iRet < 0) {
    NXReportError("ERROR: HDF cannot end access to SDS");
    return NXstatus::ERROR;
  }
  return NXstatus::OKAY;
}

/* --------------------------------------------------------------------- */

NXstatus NX4compmakedata64(NXhandle fid, CONSTCHAR *name, NXnumtype datatype, int rank, int64_t dimensions[],
                           int compress_type, int64_t const chunk_size[]) {
  UNUSED_ARG(chunk_size);
  pNexusFile pFile;
  int32 iNew, iRet, type;
  char pBuffer[256];
  int i, compress_level;
  int32 myDim[H4_MAX_VAR_DIMS];
  comp_info compstruct;

  pFile = NXIassert(fid);

  if (dimensions[0] == NX_UNLIMITED) {
    dimensions[0] = SD_UNLIMITED;
  }

  if ((iNew = NXIFindSDS(fid, name)) >= 0) {
    sprintf(pBuffer, "ERROR: SDS %s already exists at this level", name);
    NXReportError(pBuffer);
    return NXstatus::ERROR;
  }

  type = nxToHDF4Type(datatype);

  if (rank <= 0) {
    sprintf(pBuffer, "ERROR: invalid rank specified for SDS %s", name);
    NXReportError(pBuffer);
    return NXstatus::ERROR;
  }

  /*
    Check dimensions for consistency. The first dimension may be 0
    thus denoting an unlimited dimension.
  */
  for (i = 1; i < rank; i++) {
    if (dimensions[i] <= 0) {
      sprintf(pBuffer, "ERROR: invalid dimension %d, value %lld given for SDS %s", i, (long long)dimensions[i], name);
      NXReportError(pBuffer);
      return NXstatus::ERROR;
    }
  }

  /* cast the dimensions array properly for non 32-bit ints */
  for (i = 0; i < rank; i++) {
    myDim[i] = (int32)dimensions[i];
  }

  /* behave nicely, if there is still an SDS open */
  if (pFile->iCurrentSDS != 0) {
    SDendaccess(pFile->iCurrentSDS);
    pFile->iCurrentSDS = 0;
  }

  /* Do not allow creation of SDS's at the root level */
  if (pFile->iCurrentVG == 0) {
    sprintf(pBuffer, "ERROR: SDS creation at root level is not permitted");
    NXReportError(pBuffer);
    return NXstatus::ERROR;
  }

  /* dataset creation */
  iNew =
      SDcreate(pFile->iSID, static_cast<const char *>(name), static_cast<int32>(type), static_cast<int32>(rank), myDim);
  if (iNew < 0) {
    sprintf(pBuffer, "ERROR: cannot create SDS %s, check arguments", name);
    NXReportError(pBuffer);
    return NXstatus::ERROR;
  }

  /* compress SD data set */
  compress_level = 6;
  if ((compress_type / 100) == NX_COMP_LZW) {
    compress_level = compress_type % 100;
    compress_type = NX_COMP_LZW;
  }

  if (compress_type == NX_COMP_LZW) {
    compstruct.deflate.level = compress_level;
    iRet = SDsetcompress(iNew, COMP_CODE_DEFLATE, &compstruct);
    if (iRet < 0) {
      NXReportError("Deflate-Compression failure!");
      return NXstatus::ERROR;
    }
  } else if (compress_type == NX_COMP_RLE) {
    iRet = SDsetcompress(iNew, COMP_CODE_RLE, &compstruct);
    if (iRet < 0) {
      NXReportError("RLE-Compression failure!");
      return NXstatus::ERROR;
    }
  } else if (compress_type == NX_COMP_HUF) {
    compstruct.skphuff.skp_size = DFKNTsize(type);
    iRet = SDsetcompress(iNew, COMP_CODE_SKPHUFF, &compstruct);
    if (iRet < 0) {
      NXReportError("HUF-Compression failure!");
      return NXstatus::ERROR;
    }
  } else if (compress_type == NX_COMP_NONE) {
    /*      */
  } else {
    NXReportError("Unknown compression method!");
    return NXstatus::ERROR;
  }
  /* link into Vgroup, if in one */
  if (pFile->iCurrentVG != 0) {
    Vaddtagref(pFile->iCurrentVG, DFTAG_NDG, SDidtoref(iNew));
  }
  iRet = SDendaccess(iNew);
  if (iRet < 0) {
    NXReportError("ERROR: HDF cannot end access to SDS");
    return NXstatus::ERROR;
  }

  return NXstatus::OKAY;
}

/* --------------------------------------------------------------------- */

NXstatus NX4compress(NXhandle fid, int compress_type) {
  pNexusFile pFile;
  int32 iRank, iAtt, iType;
  int32 iSize[H4_MAX_VAR_DIMS];
  comp_coder_t compress_typei = COMP_CODE_NONE;
  NXname pBuffer;
  comp_info compstruct;
  int compress_level = 6;

  pFile = NXIassert(fid);

  /* check if there is an SDS open */
  if (pFile->iCurrentSDS == 0) {
    NXReportError("ERROR: no SDS open");
    return NXstatus::ERROR;
  }

  if (compress_type == NX_COMP_NONE) {
    compress_typei = COMP_CODE_NONE;
  } else if (compress_type == NX_COMP_LZW) {
    compress_typei = COMP_CODE_DEFLATE;
  } else if ((compress_type / 100) == NX_COMP_LZW) {
    compress_typei = COMP_CODE_DEFLATE;
    compress_level = compress_type % 100;
    compress_type = NX_COMP_LZW;
  } else if (compress_type == NX_COMP_RLE) {
    compress_typei = COMP_CODE_RLE;
  } else if (compress_type == NX_COMP_HUF) {
    compress_typei = COMP_CODE_SKPHUFF;
  }

  /* first read dimension information */
  SDgetinfo(pFile->iCurrentSDS, pBuffer, &iRank, iSize, &iType, &iAtt);

  /*
     according to compression type initialize compression
     information
  */
  if (compress_type == NX_COMP_LZW) {
    compstruct.deflate.level = compress_level;
  } else if (compress_type == NX_COMP_HUF) {
    compstruct.skphuff.skp_size = DFKNTsize(iType);
  }

  const int32 iRet = SDsetcompress(pFile->iCurrentSDS, compress_typei, &compstruct);
  if (iRet < 0) {
    char pError[512];
    sprintf(pError, "ERROR: failure to compress data to %s", pBuffer);
    NXReportError(pError);
    return NXstatus::ERROR;
  }
  return NXstatus::OKAY;
}

/* --------------------------------------------------------------------- */

NXstatus NX4opendata(NXhandle fid, CONSTCHAR *name) {
  pNexusFile pFile;
  int32 iNew, attID, tags[2];
  pFile = NXIassert(fid);

  /* First find the reference number of the SDS */
  iNew = NXIFindSDS(fid, name);
  if (iNew < 0) {
    char pBuffer[256];
    sprintf(pBuffer, "ERROR: SDS \"%s\" not found at this level", name);
    NXReportError(pBuffer);
    return NXstatus::ERROR;
  }
  /* Be nice: properly close the old open SDS silently if there is
   * still an SDS open.
   */
  if (pFile->iCurrentSDS) {
    const int iRet = SDendaccess(pFile->iCurrentSDS);
    if (iRet < 0) {
      NXReportError("ERROR: HDF cannot end access to SDS");
    }
  }
  /* clear pending attribute directories first */
  NXIKillAttDir(pFile);

  /* open the SDS, thereby watching for linked SDS under a different name */
  iNew = SDreftoindex(pFile->iSID, iNew);
  pFile->iCurrentSDS = SDselect(pFile->iSID, iNew);
  attID = SDfindattr(pFile->iCurrentSDS, "NAPIlink");
  if (attID >= 0) {
    SDreadattr(pFile->iCurrentSDS, attID, tags);
    SDendaccess(pFile->iCurrentSDS);
    iNew = SDreftoindex(pFile->iSID, tags[1]);
    pFile->iCurrentSDS = SDselect(pFile->iSID, iNew);
  }

  if (pFile->iCurrentSDS < 0) {
    NXReportError("ERROR: HDF error opening SDS");
    pFile->iCurrentSDS = 0;
    return NXstatus::ERROR;
  }
  return NXstatus::OKAY;
}

/* ----------------------------------------------------------------- */

NXstatus NX4closedata(NXhandle fid) {
  pNexusFile pFile;
  pFile = NXIassert(fid);

  if (pFile->iCurrentSDS != 0) {
    int iRet = SDendaccess(pFile->iCurrentSDS);
    pFile->iCurrentSDS = 0;
    if (iRet < 0) {
      NXReportError("ERROR: HDF cannot end access to SDS");
      return NXstatus::ERROR;
    }
  } else {
    NXReportError("ERROR: no SDS open --> nothing to do");
    return NXstatus::ERROR;
  }
  NXIKillAttDir(pFile); /* for attribute data */
  return NXstatus::OKAY;
}

/* ------------------------------------------------------------------- */

NXstatus NX4putdata(NXhandle fid, const void *data) {
  pNexusFile pFile;
  int32 iStart[H4_MAX_VAR_DIMS], iSize[H4_MAX_VAR_DIMS], iStride[H4_MAX_VAR_DIMS];
  NXname pBuffer;
  int32 iRank, iAtt, iType;

  pFile = NXIassert(fid);

  /* check if there is an SDS open */
  if (pFile->iCurrentSDS == 0) {
    NXReportError("ERROR: no SDS open");
    return NXstatus::ERROR;
  }
  /* first read dimension information */
  memset(iStart, 0, H4_MAX_VAR_DIMS * sizeof(int32));
  SDgetinfo(pFile->iCurrentSDS, pBuffer, &iRank, iSize, &iType, &iAtt);

  /* initialise stride to 1 */
  for (int i = 0; i < iRank; i++) {
    iStride[i] = 1;
  }

  /* actually write */
  int32 iRet =
      SDwritedata(pFile->iCurrentSDS, iStart, iStride, iSize, const_cast<void *>(static_cast<const void *>(data)));
  if (iRet < 0) {
    /* HEprint(stdout,0); */
    char pError[512];
    sprintf(pError, "ERROR: failure to write data to %s", pBuffer);
    NXReportError(pError);
    return NXstatus::ERROR;
  }
  return NXstatus::OKAY;
}

/* ------------------------------------------------------------------- */

NXstatus NX4putattr(NXhandle fid, CONSTCHAR *name, const void *data, int datalen, NXnumtype iType) {
  pNexusFile pFile;
  int iRet, type;

  pFile = NXIassert(fid);
  type = nxToHDF4Type(iType);
  if (pFile->iCurrentSDS != 0) {
    /* SDS attribute */
    iRet = SDsetattr(pFile->iCurrentSDS, static_cast<const char *>(name), static_cast<int32>(type),
                     static_cast<int32>(datalen), data);
  } else {
    if (pFile->iCurrentVG == 0) {
      /* global attribute */
      iRet = SDsetattr(pFile->iSID, static_cast<const char *>(name), static_cast<int32>(type),
                       static_cast<int32>(datalen), data);
    } else {
      /* group attribute */
      iRet = Vsetattr(pFile->iCurrentVG, static_cast<const char *>(name), static_cast<int32>(type),
                      static_cast<int32>(datalen), data);
    }
  }
  // iType = type; // TODO what is the intention here?
  if (iRet < 0) {
    NXReportError("ERROR: HDF failed to store attribute ");
    return NXstatus::ERROR;
  }
  return NXstatus::OKAY;
}

/* ------------------------------------------------------------------- */

NXstatus NX4putslab64(NXhandle fid, const void *data, const int64_t iStart[], const int64_t iSize[]) {
  pNexusFile pFile;
  int iRet;
  int32 iStride[H4_MAX_VAR_DIMS];
  int32 myStart[H4_MAX_VAR_DIMS], mySize[H4_MAX_VAR_DIMS];
  int32 i, iRank, iType, iAtt;
  NXname pBuffer;

  pFile = NXIassert(fid);

  /* check if there is an SDS open */
  if (pFile->iCurrentSDS == 0) {
    NXReportError("ERROR: no SDS open");
    return NXstatus::ERROR;
  }
  /* initialise stride to 1 */
  for (i = 0; i < H4_MAX_VAR_DIMS; i++) {
    iStride[i] = 1;
  }

  SDgetinfo(pFile->iCurrentSDS, pBuffer, &iRank, myStart, &iType, &iAtt);
  for (i = 0; i < iRank; i++) {
    myStart[i] = (int32)iStart[i];
    mySize[i] = (int32)iSize[i];
  }
  /* finally write */
  iRet = SDwritedata(pFile->iCurrentSDS, myStart, iStride, mySize, const_cast<void *>(static_cast<const void *>(data)));

  /* deal with HDF errors */
  if (iRet < 0) {
    NXReportError("ERROR: writing slab failed");
    return NXstatus::ERROR;
  }
  return NXstatus::OKAY;
}

/* ------------------------------------------------------------------- */

NXstatus NX4getdataID(NXhandle fid, NXlink *sRes) {
  pNexusFile pFile;
  int datalen;
  NXnumtype type = NXnumtype::CHAR;

  pFile = NXIassert(fid);

  if (pFile->iCurrentSDS == 0) {
    sRes->iTag = static_cast<int>(NXstatus::ERROR);
    return NXstatus::ERROR;
  } else {
    sRes->iTag = DFTAG_NDG;
    sRes->iRef = SDidtoref(pFile->iCurrentSDS);
    NXMDisableErrorReporting();
    datalen = 1024;
    memset(&sRes->targetPath, 0, 1024);
    if (NX4getattr(fid, "target", &sRes->targetPath, &datalen, &type) != NXstatus::OKAY) {
      NXIbuildPath(pFile, sRes->targetPath, 1024);
    }
    NXMEnableErrorReporting();
    return NXstatus::OKAY;
  }
  sRes->iTag = static_cast<int>(NXstatus::ERROR);
  return NXstatus::ERROR; /* not reached */
}

/* ------------------------------------------------------------------- */

NXstatus NX4makelink(NXhandle fid, NXlink *sLink) {
  pNexusFile pFile;
  int32 dataID, type = DFNT_CHAR8;
  char name[] = "target";

  pFile = NXIassert(fid);

  if (pFile->iCurrentVG == 0) { /* root level, can not link here */
    return NXstatus::ERROR;
  }
  Vaddtagref(pFile->iCurrentVG, static_cast<int32>(sLink->iTag), static_cast<int32>(sLink->iRef));
  int32 length = static_cast<int32>(strlen(sLink->targetPath));
  if (sLink->iTag == DFTAG_SDG || sLink->iTag == DFTAG_NDG || sLink->iTag == DFTAG_SDS) {
    dataID = SDreftoindex(pFile->iSID, static_cast<int32>(sLink->iRef));
    dataID = SDselect(pFile->iSID, dataID);
    SDsetattr(dataID, name, type, length, sLink->targetPath);
    SDendaccess(dataID);
  } else {
    dataID = Vattach(pFile->iVID, static_cast<int32>(sLink->iRef), "w");
    Vsetattr(dataID, static_cast<char *>(name), type, (int32)length, sLink->targetPath);
    Vdetach(dataID);
  }
  return NXstatus::OKAY;
}
/* ------------------------------------------------------------------- */

NXstatus NX4makenamedlink(NXhandle fid, CONSTCHAR *newname, NXlink *sLink) {
  pNexusFile pFile;
  int32 dataID, type = DFNT_CHAR8, rank = 1;
  NXnumtype dataType = NXnumtype::CHAR, attType = NXnumtype::INT32;
  char name[] = "target";
  int tags[2];

  pFile = NXIassert(fid);

  if (pFile->iCurrentVG == 0) { /* root level, can not link here */
    return NXstatus::ERROR;
  }

  tags[0] = static_cast<int>(sLink->iTag);
  tags[1] = static_cast<int>(sLink->iRef);

  int32 length = static_cast<int32>(strlen(sLink->targetPath));
  if (sLink->iTag == DFTAG_SDG || sLink->iTag == DFTAG_NDG || sLink->iTag == DFTAG_SDS) {
    int64_t iDim[1] = {1};
    NX4makedata64(fid, newname, dataType, rank, iDim);
    NX4opendata(fid, newname);
    NX4putattr(fid, "NAPIlink", tags, 2, attType);
    NX4closedata(fid);
    dataID = SDreftoindex(pFile->iSID, static_cast<int32>(sLink->iRef));
    dataID = SDselect(pFile->iSID, dataID);
    SDsetattr(dataID, name, type, length, sLink->targetPath);
    SDendaccess(dataID);
  } else {
    NX4makegroup(fid, newname, "NAPIlink");
    NX4opengroup(fid, newname, "NAPIlink");
    NX4putattr(fid, "NAPIlink", tags, 2, attType);
    NX4closegroup(fid);
    dataID = Vattach(pFile->iVID, static_cast<int32>(sLink->iRef), "w");
    Vsetattr(dataID, static_cast<char *>(name), type, (int32)length, sLink->targetPath);
    Vdetach(dataID);
  }
  return NXstatus::OKAY;
}

/*----------------------------------------------------------------------*/

NXstatus NX4printlink(NXhandle fid, NXlink const *sLink) {
  NXIassert(fid);
  printf("HDF4 link: iTag = %ld, iRef = %ld, target=\"%s\"\n", sLink->iTag, sLink->iRef, sLink->targetPath);
  return NXstatus::OKAY;
}

/*----------------------------------------------------------------------*/

NXstatus NX4flush(NXhandle *pHandle) {
  char *pFileName, *pCopy = NULL;
  int access, dummy, i, iStack;
  NXstatus iRet;
  pNexusFile pFile = NULL;
  NXaccess ac;
  int *iRefs = NULL;

  pFile = NXIassert(*pHandle);

  /*
    The HDF4-API does not support a flush. We help ourselves with
    inquiring the name and access type of the file, closing it and
    opening it again. This is also the reason why this needs a pointer
    to the handle structure as the handle changes. The other thing we
    do is to store the refs of all open vGroups in a temporary array
    in order to recover the position in the vGroup hierarchy before the
    flush.
  */
  iRet = static_cast<NXstatus>(Hfidinquire(pFile->iVID, &pFileName, &access, &dummy));
  if (iRet == NXstatus::EOD) {
    NXReportError("ERROR: Failed to inquire file name for HDF file");
    return NXstatus::ERROR;
  }
  if (pFile->iAccess[0] == 'r') {
    ac = NXACC_READ;
  } else if (pFile->iAccess[0] == 'w') {
    ac = NXACC_RDWR;
  } else {
    NXReportError("ERROR: NX4flush failed to determine file access mode");
    return NXstatus::ERROR;
  }
  pCopy = static_cast<char *>(malloc((strlen(pFileName) + 10) * sizeof(char)));
  if (!pCopy) {
    NXReportError("ERROR: Failed to allocate data for filename copy");
    return NXstatus::ERROR;
  }
  memset(pCopy, 0, strlen(pFileName) + 10);
  strcpy(pCopy, pFileName);

  /* get refs for recovering vGroup position */
  iStack = 0;
  if (pFile->iStackPtr > 0) {
    iStack = pFile->iStackPtr + 1;
    iRefs = static_cast<int *>(malloc(iStack * sizeof(int)));
    if (!iRefs) {
      free(pCopy); // do not leak memory
      NXReportError("ERROR: Failed to allocate data for hierarchy copy");
      return NXstatus::ERROR;
    }
    for (i = 0; i < iStack; i++) {
      iRefs[i] = pFile->iStack[i].iVref;
    }
  }

  NX4close(pHandle);

  iRet = NX4open(pCopy, ac, pHandle);
  free(pCopy);

  /* return to position in vGroup hierarchy */
  pFile = NXIassert(*pHandle);
  if (iStack > 0) {
    pFile->iStackPtr = iStack - 1;
    for (i = 0; i < iStack; i++) {
      pFile->iStack[i].iVref = iRefs[i];
    }
    free(iRefs);
    pFile->iCurrentVG = Vattach(pFile->iVID, pFile->iStack[pFile->iStackPtr].iVref, pFile->iAccess);
  }

  return iRet;
}

/*-------------------------------------------------------------------------*/

NXstatus NX4getnextentry(NXhandle fid, NXname name, NXname nxclass, NXnumtype *datatype) {
  pNexusFile pFile;
  int iStackPtr, iCurDir;
  int32 iTemp, iD1, iD2, iA;
  int32 iDim[H4_MAX_VAR_DIMS];

  pFile = NXIassert(fid);

  iStackPtr = pFile->iStackPtr;
  iCurDir = pFile->iStack[pFile->iStackPtr].iCurDir;

  /* first case to check for: no directory entry */
  if (pFile->iStack[pFile->iStackPtr].iRefDir == NULL) {
    if (NXIInitDir(pFile) == NXstatus::EOD) {
      NXReportError("ERROR: no memory to store directory info");
      return NXstatus::EOD;
    }
  }

  /* Next case: end of directory */
  if (iCurDir >= pFile->iStack[pFile->iStackPtr].iNDir) {
    NXIKillDir(pFile);
    return NXstatus::EOD;
  }

  /* Next case: we have data! supply it and increment counter */
  if (pFile->iCurrentVG == 0) { /* root level */
    iTemp = Vattach(pFile->iVID, pFile->iStack[iStackPtr].iRefDir[iCurDir], "r");
    if (iTemp < 0) {
      NXReportError("ERROR: HDF cannot attach to Vgroup");
      return NXstatus::ERROR;
    }
    Vgetname(iTemp, name);
    Vdetach(iTemp);
    findNapiClass(pFile, pFile->iStack[pFile->iStackPtr].iRefDir[iCurDir], nxclass);
    *datatype = static_cast<NXnumtype>(DFTAG_VG);
    pFile->iStack[pFile->iStackPtr].iCurDir++;
    return NXstatus::OKAY;
  } else {                                                       /* in Vgroup */
    if (pFile->iStack[iStackPtr].iTagDir[iCurDir] == DFTAG_VG) { /* Vgroup */
      iTemp = Vattach(pFile->iVID, pFile->iStack[iStackPtr].iRefDir[iCurDir], "r");
      if (iTemp < 0) {
        NXReportError("ERROR: HDF cannot attach to Vgroup");
        return NXstatus::ERROR;
      }
      Vgetname(iTemp, name);
      Vdetach(iTemp);
      findNapiClass(pFile, pFile->iStack[pFile->iStackPtr].iRefDir[iCurDir], nxclass);
      *datatype = static_cast<NXnumtype>(DFTAG_VG);
      pFile->iStack[pFile->iStackPtr].iCurDir++;
      Vdetach(iTemp);
      return NXstatus::OKAY;
      /* we are now writing using DFTAG_NDG, but need others for backward compatability */
    } else if ((pFile->iStack[iStackPtr].iTagDir[iCurDir] == DFTAG_SDG) ||
               (pFile->iStack[iStackPtr].iTagDir[iCurDir] == DFTAG_NDG) ||
               (pFile->iStack[iStackPtr].iTagDir[iCurDir] == DFTAG_SDS)) {
      iTemp = SDreftoindex(pFile->iSID, pFile->iStack[iStackPtr].iRefDir[iCurDir]);
      iTemp = SDselect(pFile->iSID, iTemp);
      SDgetinfo(iTemp, name, &iA, iDim, &iD1, &iD2);
      strcpy(nxclass, "SDS");
      *datatype = static_cast<NXnumtype>(iD1);
      SDendaccess(iTemp);
      pFile->iStack[pFile->iStackPtr].iCurDir++;
      return NXstatus::OKAY;
    } else { /* unidentified */
      strcpy(name, "UNKNOWN");
      strcpy(nxclass, "UNKNOWN");
      *datatype = static_cast<NXnumtype>(pFile->iStack[iStackPtr].iTagDir[iCurDir]);
      pFile->iStack[pFile->iStackPtr].iCurDir++;
      return NXstatus::OKAY;
    }
  }
  return NXstatus::ERROR; /* not reached */
}

/*-------------------------------------------------------------------------*/

NXstatus NX4getdata(NXhandle fid, void *data) {
  pNexusFile pFile;
  int32 iStart[H4_MAX_VAR_DIMS], iSize[H4_MAX_VAR_DIMS];
  NXname pBuffer;
  int32 iRank, iAtt, iType;

  pFile = NXIassert(fid);

  /* check if there is an SDS open */
  if (pFile->iCurrentSDS == 0) {
    NXReportError("ERROR: no SDS open");
    return NXstatus::ERROR;
  }
  /* first read dimension information */
  memset(iStart, 0, H4_MAX_VAR_DIMS * sizeof(int32));
  SDgetinfo(pFile->iCurrentSDS, pBuffer, &iRank, iSize, &iType, &iAtt);
  /* actually read */
  SDreaddata(pFile->iCurrentSDS, iStart, NULL, iSize, data);
  return NXstatus::OKAY;
}

/*-------------------------------------------------------------------------*/

NXstatus NX4getinfo64(NXhandle fid, int *rank, int64_t dimension[], NXnumtype *iType) {
  pNexusFile pFile;
  NXname pBuffer;
  int32 iAtt, myDim[H4_MAX_VAR_DIMS], i, iRank, mType;

  pFile = NXIassert(fid);

  /* check if there is an SDS open */
  if (pFile->iCurrentSDS == 0) {
    NXReportError("ERROR: no SDS open");
    return NXstatus::ERROR;
  }
  /* read information */
  SDgetinfo(pFile->iCurrentSDS, pBuffer, &iRank, myDim, &mType, &iAtt);

  /* conversion to proper ints for the platform */
  *iType = static_cast<NXnumtype>(mType);
  *rank = (int)iRank;
  for (i = 0; i < iRank; i++) {
    dimension[i] = (int)myDim[i];
  }
  return NXstatus::OKAY;
}

/*-------------------------------------------------------------------------*/

NXstatus NX4getslab64(NXhandle fid, void *data, const int64_t iStart[], const int64_t iSize[]) {
  pNexusFile pFile;
  int32 myStart[H4_MAX_VAR_DIMS], mySize[H4_MAX_VAR_DIMS];
  int32 i, iRank, iType, iAtt;
  NXname pBuffer;

  pFile = NXIassert(fid);

  /* check if there is an SDS open */
  if (pFile->iCurrentSDS == 0) {
    NXReportError("ERROR: no SDS open");
    return NXstatus::ERROR;
  }

  SDgetinfo(pFile->iCurrentSDS, pBuffer, &iRank, myStart, &iType, &iAtt);
  for (i = 0; i < iRank; i++) {
    myStart[i] = (int32)iStart[i];
    mySize[i] = (int32)iSize[i];
  }
  /* finally read  */
  SDreaddata(pFile->iCurrentSDS, myStart, NULL, mySize, data);
  return NXstatus::OKAY;
}

/*-------------------------------------------------------------------------*/

NXstatus NX4getnextattr(NXhandle fileid, NXname pName, int *iLength, NXnumtype *iType) {
  pNexusFile pFile;
  NXstatus iRet;
  int32 iPType, iCount, count;

  pFile = NXIassert(fileid);

  /* first check if we have to start a new attribute search */
  if (pFile->iAtt.iNDir == 0) {
    iRet = NXIInitAttDir(pFile);
    if (iRet == NXstatus::ERROR) {
      return NXstatus::ERROR;
    }
  }
  /* are we done ? */
  if (pFile->iAtt.iCurDir >= pFile->iAtt.iNDir) {
    NXIKillAttDir(pFile);
    return NXstatus::EOD;
  }
  /* well, there must be data to copy */
  if (pFile->iCurrentSDS == 0) {
    if (pFile->iCurrentVG == 0) {
      /* global attribute */
      iRet = static_cast<NXstatus>(SDattrinfo(pFile->iSID, pFile->iAtt.iCurDir, pName, &iPType, &iCount));
    } else {
      /* group attribute */
      iRet = static_cast<NXstatus>(Vattrinfo(pFile->iCurrentVG, pFile->iAtt.iCurDir, pName, &iPType, &iCount, &count));
    }
  } else {
    iRet = static_cast<NXstatus>(SDattrinfo(pFile->iCurrentSDS, pFile->iAtt.iCurDir, pName, &iPType, &iCount));
  }
  if (iRet == NXstatus::EOD) {
    NXReportError("ERROR: HDF cannot read attribute info");
    return NXstatus::ERROR;
  }
  *iLength = iCount;
  *iType = static_cast<NXnumtype>(iPType);
  pFile->iAtt.iCurDir++;
  return NXstatus::OKAY;
}

/*-------------------------------------------------------------------------*/

NXstatus NX4getattr(NXhandle fid, const char *name, void *data, int *datalen, NXnumtype *iType) {
  pNexusFile pFile;
  int32 iNew, iType32, count;
  void *pData = NULL;
  int32 iLen, iRet;
  int type;
  char pBuffer[256];
  NXname pNam;

  type = nxToHDF4Type(*iType);
  *datalen = (*datalen) * DFKNTsize(type);
  pFile = NXIassert(fid);

  /* find attribute */
  if (pFile->iCurrentSDS != 0) {
    /* SDS attribute */
    iNew = SDfindattr(pFile->iCurrentSDS, name);
  } else {
    if (pFile->iCurrentVG == 0) {
      /* global attribute */
      iNew = SDfindattr(pFile->iSID, name);
    } else {
      /* group attribute */
      iNew = Vfindattr(pFile->iCurrentVG, name);
    }
  }
  if (iNew < 0) {
    sprintf(pBuffer, "ERROR: attribute \"%s\" not found", name);
    NXReportError(pBuffer);
    return NXstatus::ERROR;
  }
  /* get more info, allocate temporary data space */
  iType32 = (int32)type;
  if (pFile->iCurrentSDS != 0) {
    iRet = SDattrinfo(pFile->iCurrentSDS, iNew, pNam, &iType32, &iLen);
  } else {
    if (pFile->iCurrentVG == 0) {
      iRet = SDattrinfo(pFile->iSID, iNew, pNam, &iType32, &iLen);
    } else {
      iRet = Vattrinfo(pFile->iCurrentVG, iNew, pNam, &iType32, &count, &iLen);
    }
  }
  if (iRet < 0) {
    sprintf(pBuffer, "ERROR: HDF could not read attribute info");
    NXReportError(pBuffer);
    return NXstatus::ERROR;
  }
  *iType = static_cast<NXnumtype>(iType32);
  iLen = iLen * DFKNTsize(type);
  if (*iType == NXnumtype::CHAR) {
    iLen += 1;
  }
  pData = malloc(iLen);
  if (!pData) {
    NXReportError("ERROR: allocating memory in NXgetattr");
    return NXstatus::ERROR;
  }
  memset(pData, 0, iLen);

  /* finally read the data */
  if (pFile->iCurrentSDS != 0) {
    iRet = SDreadattr(pFile->iCurrentSDS, iNew, pData);
  } else {
    if (pFile->iCurrentVG == 0) {
      iRet = SDreadattr(pFile->iSID, iNew, pData);
    } else {
      iRet = Vgetattr(pFile->iCurrentVG, iNew, pData);
    }
  }
  if (iRet < 0) {
    sprintf(pBuffer, "ERROR: HDF could not read attribute data");
    NXReportError(pBuffer);
    return NXstatus::ERROR;
  }
  /* copy data to caller */
  memset(data, 0, *datalen);
  if ((*datalen <= iLen) && (type == DFNT_UINT8 || type == DFNT_CHAR8 || type == DFNT_UCHAR8)) {
    iLen = *datalen - 1; /* this enforces NULL termination regardless of size of datalen */
  }
  memcpy(data, pData, iLen);
  *datalen = iLen / DFKNTsize(type);
  free(pData);
  return NXstatus::OKAY;
}

/*-------------------------------------------------------------------------*/

NXstatus NX4getattrinfo(NXhandle fid, int *iN) {
  pNexusFile pFile;
  int iRet;
  int32 iData, iAtt, iRank, iType;
  int32 iDim[H4_MAX_VAR_DIMS];

  pFile = NXIassert(fid);
  if (pFile->iCurrentSDS != 0) { /* SDS level */
    NXname pNam;
    iRet = SDgetinfo(pFile->iCurrentSDS, pNam, &iRank, iDim, &iType, &iAtt);
  } else {
    if (pFile->iCurrentVG == 0) {
      /* global level */
      iRet = SDfileinfo(pFile->iSID, &iData, &iAtt);
    } else {
      iRet = Vnattrs(pFile->iCurrentVG);
      iAtt = iRet;
    }
  }
  if (iRet < 0) {
    NXReportError("NX_ERROR: HDF cannot read attribute numbers");
    *iN = 0;
    return NXstatus::ERROR;
  }
  *iN = iAtt;
  return NXstatus::OKAY;
}

/*-------------------------------------------------------------------------*/

NXstatus NX4getgroupID(NXhandle fileid, NXlink *sRes) {
  pNexusFile pFile;

  pFile = NXIassert(fileid);

  if (pFile->iCurrentVG == 0) {
    sRes->iTag = static_cast<int>(NXstatus::ERROR);
    return NXstatus::ERROR;
  } else {
    sRes->iTag = DFTAG_VG;
    sRes->iRef = VQueryref(pFile->iCurrentVG);
    NXIbuildPath(pFile, sRes->targetPath, 1024);
    return NXstatus::OKAY;
  }
  /* not reached */
  sRes->iTag = static_cast<int>(NXstatus::ERROR);
  return NXstatus::ERROR;
}

/*-------------------------------------------------------------------------*/

NXstatus NX4getgroupinfo(NXhandle fid, int *iN, NXname pName, NXname pClass) {
  pNexusFile pFile;

  pFile = NXIassert(fid);
  /* check if there is a group open */
  if (pFile->iCurrentVG == 0) {
    *iN = Vlone(pFile->iVID, NULL, 0);
    strcpy(pName, "root");
    strcpy(pClass, "NXroot");
  } else {
    *iN = Vntagrefs(pFile->iCurrentVG);
    Vgetname(pFile->iCurrentVG, pName);
    Vgetclass(pFile->iCurrentVG, pClass);
  }
  return NXstatus::OKAY;
}

/* ------------------------------------------------------------------- */

NXstatus NX4sameID(NXhandle fileid, NXlink const *pFirstID, NXlink const *pSecondID) {
  NXIassert(fileid);
  if ((pFirstID->iTag == pSecondID->iTag) & (pFirstID->iRef == pSecondID->iRef)) {
    return NXstatus::OKAY;
  } else {
    return NXstatus::ERROR;
  }
}

/*-------------------------------------------------------------------------*/

NXstatus NX4initattrdir(NXhandle fid) {
  pNexusFile pFile;
  NXstatus iRet;

  pFile = NXIassert(fid);
  NXIKillAttDir(pFile);
  iRet = NXIInitAttDir(pFile);
  if (iRet == NXstatus::ERROR)
    return NXstatus::ERROR;
  return NXstatus::OKAY;
}

/*-------------------------------------------------------------------------*/

NXstatus NX4initgroupdir(NXhandle fid) {
  pNexusFile pFile;
  NXstatus iRet;

  pFile = NXIassert(fid);
  NXIKillDir(pFile);
  iRet = NXIInitDir(pFile);
  if (iRet == NXstatus::EOD) {
    NXReportError("NX_ERROR: no memory to store directory info");
    return NXstatus::EOD;
  }
  return NXstatus::OKAY;
}

/*--------------------------------------------------------------------*/
NXstatus NX4putattra(NXhandle handle, CONSTCHAR *name, const void *data, const int rank, const int dim[],
                     const NXnumtype iType) {
  if (rank > 1) {
    NXReportError("This is a HDF4 file, there is only rudimentary support for attribute arrays wirh rank <=1");
    return NXstatus::ERROR;
  }

  return NX4putattr(handle, name, data, dim[0], iType);
}

/*--------------------------------------------------------------------*/
NXstatus NX4getnextattra(NXhandle handle, NXname pName, int *rank, int dim[], NXnumtype *iType) {
  NXstatus ret = NX4getnextattr(handle, pName, dim, iType);
  if (ret != NXstatus::OKAY)
    return ret;
  (*rank) = 1;
  if (dim[0] <= 1)
    (*rank) = 0;
  return NXstatus::OKAY;
}

/*--------------------------------------------------------------------*/
NXstatus NX4getattra(NXhandle, const char *, void *) {
  NXReportError("This is a HDF4 file, attribute array API is not supported here");
  return NXstatus::ERROR;
}

/*--------------------------------------------------------------------*/
NXstatus NX4getattrainfo(NXhandle, NXname, int *, int[], NXnumtype *) {
  NXReportError("This is a HDF4 file, attribute array API is not supported here");
  return NXstatus::ERROR;
}

/*--------------------------------------------------------------------*/
void NX4assignFunctions(pNexusFunction fHandle) {
  fHandle->nxclose = NX4close;
  fHandle->nxreopen = NULL;
  fHandle->nxflush = NX4flush;
  fHandle->nxmakegroup = NX4makegroup;
  fHandle->nxopengroup = NX4opengroup;
  fHandle->nxclosegroup = NX4closegroup;
  fHandle->nxmakedata64 = NX4makedata64;
  fHandle->nxcompmakedata64 = NX4compmakedata64;
  fHandle->nxcompress = NX4compress;
  fHandle->nxopendata = NX4opendata;
  fHandle->nxclosedata = NX4closedata;
  fHandle->nxputdata = NX4putdata;
  fHandle->nxputattr = NX4putattr;
  fHandle->nxputslab64 = NX4putslab64;
  fHandle->nxgetdataID = NX4getdataID;
  fHandle->nxmakelink = NX4makelink;
  fHandle->nxmakenamedlink = NX4makenamedlink;
  fHandle->nxgetdata = NX4getdata;
  fHandle->nxgetinfo64 = NX4getinfo64;
  fHandle->nxgetnextentry = NX4getnextentry;
  fHandle->nxgetslab64 = NX4getslab64;
  fHandle->nxgetnextattr = NX4getnextattr;
  fHandle->nxgetattr = NX4getattr;
  fHandle->nxgetattrinfo = NX4getattrinfo;
  fHandle->nxgetgroupID = NX4getgroupID;
  fHandle->nxgetgroupinfo = NX4getgroupinfo;
  fHandle->nxsameID = NX4sameID;
  fHandle->nxinitgroupdir = NX4initgroupdir;
  fHandle->nxinitattrdir = NX4initattrdir;
  fHandle->nxprintlink = NX4printlink;
  fHandle->nxnativeexternallink = NULL;
  fHandle->nxputattra = NX4putattra;
  fHandle->nxgetnextattra = NX4getnextattra;
  fHandle->nxgetattra = NX4getattra;
  fHandle->nxgetattrainfo = NX4getattrainfo;
}

#endif /*HDF4*/
