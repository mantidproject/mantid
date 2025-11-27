#include "MantidAPI/FileFinder.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidDataHandling/AlignAndFocusPowderSlim/NexusLoader.h"
#include "MantidDataHandling/AlignAndFocusPowderSlim/ProcessBankSplitFullTimeTask.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidHistogramData/BinEdges.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidKernel/VectorHelper.h"
#include "MantidNexus/H5Util.h"
#include <H5Cpp.h>
#include <cxxtest/TestSuite.h>
#include <filesystem>
#include <ranges>

using namespace Mantid::DataHandling::AlignAndFocusPowderSlim;

class MockNexusLoader : public NexusLoader {
public:
  MockNexusLoader() : NexusLoader(false, {}) {}

  std::stack<EventROI> getEventIndexRanges(H5::Group &, const uint64_t,
                                           std::unique_ptr<std::vector<uint64_t>> *event_index) const override {
    std::stack<EventROI> ranges;
    ranges.emplace(0, 15); // 15 events

    if (event_index) {
      (*event_index)->push_back(0);
      (*event_index)->push_back(5);
      (*event_index)->push_back(10);
    }
    return ranges;
  }

  void loadData(H5::DataSet & /*SDS*/, std::unique_ptr<std::vector<uint32_t>> &data,
                const std::vector<size_t> & /*offsets*/, const std::vector<size_t> & /*slabsizes*/) override {
    // add detIDs
    data->resize(15);
    for (size_t i = 0; i < 15; ++i) {
      (*data)[i] = static_cast<uint32_t>(i % 2 + 1); // detIDs 1 and 2
    }
  }

  void loadData(H5::DataSet & /*SDS*/, std::unique_ptr<std::vector<float>> &data,
                const std::vector<size_t> & /*offsets*/, const std::vector<size_t> & /*slabsizes*/) override {
    // add TOFS
    data->resize(15);
    for (size_t i = 0; i < 15; ++i) {
      (*data)[i] =
          static_cast<float>((i * 1000) % 5000 + 1000); // TOFs in microseconds. (1000,2000,3000,4000,5000,1000,...)
    }
  }
};

