// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidDataHandling/AlignAndFocusPowderSlim.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataHandling/LoadBankFromDiskTask.h"
#include "MantidDataHandling/LoadEventNexus.h"
#include "MantidDataHandling/LoadEventNexusIndexSetup.h"
#include "MantidDataObjects/EventList.h"
#include "MantidKernel/VectorHelper.h"
#include "MantidNexus/NexusIOHelper.h"

#include <regex>

namespace Mantid::DataHandling {
using Mantid::API::FileProperty;
using Mantid::API::MatrixWorkspace_sptr;
using Mantid::API::WorkspaceFactory;
using Mantid::API::WorkspaceProperty;
using Mantid::Kernel::Direction;

namespace { // anonymous namespace
namespace PropertyNames {
const std::string FILENAME("Filename");
const std::string OUTPUT_WKSP("OutputWorkspace");
} // namespace PropertyNames

namespace NxsFieldNames {
const std::string TIME_OF_FLIGHT("event_time_offset");
const std::string DETID("event_id");
} // namespace NxsFieldNames

// this is used for unit conversion to correct units
const std::string MICROSEC("microseconds");

} // namespace

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(AlignAndFocusPowderSlim)

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string AlignAndFocusPowderSlim::name() const { return "AlignAndFocusPowderSlim"; }

/// Algorithm's version for identification. @see Algorithm::version
int AlignAndFocusPowderSlim::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string AlignAndFocusPowderSlim::category() const { return "TODO: FILL IN A CATEGORY"; }

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string AlignAndFocusPowderSlim::summary() const { return "TODO: FILL IN A SUMMARY"; }

const std::vector<std::string> AlignAndFocusPowderSlim::seeAlso() const { return {"AlignAndFocusPowderFromFiles"}; }

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void AlignAndFocusPowderSlim::init() {
  const std::vector<std::string> exts{".nxs.h5", ".nxs", "_event.nxs"};
  // docs copied/modified from LoadEventNexus
  declareProperty(std::make_unique<FileProperty>(PropertyNames::FILENAME, "", FileProperty::Load, exts),
                  "The name of the Event NeXus file to read, including its full or relative path. "
                  "The file name is typically of the form INST_####_event.nxs.");
  declareProperty(
      std::make_unique<WorkspaceProperty<API::MatrixWorkspace>>(PropertyNames::OUTPUT_WKSP, "", Direction::Output),
      "An output workspace.");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void AlignAndFocusPowderSlim::exec() {
  // create a histogram workspace
  constexpr size_t numHist{6};
  constexpr double xmin{6463};
  constexpr double xmax{39950};

  HistogramData::BinEdges XValues_new(0);
  UNUSED_ARG(Kernel::VectorHelper::createAxisFromRebinParams({xmin, 10, xmax}, XValues_new.mutableRawData(), true,
                                                             false, xmin, xmax));
  const size_t numBins = XValues_new.size() - 1;
  MatrixWorkspace_sptr wksp = WorkspaceFactory::Instance().create("Workspace2D", numHist, numBins + 1, numBins);
  for (size_t i = 0; i < numHist; ++i) {
    wksp->setBinEdges(i, XValues_new);
  }

  const std::string filename = getPropertyValue(PropertyNames::FILENAME);
  const Kernel::NexusHDF5Descriptor descriptor(filename);

  /*
  // TODO load instrument
  // Load the instrument
  // prog->doReport("Loading instrument"); TODO add progress bar stuff
  //LoadEventNexus::loadInstrument<MatrixWorkspace_sptr>(filename, wksp, "entry", this, &descriptor);

  // load run metadata
  // prog->doReport("Loading metadata"); TODO add progress bar stuff
  try {
    LoadEventNexus::loadEntryMetadata(filename, WS, "entry", descriptor);
  } catch (std::exception &e) {
    g_log.warning() << "Error while loading meta data: " << e.what() << '\n';
  }

  // create IndexInfo
  // prog->doReport("Creating IndexInfo"); TODO add progress bar stuff
  const std::vector<int32_t> range;
  LoadEventNexusIndexSetup indexSetup(WS, EMPTY_INT(), EMPTY_INT(), range);
  auto indexInfo = indexSetup.makeIndexInfo();
  const size_t numHist = indexInfo.size();

  // make output workspace with correct number of histograms
  MatrixWorkspace_sptr outWS = WorkspaceFactory::Instance().create(WS, numHist, 2, 1);
  // set spectrum index information
  outWS->setIndexInfo(indexInfo);
  */

  // load the events
  ::NeXus::File h5file(filename);

  h5file.openPath("/");
  h5file.openGroup("entry", "NXentry"); // TODO should this allow other entries?

  // Now we want to go through all the bankN_event entries
  const std::map<std::string, std::set<std::string>> &allEntries = descriptor.getAllEntries();
  auto itClassEntries = allEntries.find("NXevent_data");

  if (itClassEntries != allEntries.end()) {
    const std::set<std::string> &classEntries = itClassEntries->second;
    const std::regex classRegex("(/entry/)([^/]*)");
    std::smatch groups;

    size_t specnum = 0;
    for (const std::string &classEntry : classEntries) {
      if (std::regex_match(classEntry, groups, classRegex)) {
        const std::string entry_name(groups[2].str());

        // skip entries with junk data
        if (entry_name == "bank_error_events" || entry_name == "bank_unmapped_events")
          continue;

        // TODO should re-use vectors to save malloc/free calls
        std::unique_ptr<std::vector<uint32_t>> event_detid = std::make_unique<std::vector<uint32_t>>();
        std::unique_ptr<std::vector<float>> event_time_of_flight = std::make_unique<std::vector<float>>();
        // TODO std::unique_ptr<std::vector<float>> event_weight; some other time
        // std::unique_ptr<std::vector<uint64_t>> event_index; matching pulse-times with events

        g_log.information() << "Loading bank " << entry_name << '\n';
        h5file.openGroup(entry_name, "NXevent_data");

        loadTOF(event_time_of_flight, h5file);
        loadDetid(event_detid, h5file);

        auto binFinder = DataObjects::EventList::findLinearBin;
        const auto divisor{.1};
        const auto offset{xmin * divisor};

        auto &spectrum = wksp->getSpectrum(specnum);

        for (const auto &tof : *event_time_of_flight) {
          const auto binnum = binFinder(spectrum.dataX(), static_cast<double>(tof), divisor, offset, false);
          if (binnum)
            spectrum.dataY()[binnum.get()] += 1;
        }

        h5file.closeGroup();
        specnum++;
      }
    }
  }

  // go back to where we started
  h5file.closeGroup();
  h5file.close();

  // TODO load logs

  setProperty("OutputWorkspace", std::move(wksp));
}

void AlignAndFocusPowderSlim::loadTOF(std::unique_ptr<std::vector<float>> &data, ::NeXus::File &h5file) {
  g_log.information(NxsFieldNames::TIME_OF_FLIGHT);
  h5file.openData(NxsFieldNames::TIME_OF_FLIGHT);

  // This is the data size
  ::NeXus::Info id_info = h5file.getInfo();
  const auto dim0 = static_cast<size_t>(LoadBankFromDiskTask::recalculateDataSize(id_info.dims[0]));
  data->resize(dim0);

  Mantid::NeXus::NeXusIOHelper::readNexusVector<float>(*data, h5file, NxsFieldNames::TIME_OF_FLIGHT);

  // get the units
  std::string tof_unit;
  h5file.getAttr("units", tof_unit);

  // close the sds
  h5file.closeData();

  // Convert Tof to microseconds
  if (tof_unit != MICROSEC)
    Kernel::Units::timeConversionVector(*data, tof_unit, MICROSEC);
}

void AlignAndFocusPowderSlim::loadDetid(std::unique_ptr<std::vector<uint32_t>> &data, ::NeXus::File &h5file) {
  g_log.information(NxsFieldNames::DETID);
  h5file.openData(NxsFieldNames::DETID);

  // This is the data size
  ::NeXus::Info id_info = h5file.getInfo();
  const auto dim0 = static_cast<size_t>(LoadBankFromDiskTask::recalculateDataSize(id_info.dims[0]));
  data->resize(dim0);

  Mantid::NeXus::NeXusIOHelper::readNexusVector<uint32_t>(*data, h5file, NxsFieldNames::DETID);

  // close the sds
  h5file.closeData();
}

} // namespace Mantid::DataHandling
