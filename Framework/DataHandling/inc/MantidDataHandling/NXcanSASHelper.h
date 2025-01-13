// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidDataHandling/DllConfig.h"
#include <H5Cpp.h>

namespace Mantid {
namespace DataHandling {
namespace NXcanSAS {
// Helper functions for algorithms saving in NXcanSAS format.
enum class WorkspaceDimensionality;

std::string MANTID_DATAHANDLING_DLL makeCanSASRelaxedName(const std::string &input);

void MANTID_DATAHANDLING_DLL addDetectors(H5::Group &group, const Mantid::API::MatrixWorkspace_sptr &workspace,
                                          const std::vector<std::string> &detectorNames);
void MANTID_DATAHANDLING_DLL addInstrument(H5::Group &group, const Mantid::API::MatrixWorkspace_sptr &workspace,
                                           const std::string &radiationSource, const std::string &geometry,
                                           double beamHeight, double beamWidth,
                                           const std::vector<std::string> &detectorNames);
void MANTID_DATAHANDLING_DLL addSample(H5::Group &group, const double &sampleThickness);
void MANTID_DATAHANDLING_DLL addProcess(H5::Group &group, const Mantid::API::MatrixWorkspace_sptr &workspace);
void MANTID_DATAHANDLING_DLL addProcess(H5::Group &group, const Mantid::API::MatrixWorkspace_sptr &workspace,
                                        const Mantid::API::MatrixWorkspace_sptr &canWorkspace);
void MANTID_DATAHANDLING_DLL addProcessEntry(H5::Group &group, const std::string &entryName,
                                             const std::string &entryValue);
void MANTID_DATAHANDLING_DLL addTransmission(H5::Group &group, const Mantid::API::MatrixWorkspace_const_sptr &workspace,
                                             const std::string &transmissionName);

void MANTID_DATAHANDLING_DLL addData1D(H5::Group &data, const Mantid::API::MatrixWorkspace_sptr &workspace);
void MANTID_DATAHANDLING_DLL addData2D(H5::Group &data, const Mantid::API::MatrixWorkspace_sptr &workspace);

WorkspaceDimensionality MANTID_DATAHANDLING_DLL
getWorkspaceDimensionality(const Mantid::API::MatrixWorkspace_sptr &workspace);
std::vector<std::string> MANTID_DATAHANDLING_DLL splitDetectorNames(std::string detectorNames);

} // namespace NXcanSAS
} // namespace DataHandling
} // namespace Mantid
