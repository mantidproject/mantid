// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source,
//     Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------

#include "MantidDataHandling/LoadCSNSNexus.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/NumericAxis.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataHandling/LoadRawHelper.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/UnitFactory.h"

#include <algorithm>
/************************/

namespace Mantid::DataHandling {

using namespace Kernel;
using namespace API;
using namespace DataObjects;

Kernel::Logger g_log("LoadCSNSNexus");

DECLARE_ALGORITHM(LoadCSNSNexus)
/// Empty default constructor

/// Algorithm's name for identification. @see Algorithm::name
const std::string LoadCSNSNexus::name() const { return "LoadCSNSNexus"; }

/// Algorithm's version for identification. @see Algorithm::version
int LoadCSNSNexus::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string LoadCSNSNexus::category() const { return "DataHandling\\Nexus"; }

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void LoadCSNSNexus::init() {
  declareProperty(std::make_unique<PropertyWithValue<std::string>>("Instrument", "GPPD", Direction::Input),
                  "Different instrument with different detector combinations");

  const std::vector<std::string> exts{".h5", ".nxs"};
  declareProperty(std::make_unique<FileProperty>("Filename", "", FileProperty::Load, exts),
                  "The name of the Nexus file to load");

  declareProperty(std::make_unique<PropertyWithValue<std::string>>("NXentryName", "csns", Direction::Input),
                  "Optional: Name of entry (default csns)");

  declareProperty(std::make_unique<PropertyWithValue<bool>>("LoadBank", true, Direction::Input),
                  "Default true: load bank data, false: load monitor data.");

  declareProperty(std::make_unique<ArrayProperty<std::string>>("Bankname", Direction::Input),
                  "Optional: A comma-separated list of bank/monitor to read");

  declareProperty(std::make_unique<ArrayProperty<uint32_t>>("StartT0", Direction::Input),
                  "Optional: A comma-separated list of StartNo of T0 to read.");
  declareProperty(std::make_unique<ArrayProperty<uint32_t>>("EndT0", Direction::Input),
                  "Optional: A comma-separated list of endNo of T0 to read.");

  declareProperty(std::make_unique<PropertyWithValue<bool>>("LoadEvent", false, Direction::Input),
                  "Default false: load event data, true: load histogram data.");

  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>("OutputWorkspace", "", Direction::Output),
                  "An output orkspace.");

  const std::string grp1 = "Bank/Monitor";
  setPropertyGroup("Bankname", grp1);
  setPropertyGroup("loadBank", grp1);

  const std::string grp2 = "FilterByPulse";
  setPropertyGroup("LoadEvent", grp2);
  setPropertyGroup("StartT0", grp2);
  setPropertyGroup("EndT0", grp2);
}

/**
 * Return the start or end time of experiment
 * @param[in] typeName :: start_time_utc or end_time_utc
 * @return expTime
 */
Types::Core::DateAndTime LoadCSNSNexus::getExperimentTime(const std::string &typeName) {
  Types::Core::DateAndTime data;
  std::string expTime;
  m_file->openGroup(m_entry, "NXentry");
  std::map<std::string, std::string> entries = m_file->getEntries();
  if (entries.find(typeName) != entries.end())

    m_file->readData(typeName, expTime);
  else
    throw std::runtime_error("No experiment time in the NXS file!");
  m_file->closeGroup();
  try {
    data = Mantid::Types::Core::DateAndTime(expTime);
  } catch (...) {
    data = Mantid::Types::Core::DateAndTime("2100-12-31T00:00:00+08:00");
  }
  return data;
}

/**
 * Return modules for special bankName at GPPD
 * @param[in] bankName :: bank1,bank2,bank3
 * @return modules
 */
std::vector<std::string> LoadCSNSNexus::getGPPDModules(const std::string &bankName) {
  int firstModuleId(-1), secondModuleId(-1);
  if (bankName == "bank3") {
    firstModuleId = 1;
    secondModuleId = 2;
  } else if (bankName == "bank2") {
    firstModuleId = 3;
    secondModuleId = 4;
  } else if (bankName == "bank1") {
    firstModuleId = 5;
    secondModuleId = 6;
  }

  std::vector<std::string> data;
  m_file->openGroup(m_entry, "NXentry");
  m_file->openGroup("instrument", "NXinstrument");
  auto entries = m_file->getEntries();
  for (const auto &it : entries) {
    const auto &moduleName = it.first;
    if (moduleName.compare(0, 7, "module" + std::to_string(firstModuleId)) == 0 ||
        moduleName.compare(0, 7, "module" + std::to_string(secondModuleId)) == 0) {
      data.emplace_back(moduleName);
    }
  }
  m_file->closeGroup();
  m_file->closeGroup();
  return data;
}

