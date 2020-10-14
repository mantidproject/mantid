// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidDataObjects/PeakShapeFactory.h"
#include "MantidKernel/System.h"

namespace Mantid {
namespace Geometry {
// Forward declare
class PeakShape;
} // namespace Geometry
namespace DataObjects {

/** PeakShapeSphericalFactory : Factory for spherical peak shapes for
 de-serializing from JSON.
 *
*/
class DLLExport PeakShapeSphericalFactory : public PeakShapeFactory {
public:
  /// Make product
  Mantid::Geometry::PeakShape *create(const std::string &source) const override;
  /// Set a successor should this factory be unsuitable
  void setSuccessor(PeakShapeFactory_const_sptr successorFactory) override;

private:
  /// Successor factory
  PeakShapeFactory_const_sptr m_successor;
};

} // namespace DataObjects
} // namespace Mantid
