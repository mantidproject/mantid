// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/ConfigService.h"
#include <cxxtest/TestSuite.h>
#include <filesystem>
#include <gmock/gmock.h>

#include "MantidAPI/ScriptRepositoryFactory.h"
#include "MantidQtWidgets/Common/RepoModel.h"
#include "MantidQtWidgets/Common/ScriptRepositoryView.h"
#include "MockScriptRepository.h"

#include <QAbstractItemModel>

using namespace MantidQt::API;

class RepoModelTest : public CxxTest::TestSuite {

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static RepoModelTest *createSuite() { return new RepoModelTest(); }
  static void destroySuite(RepoModelTest *suite) { delete suite; }

  void setUp() override {
    if (Mantid::API::ScriptRepositoryFactory::Instance().exists("ScriptRepositoryImpl")) {
      Mantid::API::ScriptRepositoryFactory::Instance().unsubscribe("ScriptRepositoryImpl");
    }
    Mantid::API::ScriptRepositoryFactory::Instance().subscribe<testing::NiceMock<MockScriptRepositoryImpl>>(
        "ScriptRepositoryImpl");
  }

  void tearDown() override { Mantid::API::ScriptRepositoryFactory::Instance().unsubscribe("ScriptRepositoryImpl"); }

  // test the repo is create and contains the right files
  void test_create_instance() {
    // fake repo contains the files Repo/README.txt, Repo/TofConverter.py,
    // Repo/reflectometry/Reduction.py and Repo/reflectometry/script.py
    auto *model = new RepoModel();
    // contains TofConv and reflectometry folders
    TS_ASSERT_EQUALS(1, model->rowCount());
    TS_ASSERT_EQUALS(4, model->columnCount());
    auto index = model->index(0, 0);
    // in TofConv folder should be README.txt and TofConverter.py and
    // reflectometry
    TS_ASSERT_EQUALS(3, model->rowCount(index));
    // in reflectometry folder should be script.py
    TS_ASSERT_EQUALS(2, model->rowCount(model->index(2, 0, index)));
  }

  // test the data in the first column is displayed correctly. This column
  // contains the name of the file
  void test_data_first_column_entries() {
    auto *model = new RepoModel();
    int role = Qt::DisplayRole;
    std::string expectedEntries[6] = {"Repo",          "README.txt",   "TofConverter.py",
                                      "reflectometry", "Reduction.py", "script.py"};
    for (auto i = 0; i < 6; ++i) {
      auto index = getIndex(model, i, 0);
      auto rowEntry = model->data(index, role).toString().toStdString();
      TS_ASSERT_EQUALS(expectedEntries[i], rowEntry);
    }
  }

  // test the data in the second column is displayed correctly. This column
  // contains the status of the file
  void test_data_second_column_entries() {
    auto *model = new RepoModel();
    int role = Qt::DisplayRole;
    std::string expectedEntries[6] = {"LOCAL_ONLY", "UPDATED",        "REMOTE_ONLY",
                                      "CHANGED",    "REMOTE_CHANGED", "LOCAL_CHANGED"};
    for (auto i = 0; i < 6; ++i) {
      auto index = getIndex(model, i, 1);
      auto rowEntry = model->data(index, role).toString().toStdString();
      TS_ASSERT_EQUALS(expectedEntries[i], rowEntry);
    }
  }

  // test the data in the first column is displayed correctly. This column
  // contains whether the file is set to update.
  void test_data_third_column_entries() {
    auto *model = new RepoModel();
    int role = Qt::DisplayRole;
    auto filenames = scriptRepoMock.listFiles();
    // expect empty string for local only and remote only as not applicable
    std::string expectedEntries[6] = {"", "false", "", "false", "true", "true"};
    for (auto i = 0; i < 6; ++i) {
      auto index = getIndex(model, i, 2);
      auto rowEntry = model->data(index, role).toString().toStdString();
      TS_ASSERT_EQUALS(expectedEntries[i], rowEntry);
    }
  }

  // test the data in the fourth column is displayed correctly. This column
  // contains whether the file can be deleted.
  void test_data_fourth_column_entries() {
    auto *model = new RepoModel();
    int role = Qt::DisplayRole;
    auto filenames = scriptRepoMock.listFiles();
    std::string expectedEntries[6] = {"protected", "deletable", "protected", "protected", "protected", "deletable"};
    for (auto i = 0; i < 6; ++i) {
      auto index = getIndex(model, i, 3);
      auto rowEntry = model->data(index, role).toString().toStdString();
      TS_ASSERT_EQUALS(expectedEntries[i], rowEntry);
    }
  }

