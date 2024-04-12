// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "../ReflMockObjects.h"
#include "GUI/Save/AsciiSaver.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include "MockSaveAlgorithmRunner.h"

#include <Poco/Path.h>
#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>

using namespace MantidQt::CustomInterfaces::ISISReflectometry;
using Mantid::API::AnalysisDataService;
using Mantid::DataObjects::Workspace2D_sptr;
using ::testing::_;
using ::testing::Return;

class AsciiSaverTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static AsciiSaverTest *createSuite() { return new AsciiSaverTest(); }
  static void destroySuite(AsciiSaverTest *suite) { delete suite; }

  void test_save_ascii_algorithm_called_for_ANSTO_format() {
    runTestSaveAsciiAlgorithmCalledForFileFormat(NamedFormat::ANSTO);
  }

  void test_save_ascii_algorithm_called_for_Custom_format() {
    runTestSaveAsciiAlgorithmCalledForFileFormat(NamedFormat::Custom);
  }

  void test_save_ascii_algorithm_called_for_ThreeColumn_format() {
    runTestSaveAsciiAlgorithmCalledForFileFormat(NamedFormat::ThreeColumn);
  }

  void test_save_ascii_algorithm_called_for_ILLCosmos_format() {
    runTestSaveAsciiAlgorithmCalledForFileFormat(NamedFormat::ILLCosmos);
  }

  void test_save_orso_algorithm_called_for_ORSOAscii_format() {
    runTestSaveORSOAlgorithmCalledForFileFormat(NamedFormat::ORSOAscii);
  }

  void test_saving_multiple_workspaces_to_separate_files_with_save_ascii_algorithm() {
    std::vector<std::string> const workspacesToSave = {"ws_1", "ws_2", "ws_3"};
    runTestSaveAsciiAlgorithmCalledForFileFormat(NamedFormat::ANSTO, workspacesToSave);
  }

  void test_saving_to_single_file_is_ignored_for_save_ascii_algorithm() {
    std::vector<std::string> const workspacesToSave = {"ws_1", "ws_2", "ws_3"};
    runTestSaveAsciiAlgorithmCalledForFileFormat(NamedFormat::ANSTO, workspacesToSave, true);
  }

  void test_saving_multiple_workspaces_to_separate_files_for_ORSOAscii_format() {
    std::vector<std::string> const workspacesToSave = {"ws_1", "ws_2", "ws_3"};
    runTestSaveORSOAlgorithmCalledForFileFormat(NamedFormat::ORSOAscii, workspacesToSave);
  }

  void test_saving_multiple_workspaces_to_single_file_for_ORSOAscii_format() {
    std::vector<std::string> const workspacesToSave = {"ws_1", "ws_2", "ws_3"};
    runTestSaveORSOAlgorithmCalledForFileFormat(NamedFormat::ORSOAscii, workspacesToSave, true);
  }

  void test_invalid_save_path_throws_exception() {
    auto mockSaveAlgorithmRunner = std::make_unique<MockSaveAlgorithmRunner>();
    auto mockFileHandler = MockFileHandler();
    auto wsNames = createWorkspaces();
    std::vector<std::string> logParams;
    auto formatOptions = createFileFormatOptions(NamedFormat::ANSTO);

    expectIsValidSaveDirectory(mockFileHandler, false);

    auto saver = createSaver(std::move(mockSaveAlgorithmRunner), mockFileHandler);
    TS_ASSERT_THROWS(saver.save(m_saveDirectory, wsNames, logParams, formatOptions), InvalidSavePath const &);
  }

