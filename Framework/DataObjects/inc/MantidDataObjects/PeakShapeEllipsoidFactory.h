// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/System.h"
#include "PeakShapeFactory.h"

namespace Mantid {
namespace DataObjects {

/** PeakShapeEllipsoidFactory : Create ellipsoid peak shapes
 */
class DLLExport PeakShapeEllipsoidFactory : public PeakShapeFactory {
public:
  // PeakShapeFactory interface

  Mantid::Geometry::PeakShape *create(const std::string &source) const override;
  void setSuccessor(std::shared_ptr<const PeakShapeFactory> successorFactory) override;

private:
  /// Successor factory
  PeakShapeFactory_const_sptr m_successor;
};

} // namespace DataObjects
} // namespace Mantid
