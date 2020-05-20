// Mantid Repository : https://github.com/mantidproject/mantid
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
// 	NScD Oak Ridge National Laboratory, European Spallation Source,
// 	Institut Laue - Langevin
// 	&  CSNS, Institute of High Energy Physics，Chinese Academy of Sciences.

// SPDX - License - Identifier: GPL - 3.0 +

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------

#include "MantidDataHandling/LoadCSNSNexus.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/NumericAxis.h"
#include "MantidAPI/RegisterFileLoader.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceGroup.h"

#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/Unit.h"

#include "MantidKernel/DateAndTime.h"

#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/VisibleWhenProperty.h"

#include "LoadRaw/isisraw2.h"
#include "MantidAPI/IFileLoader.h"
#include "MantidDataHandling/EventWorkspaceCollection.h"
#include "MantidDataHandling/LoadRawHelper.h"
#include "MantidDataObjects/EventWorkspace.h"
#include <stdio.h>
#include <string.h>
/************************/

/************************/
// clang-format off
#include <nexus/NeXusFile.hpp>
//#include <nexus/NeXusException.hpp>
// clang-format on

//#include <boost/algorithm/string.hpp>

#include <iostream>
/****************************************/
#include "MantidDataHandling/EventWorkspaceCollection.h"
#include "MantidKernel/Logger.h"

#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
/****************************************/
using Mantid::Types::Core::DateAndTime;
using namespace Mantid::DataObjects;
// using namespace API;

namespace Mantid {
namespace DataHandling {
using namespace Kernel;
using namespace API;
using namespace std;
using namespace DataObjects;
using Mantid::Types::Event::TofEvent;
using std::string;
using std::vector;
using Types::Core::DateAndTime;

Kernel::Logger g_log("LoadCSNSNexus");

DECLARE_NEXUS_FILELOADER_ALGORITHM(LoadCSNSNexus)

/// Empty default constructor

/// Algorithm's name for identification. @see Algorithm::name
const std::string LoadCSNSNexus::name() const { return "LoadCSNSNexus"; }

/// Algorithm's version for identification. @see Algorithm::version
int LoadCSNSNexus::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string LoadCSNSNexus::category() const {
  return "DataHandling\\Nexus";
}

/*
 * Return the confidence with with this algorithm can load the file
 * @param descriptor A descriptor for the file
 * @returns An integer specifying the confidence level. 0 indicates it will not
 * be used
 */
int LoadCSNSNexus::confidence(Kernel::NexusDescriptor &descriptor) const {
  UNUSED_ARG(descriptor)
  // To ensure that this loader is somewhat hitten return 0
  int confidence(0);
  return confidence;
}

//----------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void LoadCSNSNexus::init() {
  declareProperty(make_unique<PropertyWithValue<string>>("Instrument", "GPPD",
                                                         Direction::Input),
                  "Different instrument with different detector combinations");

  const vector<string> exts{".h5", ".nxs"};
  declareProperty(
      make_unique<FileProperty>("Filename", "", FileProperty::Load, exts),
      "The name of the Nexus file to load");

  declareProperty(std::make_unique<PropertyWithValue<string>>(
                      "NXentryName", "csns", Direction::Input),
                  "Optional: Name of entry (default csns)");

  declareProperty(
      make_unique<PropertyWithValue<bool>>("LoadBank", true, Direction::Input),
      "Default true: load bank data, false: load monitor data.");

  declareProperty(
      std::make_unique<ArrayProperty<string>>("Bankname", Direction::Input),
      "Optional: A comma-separated list of bank/monitor to read");

  declareProperty(
      std::make_unique<ArrayProperty<uint32_t>>("StartT0", Direction::Input),
      "Optional: A comma-separated list of StartNo of T0 to read.");
  declareProperty(
      std::make_unique<ArrayProperty<uint32_t>>("EndT0", Direction::Input),
      "Optional: A comma-separated list of endNo of T0 to read.");

  declareProperty(make_unique<PropertyWithValue<bool>>("LoadEvent", false,
                                                       Direction::Input),
                  "Default false: load event data, true: load histogram data.");

  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>(
                      "OutputWorkspace", "", Direction::Output),
                  "An output orkspace.");

  std::string grp1 = "Bank/Monitor";
  setPropertyGroup("Bankname", grp1);
  setPropertyGroup("loadBank", grp1);

  std::string grp2 = "FilterByPulse";
  setPropertyGroup("LoadEvent", grp2);
  setPropertyGroup("StartT0", grp2);
  setPropertyGroup("EndT0", grp2);
}

/**
 * Return the start or end time of experiment
 * @param[in] typeName :: start_time_utc or end_time_utc
 * @return expTime
 */
