// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/LoadSingleMesh.h"

namespace Mantid::DataHandling {

LoadSingleMesh::LoadSingleMesh(std::string filename, std::ios_base::openmode mode, ScaleUnits scaleType)
    : MeshFileIO(scaleType), m_filename(std::move(filename)), m_file(std::ifstream(m_filename.c_str(), mode)) {
  if (!m_file) {
    g_log.error("Unable to open file: " + m_filename);
    throw Kernel::Exception::FileError("Unable to open file: ", m_filename);
  }
}

LoadSingleMesh::~LoadSingleMesh() { m_file.close(); }

} // namespace Mantid::DataHandling
