// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_COMPOSITEIMPLICITFUNCTION_H_
#define MANTID_ALGORITHMS_COMPOSITEIMPLICITFUNCTION_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidGeometry/MDGeometry/MDImplicitFunction.h"
#include "MantidKernel/System.h"
#include <vector>
#ifndef Q_MOC_RUN
#include <boost/shared_ptr.hpp>
#endif

namespace Mantid {
namespace Geometry {
/**

 This class represents a composite implicit function used for communicating and
 implementing an operation against
 an MDWorkspace.

 @author Owen Arnold, Tessella plc
 @date 01/10/2010
 */

class DLLExport CompositeImplicitFunction
    : public Mantid::Geometry::MDImplicitFunction {
public:
  //---------------------------------- Override base-class methods---
  bool isPointContained(const coord_t *coords) override;
  bool isPointContained(const std::vector<coord_t> &coords) override;
  // Unhide base class methods (avoids Intel compiler warning)
  using MDImplicitFunction::isPointContained;
  //-----------------------------------------------------------------

  bool
  addFunction(Mantid::Geometry::MDImplicitFunction_sptr constituentFunction);
  std::string getName() const override;
  std::string toXMLString() const override;
  int getNFunctions() const;
  static std::string functionName() { return "CompositeImplicitFunction"; }

protected:
  std::vector<Mantid::Geometry::MDImplicitFunction_sptr> m_Functions;
  using FunctionIterator =
      std::vector<Mantid::Geometry::MDImplicitFunction_sptr>::const_iterator;
};
} // namespace Geometry
} // namespace Mantid

#endif
