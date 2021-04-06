// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/IJournal.h"
#include "MantidDataHandling/DllConfig.h"
#include "MantidKernel/InternetHelper.h"
#include "MantidKernel/System.h"

#include <Poco/AutoPtr.h>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace Poco::XML {
class Document;
}

namespace Mantid {

namespace DataHandling {
/**
 * ISISJournal: Helper class to aid in fetching ISIS specific run information
 * from journal files
 */

class MANTID_DATAHANDLING_DLL ISISJournal : public API::IJournal {
public:
  ISISJournal(std::string const &instrument, std::string const &cycle,
              std::unique_ptr<Kernel::InternetHelper> internetHelper = std::make_unique<Kernel::InternetHelper>());
  virtual ~ISISJournal();

  ISISJournal(ISISJournal const &rhs) = delete;
  ISISJournal(ISISJournal &&rhs);
  ISISJournal const &operator=(ISISJournal const &rhs) = delete;
  ISISJournal &operator=(ISISJournal &&rhs);

  /// Get the list of cycle names
  std::vector<std::string> getCycleNames() override;
  /// Get data for runs that match the given filters
  std::vector<RunData> getRuns(std::vector<std::string> const &valuesToLookup = {},
                               RunData const &filters = RunData()) override;

private:
  std::unique_ptr<Kernel::InternetHelper> m_internetHelper;
  std::string m_runsFileURL;
  std::string m_indexFileURL;
  Poco::AutoPtr<Poco::XML::Document> m_runsDocument;
  Poco::AutoPtr<Poco::XML::Document> m_indexDocument;

  std::string getURLContents(std::string const &url);
};
} // namespace DataHandling
} // namespace Mantid
