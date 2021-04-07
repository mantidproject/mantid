// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/LoadSingleMesh.h"

namespace Mantid {
namespace DataHandling {

LoadSingleMesh::LoadSingleMesh(const std::string &filename, std::ios_base::openmode mode, ScaleUnits scaleType)
    : MeshFileIO(scaleType), m_filename(filename) {
  m_file = std::ifstream(filename.c_str(), mode);
  if (!m_file) {
    g_log.error("Unable to open file: " + filename);
    throw Kernel::Exception::FileError("Unable to open file: ", filename);
  }
}

LoadSingleMesh::~LoadSingleMesh() { m_file.close(); }

} // namespace DataHandling
} // namespace Mantid