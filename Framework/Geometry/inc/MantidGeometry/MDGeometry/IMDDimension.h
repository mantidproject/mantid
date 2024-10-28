// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidGeometry/DllConfig.h"
#include "MantidGeometry/MDGeometry/MDFrame.h"
#include "MantidGeometry/MDGeometry/MDTypes.h"
#include "MantidKernel/MDUnit.h"
#include "MantidKernel/V3D.h"
#ifndef Q_MOC_RUN
#include <memory>
#endif
#include <stdexcept>
#include <vector>

namespace Mantid {
namespace Kernel {
//---------------------------------------------------------------------------
// Forward declarations
//---------------------------------------------------------------------------
class UnitLabel;
} // namespace Kernel

namespace Geometry {
/** The class describes one dimension of multidimensional dataset representing
an orthogonal dimension and linear axis.
*
*   Abstract type for a multi dimensional dimension. Gives a read-only layer to
the concrete implementation.

    @author Owen Arnold, RAL ISIS
    @date 12/11/2010
    */

class MANTID_GEOMETRY_DLL IMDDimension {
public:
  /// Destructor
  virtual ~IMDDimension() = default;

  /// @return the name of the dimension as can be displayed along the axis
  virtual std::string getName() const = 0;

  /// @return the MDFrame
  virtual const Geometry::MDFrame &getMDFrame() const = 0;

  /// @return the units of the dimension as a string
  virtual const Kernel::UnitLabel getUnits() const = 0;

  /// @return the mdunits of the dimension
  virtual const Kernel::MDUnit &getMDUnits() const = 0;

  /// short name which identify the dimension among other dimension. A dimension
  /// can be usually find by its ID and various
  /// various method exist to manipulate set of dimensions by their names.
  /// @return Dimension ID string.
  virtual const std::string &getDimensionId() const = 0;

  /// @return the minimum extent of this dimension
  virtual coord_t getMinimum() const = 0;

  /// @return the maximum extent of this dimension
  virtual coord_t getMaximum() const = 0;

  /// @return number of bins dimension have (an integrated has one). A axis
  /// directed along dimension would have getNBins+1 axis points.
  virtual size_t getNBins() const = 0;

  /// @return number of bin boundaries (axis points)
  virtual size_t getNBoundaries() const = 0;

  /// @return an XML string representation of the dimension.
  virtual std::string toXMLString() const = 0;

  /** Change the extents and number of bins
   *  @throws std::invalid_argument If min is greater than max
   */
  virtual void setRange(size_t nBins, coord_t min, coord_t max) = 0;

  /** @return coordinate of the axis at the given index
   * @param ind :: index into the axis  */
  virtual coord_t getX(size_t ind) const = 0;

  /** @return the width of each bin */
  virtual coord_t getBinWidth() const { return (getMaximum() - getMinimum()) / static_cast<coord_t>(getNBins()); }

  /// @return true if the dimension is integrated (e.g. has only one single bin)
  virtual bool getIsIntegrated() const { return getNBins() == 1; }

  bool operator==(const IMDDimension &) const { throw std::runtime_error("Not Implemented."); }
  bool operator!=(const IMDDimension &) const { throw std::runtime_error("Not Implemented."); }
};

/// Shared Pointer for IMDDimension. Frequently used type in framework.
using IMDDimension_sptr = std::shared_ptr<IMDDimension>;
/// Shared Pointer to const IMDDimension. Not strictly necessary since
/// IMDDimension is pure abstract.
using IMDDimension_const_sptr = std::shared_ptr<const IMDDimension>;
/// Vector of constant shared pointers to IMDDimensions.
using VecIMDDimension_const_sptr = std::vector<IMDDimension_const_sptr>;
/// Vector of shared pointers to IMDDimensions.
using VecIMDDimension_sptr = std::vector<IMDDimension_sptr>;
} // namespace Geometry
} // namespace Mantid
