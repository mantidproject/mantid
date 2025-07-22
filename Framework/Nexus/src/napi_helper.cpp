#include "MantidNexus/napi_helper.h"
#include <assert.h>
#include <cstring>
#include <map>
#include <memory>
#include <stdlib.h>
#include <string.h>
#include <time.h>

std::string getObjectAddress(hid_t const obj) {
  // get correctly sized array for name
  std::size_t addrlen = H5Iget_name(obj, NULL, 0);
  char *caddr = new char[addrlen + 1];
  // get name and store inside string
  H5Iget_name(obj, caddr, addrlen + 1);
  std::string ret(caddr);
  delete[] caddr;
  return ret;
}

std::string buildCurrentAddress(NexusFile5 const &fid) {
  hid_t current = 0;
  if (fid.iCurrentD != 0) {
    current = fid.iCurrentD;
  } else if (fid.iCurrentG != 0) {
    current = fid.iCurrentG;
  } else {
    current = fid.iFID;
  }
  return getObjectAddress(current);
}

// NOTE save in case needed later as model
// std::size_t countObjectsInGroup(NexusFile5 const &fid) {
//   std::size_t count = 0;
//   herr_t iRet;
//   if (fid.iCurrentG == 0) {
//     hid_t gid = H5Gopen(fid.iFID, "/", H5P_DEFAULT);
//     iRet = H5Gget_num_objs(gid, &count);
//     H5Gclose(gid);
//   } else {
//     iRet = H5Gget_num_objs(fid.iCurrentG, &count);
//   }
//   if (iRet < 0) {
//     NXReportError("Internal error, failed to retrieve no of objects");
//     return 0;
//   }
//   return count;
// }

NXnumtype hdf5ToNXType(H5T_class_t tclass, hid_t atype) {
  // NOTE this is exploiting the bit-level representation of NXnumtype
  // where the first hex stands for: 2 = float, 1 = signed int, 0 = unsigned int
  // and where the second hex is the width in bytes (which is given by H5Tget_size)
  // and where CHAR (0XF0) and BINARY (0xF1) are special values
  unsigned short size = static_cast<unsigned short>(H5Tget_size(atype));
  NXnumtype iPtype = NXnumtype::BAD;
  if (tclass == H5T_STRING) {
    iPtype = NXnumtype::CHAR;
  } else if (tclass == H5T_BITFIELD) {
    // underused datatype, would be great for masks
    iPtype = NXnumtype::BINARY;
  } else if (tclass == H5T_INTEGER) {
    unsigned short type = size;
    H5T_sign_t sign = H5Tget_sign(atype);
    if (sign == H5T_SGN_2) {
      type += 0x10; // signed data type
    }
    iPtype = NXnumtype(type);
  } else if (tclass == H5T_FLOAT) {
    iPtype = NXnumtype(0x20u + size);
  }
  if (iPtype == NXnumtype::BAD) {
    char message[80];
    snprintf(message, 79, "ERROR: hdf5ToNXtype: invalid type (%d)", tclass);
    NXReportError(message);
  }
  return iPtype;
}

hid_t nxToHDF5Type(NXnumtype datatype) {
  hid_t type;
  switch (datatype) {
  case NXnumtype::CHAR: {
    type = H5T_C_S1;
    break;
  }
  case NXnumtype::INT8: {
    type = H5T_NATIVE_CHAR;
    break;
  }
  case NXnumtype::UINT8: {
    type = H5T_NATIVE_UCHAR;
    break;
  }
  case NXnumtype::INT16: {
    type = H5T_NATIVE_SHORT;
    break;
  }
  case NXnumtype::UINT16: {
    type = H5T_NATIVE_USHORT;
    break;
  }
  case NXnumtype::INT32: {
    type = H5T_NATIVE_INT;
    break;
  }
  case NXnumtype::UINT32: {
    type = H5T_NATIVE_UINT;
    break;
  }
  case NXnumtype::INT64: {
    type = H5T_NATIVE_INT64;
    break;
  }
  case NXnumtype::UINT64: {
    type = H5T_NATIVE_UINT64;
    break;
  }
  case NXnumtype::FLOAT32: {
    type = H5T_NATIVE_FLOAT;
    break;
  }
  case NXnumtype::FLOAT64: {
    type = H5T_NATIVE_DOUBLE;
    break;
  }
  default: {
    NXReportError("ERROR: nxToHDF5Type: unknown type");
    type = -1;
  }
  }
  return type;
}

hid_t h5MemType(hid_t atype) {
  // NOTE is this function actually necessary?
  hid_t memtype_id = -1;
  size_t size;
  H5T_sign_t sign;
  H5T_class_t tclass;

  tclass = H5Tget_class(atype);

  if (tclass == H5T_INTEGER) {
    size = H5Tget_size(atype);
    sign = H5Tget_sign(atype);
    if (size == 1) {
      if (sign == H5T_SGN_2) {
        memtype_id = H5T_NATIVE_INT8;
      } else {
        memtype_id = H5T_NATIVE_UINT8;
      }
    } else if (size == 2) {
      if (sign == H5T_SGN_2) {
        memtype_id = H5T_NATIVE_INT16;
      } else {
        memtype_id = H5T_NATIVE_UINT16;
      }
    } else if (size == 4) {
      if (sign == H5T_SGN_2) {
        memtype_id = H5T_NATIVE_INT32;
      } else {
        memtype_id = H5T_NATIVE_UINT32;
      }
    } else if (size == 8) {
      if (sign == H5T_SGN_2) {
        memtype_id = H5T_NATIVE_INT64;
      } else {
        memtype_id = H5T_NATIVE_UINT64;
      }
    }
  } else if (tclass == H5T_FLOAT) {
    size = H5Tget_size(atype);
    if (size == 4) {
      memtype_id = H5T_NATIVE_FLOAT;
    } else if (size == 8) {
      memtype_id = H5T_NATIVE_DOUBLE;
    }
  }
  if (memtype_id == -1) {
    NXReportError("ERROR: h5MemType: invalid type");
  }
  return memtype_id;
}

bool canBeOpened(std::string const &filename) {
  // this is for reading, check for existence first
  FILE *fd = NULL;
  fd = fopen(filename.c_str(), "r");
  if (fd == NULL) {
    return false;
  }
  fclose(fd);

  // check that this is indeed hdf5
  if (H5Fis_hdf5(filename.c_str()) > 0) {
    return true;
  } else {
    // file type not recognized
    return false;
  }
}
