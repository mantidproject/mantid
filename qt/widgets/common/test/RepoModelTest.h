// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDWIDGETS_REPOMODELTEST_H_
#define MANTIDWIDGETS_REPOMODELTEST_H_

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>

#include "MantidAPI/ScriptRepositoryFactory.h"
#include "MantidQtWidgets/Common/RepoModel.h"
#include "MantidQtWidgets/Common/ScriptRepositoryView.h"
#include "MockScriptRepository.h"

using namespace MantidQt::API;

class RepoModelTest : public CxxTest::TestSuite {

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static RepoModelTest *createSuite() { return new RepoModelTest(); }
  static void destroySuite(RepoModelTest *suite) { delete suite; }

  void setUp() override {
    Mantid::API::ScriptRepositoryFactory::Instance()
        .subscribe<testing::NiceMock<MockScriptRepositoryImpl>>(
            "ScriptRepositoryImpl");
  }

  void tearDown() override {
    Mantid::API::ScriptRepositoryFactory::Instance().unsubscribe(
        "ScriptRepositoryImpl");
  }

  void test_create_instance() {
    RepoModel *model = new RepoModel();
    TS_ASSERT_EQUALS(6, model->rowCount());
    TS_ASSERT_EQUALS(4, model->columnCount());
  }

  void test_data_first_column_entries_display_role() {
    // name column
    RepoModel *model = new RepoModel();
    int role = Qt::DisplayRole;
    auto expectedEntries = scriptRepoMock.listFiles();
    for (auto i = 0; i < model->rowCount(); ++i) {
      QModelIndex index = model->index(i, 0);
      auto rowEntry = model->data(index, role).toString().toStdString();
      TS_ASSERT_EQUALS(expectedEntries[i], rowEntry);
    }
  }

  void test_data_second_column_entries() {
    // status column
    RepoModel *model = new RepoModel();
    int role = Qt::DisplayRole;
    std::string expectedEntries[6] = {"UPDATED",       "REMOTE_ONLY",
                                      "LOCAL_ONLY",    "REMOTE_CHANGED",
                                      "LOCAL_CHANGED", "CHANGED"};
    for (auto i = 0; i < model->rowCount(); ++i) {
      QModelIndex index = model->index(i, 1);
      auto rowEntry = model->data(index, role).toString().toStdString();
      TS_ASSERT_EQUALS(expectedEntries[i], rowEntry);
    }
  }

  void test_data_third_column_entries() {
    // auto update column
    RepoModel *model = new RepoModel();
    int role = Qt::DisplayRole;
    auto filenames = scriptRepoMock.listFiles();
    std::string expectedEntries[6] = {"false", "", "", "true", "true", "false"};
    for (auto i = 0; i < model->rowCount(); ++i) {
      QModelIndex index = model->index(i, 2);
      auto rowEntry = model->data(index, role).toString().toStdString();
      TS_ASSERT_EQUALS(expectedEntries[i], rowEntry);
    }
  }

  void test_data_fourth_column_entries() {
    // delete column
    RepoModel *model = new RepoModel();
    int role = Qt::DisplayRole;
    auto filenames = scriptRepoMock.listFiles();
    std::string expectedEntries[6] = {"deletable", "protected", "protected",
                                      "protected", "deletable", "protected"};
    for (auto i = 0; i < model->rowCount(); ++i) {
      QModelIndex index = model->index(i, 3);
      auto rowEntry = model->data(index, role).toString().toStdString();
      TS_ASSERT_EQUALS(expectedEntries[i], rowEntry);
    }
  }

  void test_setData_sets_download() {
    RepoModel *model = new RepoModel();
    QModelIndex index = model->index(1, 1);
    auto isDataSet = model->setData(index, "Download", Qt::EditRole);
    TS_ASSERT_EQUALS(true, isDataSet);
    auto row = model->data(index, Qt::DisplayRole).toString();
    TS_ASSERT_EQUALS("DOWNLOADING", row);
  }

  void test_setData_sets_autoUpdate() {
    for (const auto &testValue : {true, false}) {
      RepoModel *model = new RepoModel();
      QModelIndex index = model->index(0, 2);
      QString value = (testValue) ? "setTrue" : "setFalse";
      TS_ASSERT_EQUALS(true, model->setData(index, value, Qt::EditRole));
      TS_ASSERT_EQUALS(testValue, model->data(index, Qt::DisplayRole));
    }
  }

  void test_setData_does_delete() {
    RepoModel *model = new RepoModel();
    QModelIndex index = model->index(0, 3);
    auto filenames = scriptRepoMock.listFiles();
    auto isDataSet = model->setData(index, "delete", Qt::EditRole);
    TS_ASSERT_EQUALS(true, isDataSet);
    auto row = model->data(index, Qt::DisplayRole).toString();
    TS_ASSERT_EQUALS("DOWNLOADING", row);
  }

  void test_setData_index_out_of_range() {
    RepoModel *model = new RepoModel();
    QModelIndex index = model->index(10, 10);
    QString value = "";
    int role = Qt::EditRole;
    TS_ASSERT_EQUALS(false, model->setData(index, value, role))
  }

  void test_setData_not_editable() {
    RepoModel *model = new RepoModel();
    QModelIndex index = model->index(1, 1);
    QString value = "";
    int role = Qt::DisplayRole;
    TS_ASSERT_EQUALS(false, model->setData(index, value, role))
  }

  void test_setData_column_0_should_not_change() {
    RepoModel *model = new RepoModel();
    QModelIndex index = model->index(1, 0);
    QString value = "";
    int role = Qt::EditRole;
    TS_ASSERT_EQUALS(false, model->setData(index, value, role))
  }

private:
  testing::NiceMock<MockScriptRepositoryImpl> scriptRepoMock;
};

#endif /* MANTIDWIDGETS_REPOMODELTEST_H_ */
