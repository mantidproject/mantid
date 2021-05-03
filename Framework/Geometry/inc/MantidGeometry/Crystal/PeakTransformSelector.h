// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidGeometry/Crystal/PeakTransformFactory.h"
#include "MantidKernel/System.h"
#include <set>

namespace Mantid {
namespace Geometry {
/**
@class PeakTransformSelector
Used to choose an appropriate PeakTransformFactory
*/
class DLLExport PeakTransformSelector {
public:
  /// Constructor
  PeakTransformSelector();
  /// Register a candidate factory
  void registerCandidate(const PeakTransformFactory_sptr &candidate);
  /// Make choice
  PeakTransformFactory_sptr makeChoice(const std::string &labelX, const std::string &labelY) const;
  /// Make default choice
  PeakTransformFactory_sptr makeDefaultChoice() const;
  /// Has a factory capable of the requested transform.
  bool hasFactoryForTransform(const std::string &labelX, const std::string &labelY) const;
  /// Get the number of registered factories
  size_t numberRegistered() const;

private:
  /// Disabled copy constructor
  PeakTransformSelector(const PeakTransformSelector &);
  /// Disabled assigment operator
  PeakTransformSelector &operator=(const PeakTransformSelector &);
  /// Collection of candidate factories.
  using Factories = std::set<PeakTransformFactory_sptr>;
  Factories m_candidateFactories;
};
} // namespace Geometry
} // namespace Mantid
