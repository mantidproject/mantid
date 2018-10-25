// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_SPECULARREFLECTIONALGORITHM_H_
#define MANTID_ALGORITHMS_SPECULARREFLECTIONALGORITHM_H_

#include "MantidAPI/DataProcessorAlgorithm.h"
#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/System.h"

namespace Mantid {
namespace Algorithms {

/** SpecularReflectionAlgorithm : Algorithm base class implementing generic
 methods required for specular reflection calculations.
 *
 */
class DLLExport SpecularReflectionAlgorithm
    : public Mantid::API::DataProcessorAlgorithm {
protected:
  /// Constructor
  SpecularReflectionAlgorithm() = default;

  /// Get the surface sample component
  Mantid::Geometry::IComponent_const_sptr
  getSurfaceSampleComponent(Mantid::Geometry::Instrument_const_sptr inst) const;

  /// Get the detector component
  Mantid::Geometry::IComponent_const_sptr
  getDetectorComponent(Mantid::API::MatrixWorkspace_sptr workspace,
                       const bool isPointDetector) const;

  /// Does the property have a default value.
  bool isPropertyDefault(const std::string &propertyName) const;

  /// initialize common properties
  void initCommonProperties();

  /// Calculate the twoTheta angle w.r.t the direct beam
  double calculateTwoTheta() const;
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_SPECULARREFLECTIONALGORITHM_H_ */
