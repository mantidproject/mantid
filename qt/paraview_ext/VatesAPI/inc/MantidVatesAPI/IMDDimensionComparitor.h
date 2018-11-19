// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef IMD_DIMENSION_COMPARITOR_H
#define IMD_DIMENSION_COMPARITOR_H

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/IMDWorkspace.h"
#include "MantidGeometry/MDGeometry/IMDDimension.h"

namespace Mantid {
namespace VATES {

/** Dimension comparitor specifically for use with visualisation layer. Given an
arrangement of dimensions in an MDImage, this type
allow the utilising code to ask wheter some dimension maps to the x, y, or z
dimensions.

@author Owen Arnold, Tessella plc
@date 25/03/2011
*/
class IMDDimensionComparitor {
public:
  /// Constructor
  IMDDimensionComparitor(Mantid::API::IMDWorkspace_sptr workspace);
  IMDDimensionComparitor operator=(IMDDimensionComparitor &) = delete;
  IMDDimensionComparitor(IMDDimensionComparitor &) = delete;

  /// Destructor
  ~IMDDimensionComparitor();

  bool isXDimension(const Mantid::Geometry::IMDDimension &queryDimension);

  bool isYDimension(const Mantid::Geometry::IMDDimension &queryDimension);

  bool isZDimension(const Mantid::Geometry::IMDDimension &queryDimension);

  bool istDimension(const Mantid::Geometry::IMDDimension &queryDimension);

private:
  /// imd workspace shared ptr.
  Mantid::API::IMDWorkspace_sptr m_workspace;
};
} // namespace VATES
} // namespace Mantid

#endif
