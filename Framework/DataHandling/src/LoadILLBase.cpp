// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidDataHandling/LoadILLBase.h"
#include "MantidAPI/FileProperty.h"
#include "MantidDataHandling/NexusEntryProvider.h"
#include "MantidKernel/PropertyManagerProperty.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::NeXus;

namespace Mantid::DataHandling {

void LoadILLBase::init() {
  declareProperty(std::make_unique<FileProperty>("Filename", "", FileProperty::Load, ".nxs"),
                  "The run number of the path of the data file to load.");
  declareProperty(std::make_unique<WorkspaceProperty<>>("OutputWorkspace", "", Direction::Output),
                  "The output workspace.");
  declareProperty(std::make_unique<PropertyManagerProperty>("EntriesToPatch", Direction::Input),
                  "JSON formatted key-value pairs to override nexus entries.");
  declareExtraProperties();
}

void LoadILLBase::exec() {
  m_root = std::make_unique<NXRoot>((getPropertyValue("Filename")));
  PropertyManager_sptr pmp = getProperty("EntriesToPatch");
  m_nep = std::make_unique<NexusEntryProvider>(*m_root, *pmp);
  validateProtocol();
  load();
}

} // namespace Mantid::DataHandling