/**
 * Judge the input of Bankname
 * @param[in] inputNames :: Bankname
 * @return true or false
 */
bool LoadCSNSNexus::checkBanknames(const std::vector<std::string> &inputNames) {
  if (inputNames.size() == 0)
    return false;
  else if (inputNames.size() > 1) {
    return std::all_of(inputNames.cbegin(), inputNames.cend(),
                       [](const auto &name) { return name.compare(0, 6, "module") == 0; });
  } else
    return true;
}
/**
 * Return modules for special instrument
 * @param[in] inst :: instrument name
 * @param[in] inputNames :: Bankname
 * @return modules
 */
std::vector<std::string> LoadCSNSNexus::getModules(const std::string &inst,
                                                   const std::vector<std::string> &inputNames) {
  std::vector<std::string> data;
  if (inst == "SANS" || inst == "MR")
    data.push_back("module1");
  else if (inst == "GPPD") {
    for (const auto &moduleName : inputNames) {
      if (moduleName.compare(0, 4, "bank") == 0)
        data = getGPPDModules(moduleName);
      else
        data.push_back(moduleName);
    }
  }
  return data;
}

/**
 * Return pixel id
 * @param[in] inputList :: moduleList or monitorList
 * @return pixelId
 */
std::vector<int64_t> LoadCSNSNexus::getPixelId(const std::vector<std::string> &inputList) {
  std::vector<int64_t> _tmp;
  std::vector<int64_t> pixelId;
  m_file->openGroup(m_entry, "NXentry");
  m_file->openGroup("instrument", "NXinstrument");
  auto entries = m_file->getEntries();
  for (const auto &entryName : inputList) {
    auto it = entries.find(entryName);
    if (it != entries.end()) {
      m_file->openGroup(it->first, it->second);
      m_file->readData("pixel_id", _tmp);
      pixelId.insert(pixelId.end(), _tmp.begin(), _tmp.end());
      m_file->closeGroup();
    }
  }
  sort(pixelId.begin(), pixelId.end());
  m_file->closeGroup();
  m_file->closeGroup();
  return pixelId;
}

/**
 * Return time-of-flight
 * @param[in] typeName :: module or monitor
 * @return time-of-flight
 */
std::vector<uint32_t> LoadCSNSNexus::getTimeBin(const std::string &typeName) {
  std::vector<int> tmp;
  m_file->openGroup(m_entry, "NXentry");
  m_file->openGroup("instrument", "NXinstrument");
  auto entries = m_file->getEntries();
  for (const auto &[entryName, nodeType] : entries) {
    if ((entryName.compare(0, 6, typeName) == 0) || (entryName.compare(0, 7, typeName) == 0)) {
      m_file->openGroup(entryName, nodeType);
      m_file->readData("time_of_flight", tmp);
      m_file->closeGroup();
      break;
    }
  }
  m_file->closeGroup();
  m_file->closeGroup();
  std::vector<uint32_t> timeBin(tmp.begin(), tmp.end());
  return timeBin;
}

/**
 * Get histogram data
 * @param[in] inputList :: moduleList or monitorList
 * @return histogram data
 */
std::vector<uint32_t> LoadCSNSNexus::getHistData(const std::vector<std::string> &inputList) {
  std::vector<int> tmp;
  std::vector<uint32_t> data;

  m_file->openGroup("csns", "NXentry");
  m_file->openGroup("histogram_data", "NXcollection");
  auto entries = m_file->getEntries();
  for (const auto &entryName : inputList) {
    auto it = entries.find(entryName);
    if (it != entries.end()) {
      m_file->openGroup(it->first, it->second);
      m_file->readData("histogram_data", tmp);
      data.insert(data.end(), tmp.begin(), tmp.end());
      m_file->closeGroup();
    }
  }
  m_file->closeGroup();
  m_file->closeGroup();
  return data;
}

/**
 * load histogram data into workspace
 * @param[in] workspace ::
 * @param[in] timeOfFlight ::
 * @param[in] pidNums :: total pixel numbers
 * @param[in] histData :: histogram data
 */
