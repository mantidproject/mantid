// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidDataObjects/PeakShapeFactory.h"

namespace Mantid {
namespace Geometry {
// Forward declare
class PeakShape;
} // namespace Geometry
namespace DataObjects {

/** PeakShapeDetectorBinFactory : Factory for DetectorBin peak shapes for
 de-serializing from JSON.
 *
*/
class MANTID_DATAOBJECTS_DLL PeakShapeDetectorBinFactory : public PeakShapeFactory {
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