private:
  const bool m_includeHeader = true;
  const bool m_includeQResolution = false;
  const bool m_includeAdditionalColumns = false;
  const std::string m_separator = ",";
  const std::string m_prefix = "test_";
  const std::string m_saveDirectory = "Test";

  AsciiSaver createSaver(std::unique_ptr<MockSaveAlgorithmRunner> saveAlgRunner, MockFileHandler &mockFileHandler) {
    return AsciiSaver(std::move(saveAlgRunner), &mockFileHandler);
  }

  Workspace2D_sptr createWorkspace(const std::string &name) {
    Workspace2D_sptr ws = WorkspaceCreationHelper::create2DWorkspace(10, 10);
    AnalysisDataService::Instance().addOrReplace(name, ws);
    return ws;
  }

  std::vector<std::string> createWorkspaces(std::vector<std::string> const &workspaceNames = {"ws_1"}) {
    for (auto name : workspaceNames) {
      createWorkspace(name);
    }
    return workspaceNames;
  }

  const FileFormatOptions createFileFormatOptions(NamedFormat format, const bool saveAsSingleFile = false) {
    return FileFormatOptions(format, m_prefix, m_includeHeader, m_separator, m_includeQResolution,
                             m_includeAdditionalColumns, saveAsSingleFile);
  }

  const std::string expectedExtension(NamedFormat format) {
    switch (format) {
    case NamedFormat::Custom:
      return "custom";
    case NamedFormat::ThreeColumn:
      return ".dat";
    case NamedFormat::ANSTO:
      return ".txt";
    case NamedFormat::ILLCosmos:
      return ".mft";
    case NamedFormat::ORSOAscii:
      return ".ort";
    default:
      throw std::runtime_error("Unknown save format.");
    }
  }

  std::string expectedSavePath(const std::string &wsName, const NamedFormat format) {
    auto savePath = Poco::Path(m_saveDirectory);

    if (format == NamedFormat::Custom)
      savePath.append(m_prefix + wsName + std::string(".dat"));
    else
      savePath.append(m_prefix + wsName);

    return savePath.toString();
  }

  void expectSaveAsciiAlgorithmCalled(MockSaveAlgorithmRunner &mockSaveAlgorithmRunner, const std::string &wsName,
                                      const NamedFormat format) {
    auto extension = expectedExtension(format);
    auto savePath = expectedSavePath(wsName, format);
    EXPECT_CALL(mockSaveAlgorithmRunner,
                runSaveAsciiAlgorithm(_, savePath, extension, _, m_includeHeader, m_includeQResolution, m_separator))
        .Times(1);
  }

  void expectSaveOrsoAlgorithmCalled(MockSaveAlgorithmRunner &mockSaveAlgorithmRunner, const std::string &wsName,
                                     const NamedFormat format) {
    auto extension = expectedExtension(format);
    auto savePath = expectedSavePath(wsName, format);
    EXPECT_CALL(mockSaveAlgorithmRunner,
                runSaveORSOAlgorithm(_, savePath, m_includeQResolution, m_includeAdditionalColumns))
        .Times(1);
  }

  void expectIsValidSaveDirectory(MockFileHandler &mockFileHandler, bool isValid = true) {
    EXPECT_CALL(mockFileHandler, fileExists(_)).Times(1).WillOnce(Return(isValid));
  }

  void runTestSaveAsciiAlgorithmCalledForFileFormat(NamedFormat format,
                                                    std::vector<std::string> const &workspacesToSave = {"ws_1"},
                                                    const bool saveAsSingleFile = false) {
    auto mockSaveAlgorithmRunner = std::make_unique<MockSaveAlgorithmRunner>();
    auto mockFileHandler = MockFileHandler();
    auto wsNames = createWorkspaces(workspacesToSave);
    std::vector<std::string> logParams;
    auto formatOptions = createFileFormatOptions(format, saveAsSingleFile);

    expectIsValidSaveDirectory(mockFileHandler);
    for (auto const &name : wsNames) {
      expectSaveAsciiAlgorithmCalled(*mockSaveAlgorithmRunner, name, formatOptions.format());
    }

    auto saver = createSaver(std::move(mockSaveAlgorithmRunner), mockFileHandler);
    saver.save(m_saveDirectory, wsNames, logParams, formatOptions);
  }

  void runTestSaveORSOAlgorithmCalledForFileFormat(NamedFormat format,
                                                   std::vector<std::string> const &workspacesToSave = {"ws_1"},
                                                   const bool saveAsSingleFile = false) {
    auto mockSaveAlgorithmRunner = std::make_unique<MockSaveAlgorithmRunner>();
    auto mockFileHandler = MockFileHandler();
    auto wsNames = createWorkspaces(workspacesToSave);
    std::vector<std::string> logParams;
    auto formatOptions = createFileFormatOptions(format, saveAsSingleFile);

    expectIsValidSaveDirectory(mockFileHandler);

    if (saveAsSingleFile) {
      expectSaveOrsoAlgorithmCalled(*mockSaveAlgorithmRunner, workspacesToSave.front(), formatOptions.format());
    } else {
      for (auto const &name : wsNames) {
        expectSaveOrsoAlgorithmCalled(*mockSaveAlgorithmRunner, name, formatOptions.format());
      }
    }

    auto saver = createSaver(std::move(mockSaveAlgorithmRunner), mockFileHandler);
    saver.save(m_saveDirectory, wsNames, logParams, formatOptions);
  }
};
