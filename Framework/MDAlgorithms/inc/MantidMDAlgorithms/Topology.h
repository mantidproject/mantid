// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef VIS_TOPOLOGY_H_
#define VIS_TOPOLOGY_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------

#include "MantidKernel/System.h"
#include <vector>

namespace Mantid {

namespace API {
class Point3D;
}

namespace API {
/**

Abstract type represents topology for visualisation.

@author Owen Arnold, Tessella plc
@date 27/10/2010
*/

class DLLExport Topology {

public:
  virtual void applyOrdering(Mantid::API::Point3D **unOrderedPoints) const = 0;

  virtual std::string getName() const = 0;

  virtual std::string toXMLString() const = 0;
};
} // namespace API
} // namespace Mantid

#endif
