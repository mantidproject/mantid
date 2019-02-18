// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CUSTOMINTERFACES_ROWPROPERTIES_H_
#define MANTID_CUSTOMINTERFACES_ROWPROPERTIES_H_

#include "../../Reduction/Slicing.h"

#include <boost/optional.hpp>
#include <map>
#include <string>
#include <vector>

namespace MantidQt {
namespace CustomInterfaces {
class Experiment;
class Instrument;
class TransmissionRunPair;
class RangeInQ;
class RangeInLambda;
class FloodCorrections;
class PolarizationCorrections;
class PerThetaDefaults;
class MonitorCorrections;
class DetectorCorrections;
class Row;

using AlgorithmRuntimeProps = std::map<std::string, std::string>;

namespace RowProperties {

void updateInputWorkspacesProperties(
    AlgorithmRuntimeProps &properties,
    std::vector<std::string> const &inputRunNumbers);

void updateTransmissionWorkspaceProperties(
    AlgorithmRuntimeProps &properties,
    TransmissionRunPair const &transmissionRuns);

void updateMomentumTransferProperties(AlgorithmRuntimeProps &properties,
                                      RangeInQ const &rangeInQ);

void updateRowProperties(AlgorithmRuntimeProps &properties, Row const &row);

void updateTransmissionRangeProperties(
    AlgorithmRuntimeProps &properties,
    boost::optional<RangeInLambda> const &range);

void updatePolarizationCorrectionProperties(
    AlgorithmRuntimeProps &properties,
    PolarizationCorrections const &corrections);

void updateFloodCorrectionProperties(AlgorithmRuntimeProps &properties,
                                     FloodCorrections const &corrections);

void updateExperimentProperties(AlgorithmRuntimeProps &properties,
                                Experiment const &experiment);

void updatePerThetaDefaultProperties(AlgorithmRuntimeProps &properties,
                                     PerThetaDefaults const *perThetaDefaults);

void updateWavelengthRangeProperties(
    AlgorithmRuntimeProps &properties,
    boost::optional<RangeInLambda> const &rangeInLambda);

void updateMonitorCorrectionProperties(AlgorithmRuntimeProps &properties,
                                       MonitorCorrections const &monitor);

void updateDetectorCorrectionProperties(AlgorithmRuntimeProps &properties,
                                        DetectorCorrections const &detector);

void updateInstrumentProperties(AlgorithmRuntimeProps &properties,
                                Instrument const &instrument);

void updateEventProperties(AlgorithmRuntimeProps &properties,
                           Slicing const &slicing);
} // namespace RowProperties
} // namespace CustomInterfaces
} // namespace MantidQt

#endif // MANTID_CUSTOMINTERFACES_ROWPROPERTIES_H_
