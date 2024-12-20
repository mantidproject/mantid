// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/ScriptRepository.h"
#include "MantidKernel/WarningSuppressions.h"
#include <gmock/gmock.h>

using namespace Mantid::API;

using fileType = std::pair<std::string, std::tuple<SCRIPTSTATUS, bool, bool>>;

GNU_DIAG_OFF_SUGGEST_OVERRIDE

class MockScriptRepositoryImpl : public ScriptRepository {
public:
  // need to mock out download and install
  MockScriptRepositoryImpl() { addFakeFiles(); }
  MOCK_METHOD1(download, void(const std::string &));
  MOCK_METHOD1(install, void(const std::string &));
  MOCK_METHOD1(connect, void(const std::string &));
  MOCK_METHOD0(check4Update, std::vector<std::string>());
  MOCK_METHOD4(upload, void(const std::string &, const std::string &, const std::string &, const std::string &));
  MOCK_METHOD4(remove, void(const std::string &, const std::string &, const std::string &, const std::string &));
  MOCK_METHOD1(setIgnorePatterns, void(const std::string &));
  MOCK_METHOD0(ignorePatterns, std::string());

  ScriptInfo info(const std::string &path) {
    ScriptInfo info;
    info.auto_update = std::get<1>(testFiles[path]);
    info.directory = std::get<2>(testFiles[path]);
    info.author = "Joe Bloggs";
    return info;
  }

  int setAutoUpdate(const std::string &input_path, bool option) {
    std::get<1>(testFiles[input_path]) = option;
    return 1;
  }

  const std::string &description(const std::string &path) override { return path; }

  bool isValid() override { return true; }

  std::vector<std::string> listFiles() override { return filePaths; }

  SCRIPTSTATUS fileStatus(const std::string &file_path) { return std::get<0>(testFiles[file_path]); }

  std::string localRepository() override {
    return Mantid::Kernel::ConfigService::Instance().getString("ScriptLocalRepository");
  }

private:
  // contains fake file entries: path, status, autoupdate, directory
  std::map<std::string, std::tuple<SCRIPTSTATUS, bool, bool>> testFiles;

  std::vector<std::string> filePaths = {"Repo",
                                        "Repo/README.txt",
                                        "Repo/TofConverter.py",
                                        "Repo/reflectometry",
                                        "Repo/reflectometry/Reduction.py",
                                        "Repo/reflectometry/script.py"};
  void addFakeFiles() {
    testFiles.insert(fileType(filePaths[0], std::make_tuple(LOCAL_ONLY, false, true)));
    testFiles.insert(fileType(filePaths[1], std::make_tuple(BOTH_UNCHANGED, false, false)));
    testFiles.insert(fileType(filePaths[2], std::make_tuple(REMOTE_ONLY, false, false)));
    testFiles.insert(fileType(filePaths[3], std::make_tuple(BOTH_CHANGED, false, true)));
    testFiles.insert(fileType(filePaths[4], std::make_tuple(REMOTE_CHANGED, true, false)));
    testFiles.insert(fileType(filePaths[5], std::make_tuple(LOCAL_CHANGED, true, false)));
  }
};

GNU_DIAG_ON_SUGGEST_OVERRIDE
