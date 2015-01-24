#include "MantidDataHandling/LoadMcStasNexus.h"
#include "MantidAPI/NumericAxis.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/RegisterFileLoader.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidKernel/Unit.h"
#include <nexus/NeXusException.hpp>
#include <nexus/NeXusFile.hpp>

#include <boost/algorithm/string.hpp>

namespace Mantid {
namespace DataHandling {
using namespace Kernel;
using namespace API;

DECLARE_NEXUS_FILELOADER_ALGORITHM(LoadMcStasNexus);

//----------------------------------------------------------------------------------------------
/** Constructor
 */
LoadMcStasNexus::LoadMcStasNexus() {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
LoadMcStasNexus::~LoadMcStasNexus() {}

//----------------------------------------------------------------------------------------------
/// Algorithm's name for identification. @see Algorithm::name
const std::string LoadMcStasNexus::name() const { return "LoadMcStasNexus"; };

/// Algorithm's version for identification. @see Algorithm::version
int LoadMcStasNexus::version() const { return 1; };

/// Algorithm's category for identification. @see Algorithm::category
const std::string LoadMcStasNexus::category() const { return "DataHandling"; }

/**
 * Return the confidence with with this algorithm can load the file
 * @param descriptor A descriptor for the file
 * @returns An integer specifying the confidence level. 0 indicates it will not
 * be used
 */
int LoadMcStasNexus::confidence(Kernel::NexusDescriptor &descriptor) const {
  UNUSED_ARG(descriptor)
  // To ensure that this loader is somewhat hitten return 0
  int confidence(0);
  return confidence;
}

//----------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void LoadMcStasNexus::init() {
  std::vector<std::string> exts;
  exts.push_back(".h5");
  exts.push_back(".nxs");
  declareProperty(new FileProperty("Filename", "", FileProperty::Load, exts),
                  "The name of the Nexus file to load");

  declareProperty(new WorkspaceProperty<Workspace>("OutputWorkspace", "",
                                                   Direction::Output),
                  "An output workspace.");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void LoadMcStasNexus::exec() {
  std::string filename = getPropertyValue("Filename");
  g_log.debug() << "Opening file " << filename << std::endl;

  ::NeXus::File nxFile(filename);
  auto entries = nxFile.getEntries();
  auto itend = entries.end();
  WorkspaceGroup_sptr outputGroup(new WorkspaceGroup);

  for (auto it = entries.begin(); it != itend; ++it) {
    std::string name = it->first;
    std::string type = it->second;
    nxFile.openGroup(name, type);
    auto dataEntries = nxFile.getEntries();

    for (auto eit = dataEntries.begin(); eit != dataEntries.end(); ++eit) {
      std::string dataName = eit->first;
      std::string dataType = eit->second;
      if (dataName == "content_nxs" || dataType != "NXdata")
        continue;
      g_log.debug() << "Opening " << dataName << "   " << dataType << std::endl;

      nxFile.openGroup(dataName, dataType);

      // Find the axis names
      auto nxdataEntries = nxFile.getEntries();
      std::string axis1Name, axis2Name;
      for (auto nit = nxdataEntries.begin(); nit != nxdataEntries.end();
           ++nit) {
        if (nit->second == "NXparameters")
          continue;
        nxFile.openData(nit->first);
        if (nxFile.hasAttr("axis")) {
          int axisNo(0);
          nxFile.getAttr("axis", axisNo);
          if (axisNo == 1)
            axis1Name = nit->first;
          else if (axisNo == 2)
            axis2Name = nit->first;
          else
            throw std::invalid_argument("Unknown axis number");
        }
        nxFile.closeData();
      }

      std::vector<double> axis1Values, axis2Values;
      nxFile.readData<double>(axis1Name, axis1Values);
      nxFile.readData<double>(axis2Name, axis2Values);

      const size_t axis1Length = axis1Values.size();
      const size_t axis2Length = axis2Values.size();
      g_log.debug() << "Axis lengths=" << axis1Length << " " << axis2Length
                    << std::endl;

      // Require "data" field
      std::vector<double> data;
      nxFile.readData<double>("data", data);

      // Optional errors field
      std::vector<double> errors;
      try {
        nxFile.readData<double>("errors", errors);
      } catch (::NeXus::Exception &) {
        g_log.information() << "Field " << dataName
                            << " contains no error information." << std::endl;
      }

      MatrixWorkspace_sptr ws = WorkspaceFactory::Instance().create(
          "Workspace2D", axis2Length, axis1Length, axis1Length);
      Axis *axis1 = ws->getAxis(0);
      axis1->title() = axis1Name;
      // Set caption
      boost::shared_ptr<Units::Label> lblUnit(new Units::Label);
      lblUnit->setLabel(axis1Name, "");
      axis1->unit() = lblUnit;

      Axis *axis2 = new NumericAxis(axis2Length);
      axis2->title() = axis2Name;
      // Set caption
      lblUnit = boost::shared_ptr<Units::Label>(new Units::Label);
      lblUnit->setLabel(axis2Name, "");
      axis2->unit() = lblUnit;

      ws->setYUnit(axis2Name);
      ws->replaceAxis(1, axis2);

      for (size_t wsIndex = 0; wsIndex < axis2Length; ++wsIndex) {
        auto &dataY = ws->dataY(wsIndex);
        auto &dataE = ws->dataE(wsIndex);
        auto &dataX = ws->dataX(wsIndex);

        for (size_t j = 0; j < axis1Length; ++j) {
          // Data is stored in column-major order so we are translating to
          // row major for Mantid
          const size_t fileDataIndex = j * axis2Length + wsIndex;

          dataY[j] = data[fileDataIndex];
          dataX[j] = axis1Values[j];
          if (!errors.empty())
            dataE[j] = errors[fileDataIndex];
        }
        axis2->setValue(wsIndex, axis2Values[wsIndex]);
      }
      // Make Mantid store the workspace in the group
      outputGroup->addWorkspace(ws);

      nxFile.closeGroup();
    }
    nxFile.closeGroup();
  }

  setProperty("OutputWorkspace", outputGroup);
}

} // namespace DataHandling
} // namespace Mantid
