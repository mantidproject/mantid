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

using namespace Mantid::API;

typedef std::pair<std::string, std::tuple<SCRIPTSTATUS, bool, bool>> fileType;

GNU_DIAG_OFF_SUGGEST_OVERRIDE

class MockScriptRepositoryImpl : public ScriptRepository {
public:
  MockScriptRepositoryImpl() { addFakeFiles(); }
  MOCK_METHOD1(download, void(const std::string &));
  MOCK_METHOD1(install, void(const std::string &));
  MOCK_METHOD1(connect, void(const std::string &));
  MOCK_METHOD0(check4Update, std::vector<std::string>());
  MOCK_METHOD4(upload, void(const std::string &, const std::string &,
                            const std::string &, const std::string &));
  MOCK_METHOD4(remove, void(const std::string &, const std::string &,
                            const std::string &, const std::string &));
  MOCK_METHOD1(setIgnorePatterns, void(const std::string &));
  MOCK_METHOD0(ignorePatterns, std::string());

  ScriptInfo info(const std::string &path) {
    ScriptInfo info;
    info.auto_update = std::get<1>(testFiles[path]);
    info.directory = std::get<2>(testFiles[path]);
    return info;
  }

  int setAutoUpdate(const std::string &input_path, bool option) {
    std::get<1>(testFiles[input_path]) = option;
    return 1;
  }

  const std::string &
  MockScriptRepositoryImpl::description(const std::string &path) override {
    return path;
  }

  bool MockScriptRepositoryImpl::isValid() override { return true; }

  std::vector<std::string> MockScriptRepositoryImpl::listFiles() override {
    return fileNames;
  }

  SCRIPTSTATUS fileStatus(const std::string &file_path) {
    return std::get<0>(testFiles[file_path]);
  }

private:
  // contains fake file entries: path, status, autoupdate, directory
  std::map<std::string, std::tuple<SCRIPTSTATUS, bool, bool>> testFiles;

  std::vector<std::string> fileNames = {"TofConv",         "README.txt",
                                        "reflectometry",   "Quick.py",
                                        "TofConverter.py", "otherFile.py"};
  void addFakeFiles() {
    testFiles.insert(
        fileType(fileNames[0], std::make_tuple(BOTH_UNCHANGED, false, false)));
    testFiles.insert(
        fileType(fileNames[1], std::make_tuple(REMOTE_ONLY, false, false)));
    testFiles.insert(
        fileType(fileNames[2], std::make_tuple(LOCAL_ONLY, false, true)));
    testFiles.insert(
        fileType(fileNames[3], std::make_tuple(REMOTE_CHANGED, true, false)));
    testFiles.insert(
        fileType(fileNames[4], std::make_tuple(LOCAL_CHANGED, true, false)));
    testFiles.insert(
        fileType(fileNames[5], std::make_tuple(BOTH_CHANGED, false, true)));
  }
};

GNU_DIAG_ON_SUGGEST_OVERRIDE
#endif /*MANTIDWIDGETS_MOCKSCRIPTREPOSITORY_H_ */
