// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidAPI/WorkspaceGroup_fwd.h"
#include "MantidDataHandling/DllConfig.h"
#include <H5Cpp.h>
#include <filesystem>
#include <vector>

namespace Mantid {
namespace DataHandling {
namespace NXcanSAS {

// DATA
void MANTID_DATAHANDLING_DLL addData1D(H5::Group &data, const Mantid::API::MatrixWorkspace_sptr &workspace);
void MANTID_DATAHANDLING_DLL addData2D(H5::Group &data, const Mantid::API::MatrixWorkspace_sptr &workspace);

// STANDARD METADATA
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

// POLARIZED DATA
void MANTID_DATAHANDLING_DLL addPolarizedData(H5::Group &data, const Mantid::API::WorkspaceGroup_sptr &wsGroup,
                                              const std::string &inputSpinStates);
void MANTID_DATAHANDLING_DLL addPolarizer(H5::Group &group, const Mantid::API::MatrixWorkspace_sptr &workspace,
                                          const std::string &componentName, const std::string &componentType,
                                          const std::string &groupSuffix);

// POLARIZED METADATA
void MANTID_DATAHANDLING_DLL addSampleEMFields(H5::Group &group, const Mantid::API::MatrixWorkspace_sptr &workspace,
                                               const std::string &emFieldStrengthLog, const std::string &emFieldDir);
void MANTID_DATAHANDLING_DLL addEMFieldDirection(H5::Group &group, const std::string &emFieldDir);

// UTILITY
std::string MANTID_DATAHANDLING_DLL makeCanSASRelaxedName(const std::string &input);
H5::H5File MANTID_DATAHANDLING_DLL prepareFile(const std::filesystem::path &path);
std::filesystem::path MANTID_DATAHANDLING_DLL prepareFilename(const std::string &baseFilename,
                                                              bool addDigitSuffix = false, size_t index = 0);
std::string MANTID_DATAHANDLING_DLL addDigit(size_t index);

} // namespace NXcanSAS
} // namespace DataHandling
} // namespace Mantid
