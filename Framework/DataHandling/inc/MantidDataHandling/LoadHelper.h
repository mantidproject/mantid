// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidAPI/Run.h"
#include "MantidDataHandling/DllConfig.h"
#include "MantidKernel/Quat.h"
#include "MantidNexus/NexusClasses_fwd.h"
#include "MantidNexus/NexusFile_fwd.h"

namespace Mantid {

namespace DataHandling {

/** LoadHelper : Auxiliary functions for Loading Files
 */
namespace LoadHelper {

std::string findInstrumentNexusAddress(const Mantid::Nexus::NXEntry &);
std::string getStringFromNexusAddress(const Mantid::Nexus::NXEntry &, const std::string &);
double getDoubleFromNexusAddress(const Mantid::Nexus::NXEntry &, const std::string &);
std::vector<double> getTimeBinningFromNexusAddress(const Mantid::Nexus::NXEntry &, const std::string &);
double calculateEnergy(double);
double calculateTOF(double, double);
double getInstrumentProperty(const API::MatrixWorkspace_sptr &, const std::string &);
void addNexusFieldsToWsRun(Nexus::File &filehandle, API::Run &runDetails, const std::string &entryName = "",
                           bool useFullAddress = false);
std::string dateTimeInIsoFormat(const std::string &);

void moveComponent(const API::MatrixWorkspace_sptr &ws, const std::string &componentName, const Kernel::V3D &newPos);
void rotateComponent(const API::MatrixWorkspace_sptr &ws, const std::string &componentName, const Kernel::Quat &rot);
Kernel::V3D getComponentPosition(const API::MatrixWorkspace_sptr &ws, const std::string &componentName);

void loadEmptyInstrument(const API::MatrixWorkspace_sptr &ws, const std::string &instrumentName,
                         const std::string &instrumentAddress = "");

void fillStaticWorkspace(const API::MatrixWorkspace_sptr &, const Mantid::Nexus::NXInt &,
                         const std::vector<double> &xAxis, int64_t initialSpectrum = 0, bool pointData = false,
                         const std::vector<int> &detectorIDs = std::vector<int>(),
                         const std::set<int> &acceptedID = std::set<int>(),
                         const std::tuple<short, short, short> &axisOrder = std::tuple<short, short, short>(0, 1, 2));

void fillMovingWorkspace(const API::MatrixWorkspace_sptr &, const Mantid::Nexus::NXInt &,
                         const std::vector<double> &xAxis, int64_t initialSpectrum = 0,
                         const std::set<int> &acceptedID = std::set<int>(),
                         const std::vector<int> &customID = std::vector<int>(),
                         const std::tuple<short, short, short> &axisOrder = std::tuple<short, short, short>(0, 1, 2));

void loadingOrder(const std::tuple<short, short, short> &axisOrder, int *dataIndices);

Nexus::NXInt getIntDataset(const Nexus::NXEntry &, const std::string &);
Nexus::NXDouble getDoubleDataset(const Nexus::NXEntry &, const std::string &);

void replaceZeroErrors(const API::MatrixWorkspace_sptr &, double);

void recurseAndAddNexusFieldsToWsRun(Nexus::File &filehandle, API::Run &runDetails, const std::string &parent_name,
                                     const std::string &parent_class, int level, bool useFullAddress);
} // namespace LoadHelper
} // namespace DataHandling
} // namespace Mantid
