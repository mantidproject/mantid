// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source,
//     Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_DATAHANDLING_LOADCSNSNEXUS_H_
#define MANTID_DATAHANDLING_LOADCSNSNEXUS_H_

#include "MantidAPI/Algorithm.h"
#include "MantidDataHandling/DllConfig.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidNexus/NeXusFile.hpp"
/****************************************/

namespace Mantid {
namespace DataHandling {

/** LoadCSNSNexus : TODO: DESCRIPTION
 */
class MANTID_DATAHANDLING_DLL LoadCSNSNexus final : public API::Algorithm {
public:
  const std::string name() const override;
  /// Summary of algorithms purpose
  const std::string summary() const override { return "Loads an CSNS NeXus file into a group workspace."; }

  int version() const override;
  const std::vector<std::string> seeAlso() const override { return {""}; }
  const std::string category() const override;

  Types::Core::DateAndTime getExperimentTime(const std::string &typeName);

  std::vector<std::string> getModules(const std::string &inst, const std::vector<std::string> &inputNames);
  bool checkBanknames(const std::vector<std::string> &inputNames);
  std::vector<std::string> getGPPDModules(const std::string &bankName);
  std::vector<int64_t> getPixelId(const std::vector<std::string> &inputList);
  std::vector<uint32_t> getTimeBin(const std::string &typeName);
  std::vector<uint32_t> getHistData(const std::vector<std::string> &inputList);
  void loadHistData(API::MatrixWorkspace_sptr &workspace, const std::vector<uint32_t> &timeOfFlight, size_t pidNums,
                    const std::vector<uint32_t> &histData);

  std::multimap<uint32_t, std::pair<float, int64_t>> getEventData(const std::vector<std::string> &inputList,
                                                                  const std::vector<uint32_t> &startList,
                                                                  const std::vector<uint32_t> &endList,
                                                                  const std::vector<int64_t> &pids);
  void loadEventData(DataObjects::EventWorkspace_sptr &workspace, const std::vector<uint32_t> &timeOfFlight,
                     size_t pidNums, const std::multimap<uint32_t, std::pair<float, int64_t>> &evtData);

private:
  void init() override;
  void exec() override;
  std::unique_ptr<::NeXus::File> m_file;
  std::string m_entry;
  std::vector<std::string> m_modules;
  std::vector<std::string> m_monitors;
};

} // namespace DataHandling
} // namespace Mantid

#endif /* MANTID_DATAHANDLING_LOADCSNSNEXUS_H_ */