class ProcessBankSplitFullTimeTaskTest : public CxxTest::TestSuite {
public:
  void test_ProcessBankSplitFullTimeTask() {
    // Create a mock loader
    std::shared_ptr<NexusLoader> mockLoader = std::make_shared<MockNexusLoader>();

    std::vector<std::string> bankEntryNames = {"bank1_events"};

    // we need a real file but we don't actually read any data from it because we mock the loader
    std::string filePath = Mantid::API::FileFinder::Instance().getFullPath("VULCAN_218062.nxs.h5");
    H5::H5File file(filePath, H5F_ACC_RDONLY, Mantid::Nexus::H5Util::defaultFileAcc());

    std::vector<int> workspaceIndices{0, 1};
    std::vector<Mantid::API::MatrixWorkspace_sptr> wksps;

    // create a two workspaces for output
    Mantid::HistogramData::BinEdges XValues(0);
    Mantid::Kernel::VectorHelper::createAxisFromRebinParams({0, 6000, 12000}, XValues.mutableRawData(), true, false);
    Mantid::HistogramData::Histogram hist(XValues, Mantid::HistogramData::Counts(XValues.size() - 1, 0.0));
    Mantid::API::MatrixWorkspace_sptr ws = Mantid::DataObjects::create<Mantid::DataObjects::Workspace2D>(1, hist);
    // create TimeSeriesProperty<double> for frequency log and add to workspace. 10ms pulses, 100Hz.
    auto p = new Mantid::Kernel::TimeSeriesProperty<double>("frequency");
    for (const auto &time : std::views::iota(0, 3)) {
      p->addValue(Mantid::Types::Core::DateAndTime("2024-01-01T00:00:00") + static_cast<uint64_t>(time * 10000000),
                  100.0);
    }
    ws->mutableRun().addProperty(p->clone());
    wksps.push_back(ws);
    Mantid::API::MatrixWorkspace_sptr ws2 = Mantid::DataObjects::create<Mantid::DataObjects::Workspace2D>(1, hist);
    ws2->mutableRun().addProperty(p->clone());
    wksps.push_back(ws2);

    const std::map<Mantid::detid_t, double> calibration{{1, 1.}, {2, 2.}};
    const std::map<Mantid::detid_t, double> scale_at_sample{{1, 1000.0}, {2, 1000.0}};
    const std::set<Mantid::detid_t> masked;
    std::map<Mantid::Types::Core::DateAndTime, int> splitterMap;
    splitterMap.emplace(Mantid::Types::Core::DateAndTime("2024-01-01T00:00:00"), 0);
    splitterMap.emplace(Mantid::Types::Core::DateAndTime("2024-01-01T00:00:00.005"), 1);
    splitterMap.emplace(Mantid::Types::Core::DateAndTime("2024-01-01T00:00:00.013"), -1);
    splitterMap.emplace(Mantid::Types::Core::DateAndTime("2024-01-01T00:00:00.015"), 0);
    splitterMap.emplace(Mantid::Types::Core::DateAndTime("2024-01-01T00:00:00.05"), -1);

    std::shared_ptr<Mantid::API::Progress> progress = std::make_shared<Mantid::API::Progress>();

    ProcessBankSplitFullTimeTask task(bankEntryNames, file, mockLoader, workspaceIndices, wksps, calibration,
                                      scale_at_sample, masked, 1000, 100, splitterMap, progress);

    // Run the task
    task(tbb::blocked_range<size_t>(0, 1));

    // Check results

    /* should match this python calculation

    calibration = np.array([(i % 2) + 1 for i in range(15)])
    tofs = np.array([(i*1000) % 5000 + 1000 for i in range(15)])
    full_time = np.array([tofs[i]/1e6 + (i//5)*0.01 for i in range(15)])
    calibrated_tofs = tofs * calibration
    bin_count = np.zeros((2, 2))
    for i in range(15):
        if full_time[i] < 0.005:
            x = 0
        elif full_time[i] < 0.013:
            x = 1
        elif full_time[i] < 0.015:
            continue
        else:
            x = 0
        y = 0 if calibrated_tofs[i] < 6000 else 1
        bin_count[x][y] += 1
    print(bin_count)
    */
    TS_ASSERT_EQUALS(ws->readY(0)[0], 7.0);
    TS_ASSERT_EQUALS(ws->readY(0)[1], 3.0);
    TS_ASSERT_EQUALS(ws2->readY(0)[0], 3.0);
    TS_ASSERT_EQUALS(ws2->readY(0)[1], 0.0);

    // now test with different correction to sample
    const std::map<Mantid::detid_t, double> scale_at_sample2{{1, 1000.}, {2, 500.}};

    ProcessBankSplitFullTimeTask task2(bankEntryNames, file, mockLoader, workspaceIndices, wksps, calibration,
                                       scale_at_sample2, masked, 1000, 100, splitterMap, progress);

    // Run the task
    task2(tbb::blocked_range<size_t>(0, 1));
    file.close();

    // Check results

    /* should match this python calculation

    correction_to_sample = np.array([1 - (i % 2)*0.5 for i in range(15)])
    calibration = np.array([(i % 2) + 1 for i in range(15)])
    tofs = np.array([(i*1000) % 5000 + 1000 for i in range(15)])
    full_time = np.array([tofs[i]*correction_to_sample[i]/1e6 + (i//5)*0.01 for i in range(15)])
    calibrated_tofs = tofs * calibration
    bin_count = np.zeros((2, 2))
    for i in range(15):
        if full_time[i] < 0.005:
            x = 0
        elif full_time[i] < 0.013:
            x = 1
        elif full_time[i] < 0.015:
            continue
        else:
            x = 0
        y = 0 if calibrated_tofs[i] < 6000 else 1
        bin_count[x][y] += 1
    print(bin_count)
    */

    TS_ASSERT_EQUALS(ws->readY(0)[0], 7.0);
    TS_ASSERT_EQUALS(ws->readY(0)[1], 2.0);
    TS_ASSERT_EQUALS(ws2->readY(0)[0], 3.0);
    TS_ASSERT_EQUALS(ws2->readY(0)[1], 2.0);
  }
};
