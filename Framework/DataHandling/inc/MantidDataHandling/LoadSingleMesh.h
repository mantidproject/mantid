// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include "MantidDataHandling/MeshFileIO.h"
#include "MantidKernel/Logger.h"

#include <fstream>

namespace Mantid {

namespace Geometry {
class MeshObject;
}
namespace DataHandling {
namespace {
Mantid::Kernel::Logger g_log("LoadSingleMesh");
}
class DLLExport LoadSingleMesh : public MeshFileIO {
public:
  LoadSingleMesh(const std::string &filename, std::ios_base::openmode mode, ScaleUnits scaleType);
  virtual ~LoadSingleMesh();
  virtual std::unique_ptr<Geometry::MeshObject> readShape() = 0;

protected:
  const std::string m_filename;
  std::ifstream m_file;
};
} // namespace DataHandling
} // namespace Mantid