void LoadCSNSNexus::loadHistData(MatrixWorkspace_sptr &workspace, const std::vector<uint32_t> &timeOfFlight,
                                 size_t pidNums, const std::vector<uint32_t> &histData) {
  size_t timeNums = timeOfFlight.size();
  MantidVecPtr x, e;
  MantidVec xA;
  MantidVec xRef(timeNums);

  std::copy(timeOfFlight.cbegin(), timeOfFlight.cend(), xRef.begin());

  std::vector<double> err;
  err.resize(histData.size());
  std::transform(histData.cbegin(), histData.cend(), std::back_inserter(err),
                 [](auto const &hist) { return sqrt(hist); });

  auto it_start = histData.begin();
  auto it_end = it_start + timeNums - 1;

  auto it_err_start = err.begin();
  auto it_err_end = it_err_start + timeNums - 1;

  size_t hist = 0;
  while (hist < pidNums) {
    auto &Y = workspace->mutableY(hist);
    auto &E = workspace->mutableE(hist);
    Y.assign(it_start, it_end);
    E.assign(it_err_start, it_err_end);
    it_start += (timeNums - 1);
    it_end += (timeNums - 1);
    it_err_start += (timeNums - 1);
    it_err_end += (timeNums - 1);
    auto toPass = Kernel::make_cow<HistogramData::HistogramX>(xRef);
    workspace->setX(hist, toPass);
    ++hist;
  }
}

/**
 * Get event data
 * @param[in] inputList :: moduleList or monitorList
 * @param[in] startList :: list of T0 start number
 * @param[in] endList :: list of T0 end number
 * @param[in] pids :: total pixel id
 * @return event data :: specNo, tof, t0
 */
std::multimap<uint32_t, std::pair<float, int64_t>>
LoadCSNSNexus::getEventData(const std::vector<std::string> &inputList, const std::vector<uint32_t> &startList,
                            const std::vector<uint32_t> &endList, const std::vector<int64_t> &pids) {
  std::vector<int64_t> pid_tmp;
  std::vector<int64_t> pidList;
  std::vector<int64_t> t0_tmp;
  std::vector<int64_t> t0List;
  std::vector<float> tof_tmp;
  std::vector<float> tofList;
  std::pair<float, int64_t> tofPulse;
  m_file->openGroup(m_entry, "NXentry");
  m_file->openGroup("event_data", "NXcollection");
  auto entries = m_file->getEntries();
  for (auto const &entryName : inputList) {
    auto it = entries.find(entryName);
    if (it != entries.end()) {
      m_file->openGroup(it->first, it->second);
      m_file->readData("event_pulse_time", t0_tmp);
      m_file->readData("event_pixel_id", pid_tmp);
      m_file->readData("event_time_of_flight", tof_tmp);
      if (startList.size() == 0) {
        t0List.insert(t0List.end(), t0_tmp.begin(), t0_tmp.end());
        pidList.insert(pidList.end(), pid_tmp.begin(), pid_tmp.end());
        tofList.insert(tofList.end(), tof_tmp.begin(), tof_tmp.end());
      } else {
        for (size_t k = 0; k < t0_tmp.size(); k++) {
          for (size_t j = 0; j < startList.size(); j++) {
            if (t0_tmp[k] >= startList[j] && t0_tmp[k] <= endList[j]) {
              t0List.push_back(t0_tmp[k]);
              pidList.push_back(pid_tmp[k]);
              tofList.push_back(tof_tmp[k]);
            }
          }
        }
      }
      m_file->closeGroup();
    }
  }
  tof_tmp.clear();
  t0_tmp.clear();
  pid_tmp.clear();
  m_file->closeGroup();
  m_file->closeGroup();
  std::map<int64_t, uint32_t> mapping;
  uint32_t p = 0;
  for (const auto &pidNum : pids) {
    mapping.insert(std::map<int64_t, uint32_t>::value_type(pidNum, p));
    p++;
  }
  std::multimap<uint32_t, std::pair<float, int64_t>> data;
  for (size_t i = 0; i < pidList.size(); i++) {
    uint32_t specNo = mapping[pidList[i]];
    tofPulse = std::make_pair(tofList[i], t0List[i]);
    data.insert(std::multimap<uint32_t, std::pair<float, int64_t>>::value_type(specNo, tofPulse));
  }

  tofList.clear();
  t0List.clear();
  pidList.clear();
  return data;
}

/**
 * load event data into workspace
 * @param[in] workspace :: event workspace ptr
 * @param[in] timeOfFlight :: tof
 * @param[in] pidNums :: total pixel numbers
 * @param[in] evtData :: event data
 */
