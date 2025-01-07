// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/LoadMcStasNexus.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/NumericAxis.h"
#include "MantidAPI/RegisterFileLoader.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidKernel/Unit.h"
// clang-format off
#include "MantidNexusCpp/NeXusFile.hpp"
#include "MantidNexusCpp/NeXusException.hpp"
// clang-format on

namespace Mantid::DataHandling {
using namespace Kernel;
using namespace API;

DECLARE_NEXUS_HDF5_FILELOADER_ALGORITHM(LoadMcStasNexus)

//----------------------------------------------------------------------------------------------
/// Algorithm's name for identification. @see Algorithm::name
const std::string LoadMcStasNexus::name() const { return "LoadMcStasNexus"; }

/// Algorithm's version for identification. @see Algorithm::version
int LoadMcStasNexus::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string LoadMcStasNexus::category() const { return "DataHandling\\Nexus"; }

/**
 * Return the confidence with with this algorithm can load the file
 * @param descriptor A descriptor for the file
 * @returns An integer specifying the confidence level. 0 indicates it will not
 * be used
 */
int LoadMcStasNexus::confidence(Kernel::NexusHDF5Descriptor &descriptor) const {
  int confidence(0);
  const auto &entries = descriptor.getAllEntries();
  for (auto iter = entries.begin(); iter != entries.end(); ++iter) {
    const auto grouped_entries = iter->second;
    if (std::any_of(grouped_entries.cbegin(), grouped_entries.cend(),
                    [](const auto &path) { return path.ends_with("information"); })) {
      confidence = 40;
      break;
    }
  }
  return confidence;
}

//----------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void LoadMcStasNexus::init() {
  const std::vector<std::string> exts{".h5", ".nxs"};
  declareProperty(std::make_unique<FileProperty>("Filename", "", FileProperty::Load, exts),
                  "The name of the Nexus file to load");

  declareProperty(std::make_unique<WorkspaceProperty<Workspace>>("OutputWorkspace", "", Direction::Output),
                  "An output workspace.");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void LoadMcStasNexus::exec() {
  std::string filename = getPropertyValue("Filename");
  g_log.debug() << "Opening file " << filename << '\n';

  ::NeXus::File nxFile(filename);
  auto entries = nxFile.getEntries();
  auto itend = entries.end();
  WorkspaceGroup_sptr outputGroup(new WorkspaceGroup);

  for (auto it = entries.begin(); it != itend; ++it) {
    std::string entryName = it->first;
    std::string type = it->second;
    nxFile.openGroup(entryName, type);
    auto dataEntries = nxFile.getEntries();

    for (auto &dataEntry : dataEntries) {
      const std::string &dataName = dataEntry.first;
      const std::string &dataType = dataEntry.second;
      if (dataName == "content_nxs" || dataType != "NXdata")
        continue;
      g_log.debug() << "Opening " << dataName << "   " << dataType << '\n';

      nxFile.openGroup(dataName, dataType);

      // Find the axis names
      auto nxdataEntries = nxFile.getEntries();
      std::string axis1Name, axis2Name;
      for (const auto &nxdataEntry : nxdataEntries) {
        if (nxdataEntry.second == "NXparameters")
          continue;
        nxFile.openData(nxdataEntry.first);
        if (nxFile.hasAttr("axis")) {
          int axisNo(0);
          nxFile.getAttr("axis", axisNo);
          if (axisNo == 1)
            axis1Name = nxdataEntry.first;
          else if (axisNo == 2)
            axis2Name = nxdataEntry.first;
          else
            throw std::invalid_argument("Unknown axis number");
        }
        nxFile.closeData();
      }

      std::vector<double> axis1Values, axis2Values;
      nxFile.readData<double>(axis1Name, axis1Values);
      nxFile.readData<double>(axis2Name, axis2Values);

      const auto axis1Length = axis1Values.size();
      const auto axis2Length = axis2Values.size();
      g_log.debug() << "Axis lengths=" << axis1Length << " " << axis2Length << '\n';

      // Require "data" field
      std::vector<double> data;
      nxFile.readData<double>("data", data);

      // Optional errors field
      std::vector<double> errors;
      try {
        nxFile.readData<double>("errors", errors);
      } catch (::NeXus::Exception &) {
        g_log.information() << "Field " << dataName << " contains no error information.\n";
      }

      MatrixWorkspace_sptr ws =
          WorkspaceFactory::Instance().create("Workspace2D", axis2Length, axis1Length, axis1Length);
      Axis *axis1 = ws->getAxis(0);
      axis1->title() = axis1Name;
      // Set caption
      auto lblUnit = std::make_shared<Units::Label>();
      lblUnit->setLabel(axis1Name, "");
      axis1->unit() = lblUnit;

      auto axis2 = std::make_unique<NumericAxis>(axis2Length);
      auto axis2Raw = axis2.get();
      axis2->title() = axis2Name;
      // Set caption
      lblUnit = std::make_shared<Units::Label>();
      lblUnit->setLabel(axis2Name, "");
      axis2->unit() = lblUnit;

      ws->setYUnit(axis2Name);
      ws->replaceAxis(1, std::move(axis2));

      ws->mutableX(0) = axis1Values;

      for (size_t wsIndex = 0; wsIndex < axis2Length; ++wsIndex) {
        auto &dataY = ws->mutableY(wsIndex);
        auto &dataE = ws->mutableE(wsIndex);
        ws->setSharedX(wsIndex, ws->sharedX(0));

        for (size_t j = 0; j < axis1Length; ++j) {
          // Data is stored in column-major order so we are translating to
          // row major for Mantid
          const size_t fileDataIndex = j * axis2Length + wsIndex;

          dataY[j] = data[fileDataIndex];
          if (!errors.empty())
            dataE[j] = errors[fileDataIndex];
        }
        axis2Raw->setValue(wsIndex, axis2Values[wsIndex]);
      }
      // Make Mantid store the workspace in the group
      outputGroup->addWorkspace(ws);

      nxFile.closeGroup();
    }
    nxFile.closeGroup();
  }

  setProperty("OutputWorkspace", outputGroup);
}

} // namespace Mantid::DataHandling
