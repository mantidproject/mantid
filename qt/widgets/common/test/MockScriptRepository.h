// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

#ifndef MANTIDWIDGETS_MOCKSCRIPTREPOSITORY_H_
#define MANTIDWIDGETS_MOCKSCRIPTREPOSITORY_H_
#include "MantidAPI/ScriptRepository.h"
#include "MantidKernel/WarningSuppressions.h"
#include <gmock/gmock.h>

GNU_DIAG_OFF_SUGGEST_OVERRIDE

namespace Mantid {
namespace API {

class MockScriptRepositoryImpl : public ScriptRepository {
public:
  MOCK_METHOD1(info, ScriptInfo(const std::string &));
  MOCK_METHOD0(listFiles, std::vector<std::string>());
  MOCK_METHOD1(download, void(const std::string &));
  MOCK_METHOD1(fileStatus, SCRIPTSTATUS(const std::string &));
  MOCK_METHOD0(isValid, bool());
  MOCK_METHOD1(install, void(const std::string &));
  MOCK_METHOD1(connect, void(const std::string &));
  MOCK_METHOD0(check4Update, std::vector<std::string>());
  MOCK_METHOD4(upload, void(const std::string &, const std::string &,
                            const std::string &, const std::string &));
  MOCK_METHOD4(remove, void(const std::string &, const std::string &,
                            const std::string &, const std::string &));
  MOCK_METHOD1(setIgnorePatterns, void(const std::string &));
  MOCK_METHOD0(ignorePatterns, std::string());
  MOCK_METHOD2(setAutoUpdate, int(const std::string &, bool));

  const std::string &
  MockScriptRepositoryImpl::description(const std::string &path) {
    return path;
  }
};
} // namespace API
} // namespace Mantid
GNU_DIAG_ON_SUGGEST_OVERRIDE
#endif /*MANTIDWIDGETS_MOCKSCRIPTREPOSITORY_H_ */