Types::Core::DateAndTime LoadCSNSNexus::getExperimentTime(string typeName) {
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
vector<string> LoadCSNSNexus::getGPPDModules(string bankName) {
  vector<string> data;
  std::map<std::string, std::string> entries;
  m_file->openGroup(m_entry, "NXentry");
  m_file->openGroup("instrument", "NXinstrument");
  entries = m_file->getEntries();
  if (bankName == "bank3") {
    for (auto it = entries.begin(); it != entries.end(); ++it) {
      std::string name = it->first;
      string tmp = name.substr(0, 7);
      // if (!name.find("module1") || !name.find("module2"))
      if (tmp == "module1" || tmp == "module2")
        data.push_back(name);
    }
  } else if (bankName == "bank2") {
    for (auto it = entries.begin(); it != entries.end(); ++it) {
      std::string name = it->first;
      string tmp = name.substr(0, 7);
      // if (!name.find("module3") || !name.find("module4"))
      if (tmp == "module3" || tmp == "module4")
        data.push_back(name);
    }
  } else if (bankName == "bank1") {
    for (auto it = entries.begin(); it != entries.end(); ++it) {
      std::string name = it->first;
      string tmp = name.substr(0, 7);
      // if (!name.find("module5") || !name.find("module6"))
      if (tmp == "module5" || tmp == "module6")
        data.push_back(name);
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
bool LoadCSNSNexus::checkBanknames(const vector<string> &inputNames) {
  if (inputNames.size() == 0)
    return false;
  else if (inputNames.size() > 1) {
    for (auto it = inputNames.begin(); it != inputNames.end(); it++) {
      string name = *it;
      if (name.substr(0, 6) != "module")
        return false;
    }
    return true;
  } else
    return true;
}
/**
 * Return modules for special instrument
 * @param[in] inst :: instrument name
 * @param[in] inputNames :: Bankname
 * @return modules
 */
vector<string> LoadCSNSNexus::getModules(string inst,
                                         const vector<string> &inputNames) {
  vector<string> data;
  if (inst == "SANS" || inst == "MR")
    data.push_back("module1");
  else if (inst == "GPPD") {
    for (auto it = inputNames.begin(); it != inputNames.end(); it++) {
      if ((*it).substr(0, 4) == "bank")
        data = getGPPDModules(*it);
      else
        data.push_back(*it);
    }
  }
  return data;
}

/**
 * Return pixel id
 * @param[in] inputList :: moduleList or monitorList
 * @return pixelId
 */
std::vector<int64_t>
LoadCSNSNexus::getPixelId(const vector<string> &inputList) {
  std::vector<int64_t> _tmp;
  std::vector<int64_t> pixelId;
  m_file->openGroup(m_entry, "NXentry");
  m_file->openGroup("instrument", "NXinstrument");
  std::map<std::string, std::string> entries = m_file->getEntries();
  for (auto j = inputList.begin(); j != inputList.end(); j++) {
    auto it = entries.find(*j);
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
std::vector<uint32_t> LoadCSNSNexus::getTimeBin(string typeName) {
  std::vector<int> tmp;
  std::map<std::string, std::string> entries;
  m_file->openGroup(m_entry, "NXentry");
  m_file->openGroup("instrument", "NXinstrument");
  entries = m_file->getEntries();
  string detName = "";

  for (auto it = entries.begin(); it != entries.end(); ++it) {
    string name = it->first;
    if (name.substr(0, 6) == typeName.substr(0, 6)) {
      m_file->openGroup(name, it->second);
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
std::vector<uint32_t>
LoadCSNSNexus::getHistData(const vector<string> &inputList) {
  std::vector<int> tmp;
  std::vector<uint32_t> data;

  m_file->openGroup("csns", "NXentry");
  m_file->openGroup("histogram_data", "NXcollection");
  std::map<std::string, std::string> entries = m_file->getEntries();
  for (auto j = inputList.begin(); j != inputList.end(); ++j) {
    auto it = entries.find(*j);
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
void LoadCSNSNexus::loadHistData(MatrixWorkspace_sptr &workspace,
                                 std::vector<uint32_t> &timeOfFlight,
                                 size_t pidNums,
                                 std::vector<uint32_t> &histData) {
  size_t timeNums = timeOfFlight.size();
  MantidVecPtr x, e;
  MantidVec xA;
  MantidVec xRef(timeNums);

  int st1 = 0;
  for (auto it = timeOfFlight.begin(); it != timeOfFlight.end(); it++) {
    xRef[st1] = (*it);
    st1++;
  }

  std::vector<double> err;
  for (auto it = histData.begin(); it != histData.end(); it++)
    err.push_back(sqrt(*it));

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
std::multimap<uint32_t, std::pair<float, int64_t>> LoadCSNSNexus::getEventData(
    const vector<string> &inputList, const vector<uint32_t> &startList,
    const vector<uint32_t> &endList, const vector<int64_t> &pids) {
  std::vector<int64_t> pid_tmp;
  std::vector<int64_t> pidList;
  std::vector<int64_t> t0_tmp;
  std::vector<int64_t> t0List;
  std::vector<float> tof_tmp;
  std::vector<float> tofList;
  std::pair<float, int64_t> tofPulse;
  m_file->openGroup(m_entry, "NXentry");
  m_file->openGroup("event_data", "NXcollection");
  std::map<std::string, std::string> entries = m_file->getEntries();

  for (auto j = inputList.begin(); j != inputList.end(); j++) {
    auto it = entries.find(*j);
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
  for (auto i = pids.begin(); i != pids.end(); i++) {
    mapping.insert(std::map<int64_t, uint32_t>::value_type(*i, p));
    p++;
  }
  std::multimap<uint32_t, std::pair<float, int64_t>> data;
  for (size_t i = 0; i < pidList.size(); i++) {
    uint32_t specNo = mapping[pidList[i]];
    tofPulse = std::make_pair(tofList[i], t0List[i]);
    data.insert(std::multimap<uint32_t, std::pair<float, int64_t>>::value_type(
        specNo, tofPulse));
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
void LoadCSNSNexus::loadEventData(
    EventWorkspace_sptr &workspace, const std::vector<uint32_t> &timeOfFlight,
    size_t pidNums,
    std::multimap<uint32_t, std::pair<float, int64_t>> evtData) {
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
        workspace->getSpectrum(i) += TofEvent(m_tof, m_pulseTime);
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
  g_log.information() << "start load csns nexus data " << endl;
  string m_filename = getPropertyValue("Filename");
  m_entry = getPropertyValue("NXentryName");
  vector<string> bankNames = getProperty("Bankname");
  m_file = std::make_unique<::NeXus::File>(m_filename, NXACC_READ);

  bool m_loadBank = getProperty("LoadBank");
  bool m_loadEvent = getProperty("LoadEvent");
  Mantid::Types::Core::DateAndTime start_time =
      getExperimentTime("start_time_utc");
  Mantid::Types::Core::DateAndTime end_time = getExperimentTime("end_time_utc");
  g_log.information() << "load experiment time " << endl;

  MatrixWorkspace_sptr ws_hist;
  EventWorkspace_sptr ws_evt(new EventWorkspace);

  if (m_loadBank) {
    string instName = getPropertyValue("Instrument");
    bool m_inputCheck = checkBanknames(bankNames);
    if (m_inputCheck) {
      m_modules = getModules(instName, bankNames);
      vector<int64_t> pid_bank = getPixelId(m_modules);
      vector<uint32_t> tof_bank = getTimeBin("module");

      if (m_loadEvent) {
        g_log.information() << "load event data " << endl;
        vector<uint32_t> startT0 = getProperty("startT0");
        vector<uint32_t> endT0 = getProperty("endT0");
        std::multimap<uint32_t, std::pair<float, int64_t>> evtData =
            getEventData(m_modules, startT0, endT0, pid_bank);
        loadEventData(ws_evt, tof_bank, pid_bank.size(), evtData);
        ws_evt->mutableRun().setStartAndEndTime(start_time, end_time);
        ws_evt->getAxis(0)->unit() =
            Kernel::UnitFactory::Instance().create("TOF");
        ws_evt->setYUnit("Counts");

      } else {
        g_log.information() << "load histogram data " << endl;
        ws_hist = WorkspaceFactory::Instance().create(
            "Workspace2D", pid_bank.size(), tof_bank.size(),
            tof_bank.size() - 1);
        std::vector<uint32_t> histData = getHistData(m_modules);
        loadHistData(ws_hist, tof_bank, pid_bank.size(), histData);
        ws_hist->mutableRun().setStartAndEndTime(start_time, end_time);
        ws_hist->getAxis(0)->unit() =
            Kernel::UnitFactory::Instance().create("TOF");
        ws_hist->setYUnit("Counts");
      }
    } else {
      throw std::runtime_error("Error in Banknames input!");
    }
  } else {

    g_log.information() << "load monitor data " << endl;
    vector<int64_t> pid_mon = getPixelId(bankNames);
    vector<uint32_t> tof_mon = getTimeBin("monitor");
    ws_hist = WorkspaceFactory::Instance().create(
        "Workspace2D", pid_mon.size(), tof_mon.size(), tof_mon.size() - 1);
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

} // namespace DataHandling
} // namespace Mantid
