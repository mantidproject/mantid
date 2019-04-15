// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidKernel/Logger.h"
#include "MantidKernel/V3D.h"

#ifndef MANTID_DATAHANDLING_LOADSHAPE_H_
#define MANTID_DATAHANDLING_LOADSHAPE_H_
namespace {
Mantid::Kernel::Logger g_log("LoadShape");
}
namespace Mantid {
namespace DataHandling {
enum ScaleUnits { metres, centimetres, milimetres };

class DLLExport LoadShape {

protected:
  LoadShape(ScaleUnits scaleType) : m_scaleType(scaleType) {}
  Kernel::V3D createScaledV3D(double xVal, double yVal, double zVal) {
    switch (m_scaleType) {
    case centimetres:
      xVal = xVal / 100;
      yVal = yVal / 100;
      zVal = zVal / 100;
      break;
    case milimetres:
      xVal = xVal / 1000;
      yVal = yVal / 1000;
      zVal = zVal / 1000;
      break;
    case metres:
      break;
    }
    return Kernel::V3D(double(xVal), double(yVal), double(zVal));
  }

  const ScaleUnits m_scaleType;
  std::vector<uint32_t> m_triangle;
  std::vector<Kernel::V3D> m_vertices;
}; // namespace DataHandling
} // namespace DataHandling
} // namespace Mantid
#endif /*MANTID_DATAHANDLING_LOADSHAPE_H_*/