// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

#ifndef MANTID_DATAHANDLING_LOADISISNEXUSHELPER_H_
#define MANTID_DATAHANDLING_LOADISISNEXUSHELPER_H_

#include "MantidDataObjects/Workspace2D.h"
#include "MantidNexus/NexusClasses_fwd.h"

namespace Mantid {

namespace API {
class Run;
class Sample;
} // namespace API
namespace DataHandling {

namespace LoadISISNexusHelper {

/// find the number of spectra in the nexus file
int64_t findNumberOfSpectra(const Nexus::NXEntry &entry, const bool hasVMSBlock);

/// find detector ids and spectrum numbers
std::tuple<Nexus::NXInt, Nexus::NXInt> findDetectorIDsAndSpectrumNumber(const Nexus::NXEntry &entry,
                                                                        const bool hasVMSBlock);

// Load sample geometry into a workspace
void loadSampleGeometry(API::Sample &sample, const Nexus::NXEntry &entry, const bool hasVMSBlock);

// Load details about the run
void loadRunDetails(API::Run &runDetails, const Nexus::NXEntry &entry, const bool hasVMSBlock);

// Load time axis data
std::shared_ptr<HistogramData::HistogramX> loadTimeData(const Nexus::NXEntry &entry);

} // namespace LoadISISNexusHelper

} // namespace DataHandling

} // namespace Mantid

#endif /*MANTID_DATAHANDLING_LOADISISNEXUSHELPER_H_*/
