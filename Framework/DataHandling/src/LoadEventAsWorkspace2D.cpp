// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidDataHandling/LoadEventAsWorkspace2D.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/Sample.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataHandling/LoadEventNexus.h"
#include "MantidDataHandling/LoadEventNexusIndexSetup.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidNexus/NexusIOHelper.h"

#include <regex>

namespace Mantid {
namespace DataHandling {
using namespace API;
using Mantid::HistogramData::HistogramX;
using Mantid::Kernel::Direction;
using Mantid::Kernel::TimeSeriesProperty;
// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(LoadEventAsWorkspace2D)

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string LoadEventAsWorkspace2D::name() const { return "LoadEventAsWorkspace2D"; }

/// Algorithm's version for identification. @see Algorithm::version
int LoadEventAsWorkspace2D::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string LoadEventAsWorkspace2D::category() const { return "TODO: FILL IN A CATEGORY"; }

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string LoadEventAsWorkspace2D::summary() const { return "TODO: FILL IN A SUMMARY"; }

//----------------------------------------------------------------------------------------------
/** Initialize the algorithms properties.
 */
void LoadEventAsWorkspace2D::init() {
  const std::vector<std::string> exts{".nxs.h5", ".nxs", "_event.nxs"};
  this->declareProperty(std::make_unique<FileProperty>("Filename", "", FileProperty::Load, exts),
                        "The name of the Event NeXus file to read, including its full or "
                        "relative path. ");
  declareProperty(std::make_unique<WorkspaceProperty<API::Workspace>>("OutputWorkspace", "", Direction::Output),
                  "An output workspace.");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void LoadEventAsWorkspace2D::exec() {
  std::string filename = getPropertyValue("Filename");

  // temporary workspace to load instrument and metadata
  MatrixWorkspace_sptr WS = WorkspaceFactory::Instance().create("Workspace2D", 1, 1, 1);

  // Load the logs
  int nPeriods = 1;                                                               // Unused
  auto periodLog = std::make_unique<const TimeSeriesProperty<int>>("period_log"); // Unused
  LoadEventNexus::runLoadNexusLogs<MatrixWorkspace_sptr>(filename, WS, *this, false, nPeriods, periodLog);

  if (nPeriods != 1)
    g_log.warning("This algorithm does not correctly handle period data");

  const Kernel::NexusHDF5Descriptor descriptor(filename);

  // Load the instrument
  LoadEventNexus::loadInstrument<MatrixWorkspace_sptr>(filename, WS, "entry", this, &descriptor);

  // load run metadata
  try {
    LoadEventNexus::loadEntryMetadata(filename, WS, "entry", descriptor);
  } catch (std::exception &e) {
    g_log.warning() << "Error while loading meta data: " << e.what() << '\n';
  }

  // create IndexInfo
  const std::vector<int32_t> range;
  LoadEventNexusIndexSetup indexSetup(WS, EMPTY_INT(), EMPTY_INT(), range);
  auto indexInfo = indexSetup.makeIndexInfo();
  const size_t numHist = indexInfo.size();
  g_log.warning() << "found " << indexInfo.size() << " histograms\n";

  // make output workspace with correct number of histograms
  MatrixWorkspace_sptr outWS = WorkspaceFactory::Instance().create(WS, numHist, 2, 1);
  // set spectrum index information
  outWS->setIndexInfo(indexInfo);

  // now load the data
  const auto id_to_wi = outWS->getDetectorIDToWorkspaceIndexMap();

  std::vector<uint32_t> Y(numHist, 0);

  ::NeXus::File h5file(filename);

  h5file.openPath("/");
  h5file.openGroup("entry", "NXentry");

  // Now we want to go through all the bankN_event entries
  const std::map<std::string, std::set<std::string>> &allEntries = descriptor.getAllEntries();

  auto itClassEntries = allEntries.find("NXevent_data");

  if (itClassEntries != allEntries.end()) {

    const std::set<std::string> &classEntries = itClassEntries->second;
    const std::regex classRegex("(/entry/)([^/]*)");
    std::smatch groups;

    for (const std::string &classEntry : classEntries) {

      if (std::regex_match(classEntry, groups, classRegex)) {
        const std::string entry_name(groups[2].str());

        // skip entries with junk data
        if (entry_name == "bank_error_events" || entry_name == "bank_unmapped_events")
          continue;
        h5file.openGroup(entry_name, "NXevent_data");

        const auto event_ids = Mantid::NeXus::NeXusIOHelper::readNexusVector<uint32_t>(h5file, "event_id");
        const auto event_times = Mantid::NeXus::NeXusIOHelper::readNexusVector<float>(h5file, "event_time_offset");

        for (size_t i = 0; i < event_ids.size(); i++) {
          auto det_id = event_ids[i];
          auto wi = id_to_wi.find(det_id);
          if (wi == id_to_wi.end())
            continue;

          auto tof = event_times[i];
          if (tof > 20000. || tof < -20000.)
            continue;

          Y[wi->second]++;
        }

        h5file.closeGroup();
      }
    }
  }

  h5file.closeGroup();
  h5file.close();

  // determine x values
  const auto center = outWS->run().getStatistics("wavelength").mean;
  const auto width = outWS->run().getStatistics("wavelength_spread").mean * center;
  const auto xBins = {center - width / 2, center + width / 2};

  // set the data on the workspace
  auto histX = Mantid::Kernel::make_cow<HistogramX>(xBins);
  for (size_t idx = 0; idx < numHist; idx++) {
    outWS->mutableY(idx) = Y[idx];
    outWS->mutableE(idx) = sqrt(Y[idx]);
    outWS->setSharedX(idx, histX);
  }

  // set units
  outWS->getAxis(0)->setUnit("wavelength");
  outWS->setYUnit("Counts");

  // add filename
  outWS->mutableRun().addProperty("Filename", filename);
  setProperty("OutputWorkspace", outWS);
}

} // namespace DataHandling
} // namespace Mantid
