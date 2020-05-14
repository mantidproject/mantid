// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_DATAHANDLING_LOADCSNSNEXUS_H_
#define MANTID_DATAHANDLING_LOADCSNSNEXUS_H_

#include "MantidAPI/IFileLoader.h"
#include "MantidKernel/NexusDescriptor.h"
#include "MantidKernel/System.h"
//#include <nexus/NeXusException.hpp>
#include <nexus/NeXusFile.hpp>
/****************************************/
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/LatticeDomain.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/PeaksWorkspace.h"

#include <boost/shared_ptr.hpp>

using Mantid::Kernel::V3D;

using namespace Mantid::API;

/****************************************/
using namespace Mantid::DataObjects;

namespace Mantid {
namespace DataHandling {

using namespace std;
using namespace DataObjects;
/** LoadCSNSNexus : TODO: DESCRIPTION
 */
class DLLExport LoadCSNSNexus
    : public API::IFileLoader<Kernel::NexusDescriptor> {
public:
  const std::string name() const override;
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Loads an CSNS NeXus file into a group workspace.";
  }

  int version() const override;
  const std::vector<std::string> seeAlso() const override {
    return {"LoadMcStas"};
  }
  const std::string category() const override;

  /// Returns a confidence value that this algorithm can load a file
  int confidence(Kernel::NexusDescriptor &descriptor) const override;

  Types::Core::DateAndTime getExperimentTime(std::string typeName);

  std::vector<std::string>
  getModules(std::string inst, const std::vector<std::string> &inputNames);
  bool checkBanknames(const std::vector<std::string> &inputNames);
  std::vector<std::string> getGPPDModules(std::string bankName);
  std::vector<int64_t> getPixelId(const std::vector<std::string> &inputList);
  std::vector<uint32_t> getTimeBin(std::string typeName);
  std::vector<uint32_t> getHistData(const vector<string> &inputList);
  void loadHistData(boost::shared_ptr<API::MatrixWorkspace> workspace,
                    std::vector<uint32_t> &timeOfFlight, size_t pidNums,
                    std::vector<uint32_t> &histData);

  std::multimap<uint32_t, std::pair<float, int64_t>>
  getEventData(const vector<string> &inputList,
               const vector<uint32_t> &startList,
               const vector<uint32_t> &endList, const vector<int64_t> &pids);
  void
  loadEventData(boost::shared_ptr<DataObjects::EventWorkspace> workspace,
                const std::vector<uint32_t> &timeOfFlight, size_t pidNums,
                std::multimap<uint32_t, std::pair<float, int64_t>> evtData);

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
