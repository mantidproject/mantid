// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------

#include "MantidMDAlgorithms/DllConfig.h"
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

class MANTID_MDALGORITHMS_DLL Topology {

public:
  virtual void applyOrdering(Mantid::API::Point3D **unOrderedPoints) const = 0;

  virtual std::string getName() const = 0;

  virtual std::string toXMLString() const = 0;
};
} // namespace API
} // namespace Mantid