void LoadCSNSNexus::loadEventData(EventWorkspace_sptr &workspace, const std::vector<uint32_t> &timeOfFlight,
                                  size_t pidNums, const std::multimap<uint32_t, std::pair<float, int64_t>> &evtData) {
  workspace->initialize(pidNums, 1, 1);
  float m_tof;
  uint64_t m_pulseTime;
  int endNum = static_cast<int>(pidNums);
  for (auto i = 0; i != endNum; i++) {
    auto it = evtData.find(i);
    if (it != evtData.end()) {
      for (size_t j = 0; j != evtData.count(i); j++, it++) {
        m_pulseTime = it->second.second;
        m_tof = it->second.first;
        workspace->getSpectrum(i) += Types::Event::TofEvent(m_tof, m_pulseTime);
      }
      workspace->getSpectrum(i).setSpectrumNo(i);
    }
  }
  double dataMin = *(timeOfFlight.begin());
  double dataMax = *(timeOfFlight.end() - 1);
  workspace->setAllX(HistogramData::BinEdges{dataMin, dataMax});
}

/** Execute the algorithm.
 */
void LoadCSNSNexus::exec() {
  g_log.information() << "start load csns nexus data " << std::endl;
  const std::string m_filename = getPropertyValue("Filename");
  m_entry = getPropertyValue("NXentryName");
  const std::vector<std::string> bankNames = getProperty("Bankname");
  m_file = std::make_unique<::NeXus::File>(m_filename, NXACC_READ);
  const bool m_loadBank = getProperty("LoadBank");
  const bool m_loadEvent = getProperty("LoadEvent");
  const auto start_time = getExperimentTime("start_time_utc");
  const auto end_time = getExperimentTime("end_time_utc");
  g_log.information() << "load experiment time " << std::endl;

  MatrixWorkspace_sptr ws_hist;
  EventWorkspace_sptr ws_evt(new EventWorkspace);

  if (m_loadBank) {
    const std::string instName = getPropertyValue("Instrument");
    const bool m_inputCheck = checkBanknames(bankNames);
    if (m_inputCheck) {
      m_modules = getModules(instName, bankNames);
      std::vector<int64_t> pid_bank = getPixelId(m_modules);
      std::vector<uint32_t> tof_bank = getTimeBin("module");

      if (m_loadEvent) {
        g_log.information() << "load event data " << std::endl;
        std::vector<uint32_t> startT0 = getProperty("startT0");
        std::vector<uint32_t> endT0 = getProperty("endT0");
        std::multimap<uint32_t, std::pair<float, int64_t>> evtData = getEventData(m_modules, startT0, endT0, pid_bank);
        loadEventData(ws_evt, tof_bank, pid_bank.size(), evtData);
        ws_evt->mutableRun().setStartAndEndTime(start_time, end_time);
        ws_evt->getAxis(0)->unit() = Kernel::UnitFactory::Instance().create("TOF");
        ws_evt->setYUnit("Counts");

      } else {
        g_log.information() << "load histogram data " << std::endl;
        ws_hist =
            WorkspaceFactory::Instance().create("Workspace2D", pid_bank.size(), tof_bank.size(), tof_bank.size() - 1);
        std::vector<uint32_t> histData = getHistData(m_modules);
        loadHistData(ws_hist, tof_bank, pid_bank.size(), histData);
        ws_hist->mutableRun().setStartAndEndTime(start_time, end_time);
        ws_hist->getAxis(0)->unit() = Kernel::UnitFactory::Instance().create("TOF");
        ws_hist->setYUnit("Counts");
      }
    } else {
      throw std::runtime_error("Error in Banknames input!");
    }
  } else {

    g_log.information() << "load monitor data " << std::endl;
    std::vector<int64_t> pid_mon = getPixelId(bankNames);
    std::vector<uint32_t> tof_mon = getTimeBin("monitor");
    ws_hist = WorkspaceFactory::Instance().create("Workspace2D", pid_mon.size(), tof_mon.size(), tof_mon.size() - 1);
    std::vector<uint32_t> histData_mon = getHistData(bankNames);
    loadHistData(ws_hist, tof_mon, pid_mon.size(), histData_mon);
    ws_hist->mutableRun().setStartAndEndTime(start_time, end_time);
    ws_hist->getAxis(0)->unit() = Kernel::UnitFactory::Instance().create("TOF");
    ws_hist->setYUnit("Counts");
  }

  if (m_loadEvent) {
    setProperty("OutputWorkspace", ws_evt);
  } else {
    setProperty("OutputWorkspace", ws_hist);
  }
}
} // namespace Mantid::DataHandling
