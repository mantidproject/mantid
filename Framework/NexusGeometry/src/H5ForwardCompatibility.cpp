// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidNexusGeometry/H5ForwardCompatibility.h"

namespace Mantid {
namespace NexusGeometry {

namespace H5ForwardCompatibility {

/*
 * Function "getObjName"
 * Ported from newer versions of the HDF API for forward compatibility.
 */

ssize_t getObjName(const H5::H5Object &obj, char *obj_name, size_t buf_size) {
  // H5Iget_name will get buf_size-1 chars of the name to null terminate it
  ssize_t name_size = H5Iget_name(obj.getId(), obj_name, buf_size);

  // If H5Iget_name returns a negative value, raise an exception
  if (name_size < 0) {
    throw H5::Exception("getObjName", "H5Iget_name failed");
  } else if (name_size == 0) {
    throw H5::Exception("getObjName",
                        "Object must have a name, but name length is 0");
  }
  // Return length of the name
  return (name_size);
}

std::string getObjName(const H5::H5Object &obj) {
  std::string obj_name(""); // object name to return

  // Preliminary call to get the size of the object name
  ssize_t name_size = H5Iget_name(obj.getId(), NULL, static_cast<size_t>(0));

  // If H5Iget_name failed, throw exception
  if (name_size < 0) {
    throw H5::Exception("getObjName", "H5Iget_name failed");
  } else if (name_size == 0) {
    throw H5::Exception("getObjName",
                        "Object must have a name, but name length is 0");
  }
  // Object's name exists, retrieve it
  else if (name_size > 0) {
    char *name_C = new char[name_size + 1]; // temporary C-string
    memset(name_C, 0, name_size + 1);       // clear buffer

    // Use overloaded function
    name_size = getObjName(obj, name_C, name_size + 1);

    // Convert the C object name to return
    obj_name = name_C;

    // Clean up resource
    delete[] name_C;
  }
  // Return object's name
  return (obj_name);
}

} // namespace H5ForwardCompatibility

} // namespace NexusGeometry
} // namespace Mantid
