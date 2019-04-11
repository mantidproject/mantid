// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/LoadStl.h"
#include "MantidKernel/V3D.h"
#include <Poco/File.h>

#include <functional>

namespace Mantid {
namespace DataHandling {

bool LoadStl::areEqualVertices(Kernel::V3D const &v1,
                               Kernel::V3D const &v2) const {
  const Kernel::V3D diff = v1 - v2;
  const double nanoMetre = 1e-9;
  return diff.norm() < nanoMetre; // This is 1 nanometre for a unit of a metre.
}

void LoadStl::changeToVector() {
  m_verticies.resize(vertexSet.size());
  for (auto const &mapValue : vertexSet) {
    m_verticies[mapValue.second] = mapValue.first;
  }
}

Kernel::V3D LoadStl::createScaledV3D(double xVal, double yVal, double zVal){
  switch(m_scaleType){
      case centimetres : xVal = xVal/100;
                         yVal = yVal/100;
                         zVal = zVal/100;
                         break;
      case milimetres  : xVal = xVal/1000;
                         yVal = yVal/1000;
                         zVal = zVal/1000;
                         break;
      case metres : break;
    }
    return Kernel::V3D(double(xVal), double(yVal), double(zVal));
}
} // namespace DataHandling
} // namespace Mantid