  // test setData will set the file to autoupdate if selected
  void test_setData_sets_autoUpdate() {
    for (const auto &testValue : {true, false}) {
      RepoModel *model = new RepoModel();
      auto index = getIndex(model, 1, 2);
      QString value = (testValue) ? "setTrue" : "setFalse";
      TS_ASSERT_EQUALS(true, model->setData(index, value, Qt::EditRole));
      TS_ASSERT_EQUALS(testValue, model->data(index, Qt::DisplayRole));
    }
  }

  // test setData will not change the data if the index is out of range
  void test_setData_index_out_of_range() {
    auto *model = new RepoModel();
    QModelIndex index = model->index(10, 10);
    QString value = "";
    int role = Qt::EditRole;
    TS_ASSERT_EQUALS(false, model->setData(index, value, role))
  }

  // test setData will not change the data if the role is not Qt::EditRole
  void test_setData_not_editable() {
    auto *model = new RepoModel();
    auto index = getIndex(model, 1, 1);
    QString value = "";
    int role = Qt::DisplayRole;
    TS_ASSERT_EQUALS(false, model->setData(index, value, role))
  }

  // test setData will not change the data in the first column - the path of the
  // file
  void test_setData_column_0_should_not_change() {
    auto *model = new RepoModel();
    auto index = getIndex(model, 1, 0);
    QString value = "";
    int role = Qt::EditRole;
    TS_ASSERT_EQUALS(false, model->setData(index, value, role))
  }

  void test_file_description_gives_file_path() {
    auto *model = new RepoModel();
    auto filenames = scriptRepoMock.listFiles();
    for (auto i = 0; i < 6; ++i) {
      for (auto j = 0; j < model->columnCount(); ++j) {
        auto index = getIndex(model, i, j);
        auto description = model->fileDescription(index).toStdString();
        TS_ASSERT_EQUALS(filenames[i], description)
      }
    }
  }

  void test_author_returns_correct_author() {
    auto *model = new RepoModel();
    for (auto i = 0; i < 6; ++i) {
      for (auto j = 0; j < model->columnCount(); ++j) {
        auto index = getIndex(model, i, j);
        auto author = model->author(index).toStdString();
        TS_ASSERT_EQUALS("Joe Bloggs", author)
      }
    }
  }

  void test_filepath_returns_correct_path() {
    auto repo_path = Mantid::Kernel::ConfigService::Instance().getString("ScriptLocalRepository");
    auto *model = new RepoModel();
    auto filenames = scriptRepoMock.listFiles();
    for (auto i = 0; i < 6; ++i) {
      std::string expectedPath = "";
      if (scriptRepoMock.info(filenames[i]).directory == false &&
          scriptRepoMock.fileStatus(filenames[i]) != REMOTE_ONLY)
        expectedPath = expectedPath = repo_path + "/" + filenames[i];
      for (auto j = 0; j < model->columnCount(); ++j) {
        auto index = getIndex(model, i, j);
        auto path = model->filePath(index).toStdString();
        TS_ASSERT_EQUALS(expectedPath, path)
      }
    }
  }

  void test_repo_ptr_invalidated_on_install_dir_move() {
    auto repoPath = Mantid::Kernel::ConfigService::Instance().getString("ScriptLocalRepository");
    auto *model = new RepoModel();
    const auto differentLocation = std::filesystem::temp_directory_path() / "tempScriptRepo";
    std::filesystem::copy(repoPath, differentLocation, std::filesystem::copy_options::recursive);
    std::filesystem::remove_all(repoPath);
    EXPECT_CALL(scriptRepoMock, setValid(false)).Times(2);
    std::filesystem::copy(differentLocation, repoPath, std::filesystem::copy_options::recursive);
  }

private:
  testing::NiceMock<MockScriptRepositoryImpl> scriptRepoMock;

  // gets the index of the data assuming the repo contains the fake file:
  // TofConv/README.txt, TofConv/TofConverter.py and
  // reflectometry/offspec/script.py
  QModelIndex getIndex(RepoModel *model, int row, int column) {
    if (row == 1 || row == 2 || row == 3) {
      auto folderIndex = model->index(0, 0);
      return model->index(row - 1, column, folderIndex);
    } else if (row == 4 || row == 5) {
      auto firstFolder = model->index(0, 0);
      auto secondFolder = model->index(2, 0, firstFolder);
      return model->index(row - 4, column, secondFolder);
    } else {
      return model->index(row, column);
    }
  }
};
