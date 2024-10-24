// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "Bindings/Cpp/lib3mf_implicit.hpp"
#include "MantidDataHandling/MeshFileIO.h"
#include <memory>

namespace Mantid {

namespace Geometry {
class MeshObject;
}
namespace DataHandling {

/// Typdef for a shared pointer
using MeshObject_sptr = std::shared_ptr<Geometry::MeshObject>;
/// Typdef for a shared pointer to a const object
using MeshObject_const_sptr = std::shared_ptr<const Geometry::MeshObject>;

/**

Class to load and save .3mf files
.3mf format is a 3D manufacturing format for storing mesh descriptions
of multi-component objects + metadata about the overall model (eg scale)
and the individual components. The class currently uses the Lib3MF library
to parse the files

*/

class MANTID_DATAHANDLING_DLL Mantid3MFFileIO : public MeshFileIO {
public:
  Mantid3MFFileIO() : MeshFileIO(ScaleUnits::undefined) {
    Lib3MF::PWrapper wrapper = Lib3MF::CWrapper::loadLibrary();
    model = wrapper->CreateModel();
  };
  void LoadFile(std::string filename);
  void readMeshObjects(std::vector<MeshObject_sptr> &meshObjects, MeshObject_sptr &sample);
  void writeMeshObjects(std::vector<const Geometry::MeshObject *> meshObjects, MeshObject_const_sptr &sample,
                        DataHandling::ScaleUnits scale);
  void saveFile(std::string filename);

private:
  Lib3MF::PModel model;
  std::string m_filename;
  MeshObject_sptr loadMeshObject(Lib3MF::PMeshObject meshObject, sLib3MFTransform buildTransform);
  void readMeshObject(std::vector<MeshObject_sptr> &meshObjects, MeshObject_sptr &sample, uint32_t objectResourceID,
                      sLib3MFTransform transform);
  void readComponents(std::vector<MeshObject_sptr> &meshObjects, MeshObject_sptr &sample, uint32_t objectResourceID,
                      sLib3MFTransform transform);
  void writeMeshObject(const Geometry::MeshObject &meshObject, std::string name);
  void AddBaseMaterial(std::string materialName, int materialColor, int &resourceID, Lib3MF_uint32 &materialPropertyID);
  void setMaterialOnObject(std::string objectName, std::string materialName, int materialColor);
  int generateRandomColor();
};
} // namespace DataHandling
} // namespace Mantid
