// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_MDALGORITHMS_MODERATORCHOPPERRESOLUTION_H_
#define MANTID_MDALGORITHMS_MODERATORCHOPPERRESOLUTION_H_

#include "MantidGeometry/IDTypes.h"

#include <boost/shared_ptr.hpp>

namespace Mantid {
//
// Forward declarations
//
namespace API {
class ChopperModel;
class ExperimentInfo;
class ModeratorModel;
} // namespace API

namespace MDAlgorithms {
class CachedExperimentInfo;
/**
 *
 * Calculates the energy resolution for a moderator-chopper
 * combination from a given observation
 *
 */
class DLLExport ModeratorChopperResolution {
public:
  /// Constructor
  ModeratorChopperResolution(const CachedExperimentInfo &observation);

  /// Disable default constructor
  ModeratorChopperResolution() = delete;

  /// Disable copy operator
  ModeratorChopperResolution(const ModeratorChopperResolution &) = delete;

  /// Disable assignment operator
  ModeratorChopperResolution &
  operator=(const ModeratorChopperResolution &) = delete;

  /// Return a width in energy for the model
  double energyWidth(const double deltaE) const;

private:
  /// Store required cached variables
  void initCaches();

  /// A reference to the observation object
  const CachedExperimentInfo &m_observation;
  /// A pointer to the moderator object
  boost::shared_ptr<const API::ModeratorModel> m_moderator;
  /// A pointer to the chopper object
  boost::shared_ptr<const API::ChopperModel> m_chopper;

  /// Moderator-chopper distance
  double m_modChopDist;
  /// ChopperModel-sample distance
  double m_chopSampleDist;
};
} // namespace MDAlgorithms
} // namespace Mantid

#endif /* MANTID_MDALGORITHMS_MODERATORCHOPPERRESOLUTION_H_ */
