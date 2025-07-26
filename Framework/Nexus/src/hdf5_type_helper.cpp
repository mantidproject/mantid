#include "MantidNexus/hdf5_type_helper.h"
#include <assert.h>
#include <cstring>
#include <map>
#include <memory>
#include <stdlib.h>
#include <string.h>
#include <time.h>

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
  return memtype_id;
}
