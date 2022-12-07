// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Run.h"
#include "MantidDataHandling/DllConfig.h"
#include "MantidNexus/NexusClasses.h"

namespace Mantid {

namespace DataHandling {

/** LoadHelper : Auxiliary functions for Loading Files
 */
namespace LoadHelper {

std::string findInstrumentNexusPath(const Mantid::NeXus::NXEntry &);
std::string getStringFromNexusPath(const Mantid::NeXus::NXEntry &, const std::string &);
double getDoubleFromNexusPath(const Mantid::NeXus::NXEntry &, const std::string &);
std::vector<double> getTimeBinningFromNexusPath(const Mantid::NeXus::NXEntry &, const std::string &);
double calculateEnergy(double);
double calculateTOF(double, double);
double getInstrumentProperty(const API::MatrixWorkspace_sptr &, const std::string &);
void addNexusFieldsToWsRun(NXhandle nxfileID, API::Run &runDetails, const std::string &entryName = "",
                           bool useFullPath = false);
void dumpNexusAttributes(NXhandle nxfileID);
std::string dateTimeInIsoFormat(const std::string &);

void moveComponent(const API::MatrixWorkspace_sptr &ws, const std::string &componentName, const Kernel::V3D &newPos);
void rotateComponent(const API::MatrixWorkspace_sptr &ws, const std::string &componentName, const Kernel::Quat &rot);
Kernel::V3D getComponentPosition(const API::MatrixWorkspace_sptr &ws, const std::string &componentName);

void loadEmptyInstrument(const API::MatrixWorkspace_sptr &ws, const std::string &instrumentName,
                         const std::string &instrumentPath = "");

void recurseAndAddNexusFieldsToWsRun(NXhandle nxfileID, API::Run &runDetails, std::string &parent_name,
                                     std::string &parent_class, int level, bool useFullPath);
} // namespace LoadHelper
} // namespace DataHandling
} // namespace Mantid
