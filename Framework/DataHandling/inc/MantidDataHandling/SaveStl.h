// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_DATAHANDLING_SAVESTL_H_
#define MANTID_DATAHANDLING_SAVESTL_H_

#include "MantidKernel/BinaryStreamWriter.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/V3D.h"

namespace Mantid {
namespace Geometry {
class MeshObject;
}
namespace DataHandling {
enum class ScaleUnits;
class DLLExport SaveStl {
  /**
   * Class to contain functionality for writing out STL files for
   * SaveShapeAndEnvironment. handles actual writing to file, creating the
   * header, and removing the scale applied when loading.
   *
   */
public:
  SaveStl(const std::string &filename, const std::vector<uint32_t> triangle,
          std::vector<Kernel::V3D> vertices, ScaleUnits scaleType)
      : m_filename(filename), m_triangle(triangle), m_vertices(vertices),
        m_units(scaleType) {}

  void writeStl();

private:
  const std::string m_filename;
  const std::vector<uint32_t> m_triangle;
  const std::vector<Kernel::V3D> m_vertices;
  const ScaleUnits m_units;
  void writeHeader(Kernel::BinaryStreamWriter streamWriter);
  void writeTriangle(Kernel::BinaryStreamWriter streamWriter,
                     uint32_t triangle);
  float removeScale(double value);
};

} // namespace DataHandling
} // namespace Mantid
#endif /* MANTID_DATAHANDLING_SAVESTL_H_ */