// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef CONVERTTABLETOMATRIXWORKSPACETEST_H_
#define CONVERTTABLETOMATRIXWORKSPACETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAlgorithms/ConvertTableToMatrixWorkspace.h"
#include "MantidKernel/Unit.h"

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::Kernel;

class ConvertTableToMatrixWorkspaceTest : public CxxTest::TestSuite {
public:
  void testName() {
    TS_ASSERT_EQUALS(m_converter->name(), "ConvertTableToMatrixWorkspace")
  }

  void testVersion() { TS_ASSERT_EQUALS(m_converter->version(), 1) }

  void testInit() {
    TS_ASSERT_THROWS_NOTHING(m_converter->initialize())
    TS_ASSERT(m_converter->isInitialized())
  }

  void testExec() {

    ITableWorkspace_sptr tws = WorkspaceFactory::Instance().createTable();
    tws->addColumn("int", "A");
    tws->addColumn("double", "B");
    tws->addColumn("double", "C");

    size_t n = 10;
    for (size_t i = 0; i < n; ++i) {
      TableRow row = tws->appendRow();
      int x = int(i);
      double y = x * 1.1;
      double e = sqrt(y);
      row << x << y << e;
    }

    TS_ASSERT_THROWS_NOTHING(m_converter->setProperty("InputWorkspace", tws));
    TS_ASSERT_THROWS_NOTHING(
        m_converter->setPropertyValue("OutputWorkspace", "out"));
    TS_ASSERT_THROWS_NOTHING(m_converter->setPropertyValue("ColumnX", "A"));
    TS_ASSERT_THROWS_NOTHING(m_converter->setPropertyValue("ColumnY", "B"));
    TS_ASSERT_THROWS_NOTHING(m_converter->setPropertyValue("ColumnE", "C"));

    TS_ASSERT(m_converter->execute());

    MatrixWorkspace_sptr mws = boost::dynamic_pointer_cast<MatrixWorkspace>(
        API::AnalysisDataService::Instance().retrieve("out"));

    TS_ASSERT(mws);
    TS_ASSERT_EQUALS(mws->getNumberHistograms(), 1);
    TS_ASSERT(!mws->isHistogramData());
    TS_ASSERT_EQUALS(mws->blocksize(), tws->rowCount());

    auto &X = mws->x(0);
    auto &Y = mws->y(0);
    auto &E = mws->e(0);

    for (size_t i = 0; i < tws->rowCount(); ++i) {
      TableRow row = tws->getRow(i);
      int x;
      double y, e;
      row >> x >> y >> e;
      TS_ASSERT_EQUALS(static_cast<double>(x), X[i]);
      TS_ASSERT_EQUALS(y, Y[i]);
      TS_ASSERT_EQUALS(e, E[i]);
    }

    boost::shared_ptr<Units::Label> label =
        boost::dynamic_pointer_cast<Units::Label>(mws->getAxis(0)->unit());
    TS_ASSERT(label);
    TS_ASSERT_EQUALS(label->caption(), "A");
    TS_ASSERT_EQUALS(mws->YUnitLabel(), "B");

    API::AnalysisDataService::Instance().remove("out");
  }

  void test_Default_ColumnE() {
    size_t n = 10;
    for (size_t i = 0; i < n; ++i) {
      TableRow row = tws->appendRow();
      double x = static_cast<double>(i);
      double y = x * 1.1;
      row << x << y;
    }

    TS_ASSERT(m_converter->execute());

    MatrixWorkspace_sptr mws =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("out");

    TS_ASSERT(mws);
    TS_ASSERT_EQUALS(mws->getNumberHistograms(), 1);
    TS_ASSERT(!mws->isHistogramData());
    TS_ASSERT_EQUALS(mws->blocksize(), tws->rowCount());

    auto &X = mws->x(0);
    auto &Y = mws->y(0);
    auto &E = mws->e(0);

    for (size_t i = 0; i < tws->rowCount(); ++i) {
      TableRow row = tws->getRow(i);
      double x, y;
      row >> x >> y;
      TS_ASSERT_EQUALS(double(x), X[i]);
      TS_ASSERT_EQUALS(y, Y[i]);
      TS_ASSERT_EQUALS(0.0, E[i]);
    }

    API::AnalysisDataService::Instance().remove("out");
  }

  void test_fail_on_empty_table() {
    TS_ASSERT_THROWS(m_converter->execute(), const std::runtime_error &);
  }

  void setUp() override {
    tws = WorkspaceFactory::Instance().createTable();
    tws->addColumn("double", "A");
    tws->addColumn("double", "B");

    m_converter =
        boost::make_shared<Mantid::Algorithms::ConvertTableToMatrixWorkspace>();
    m_converter->setRethrows(true);
    m_converter->initialize();
    TS_ASSERT_THROWS_NOTHING(m_converter->setProperty("InputWorkspace", tws));
    TS_ASSERT_THROWS_NOTHING(
        m_converter->setPropertyValue("OutputWorkspace", "out"));
    TS_ASSERT_THROWS_NOTHING(m_converter->setPropertyValue("ColumnX", "A"));
    TS_ASSERT_THROWS_NOTHING(m_converter->setPropertyValue("ColumnY", "B"));
  }
  // test if it can convert a string to a double, if the string is numeric
  void testStringToDouble() {

    auto tws = WorkspaceFactory::Instance().createTable();
    tws->addColumn("str", "A");
    tws->addColumn("double", "B");
    tws->addColumn("double", "C");

    size_t n = 10;
    for (size_t i = 0; i < n; ++i) {
      TableRow row = tws->appendRow();
      std::string x = "1";
      double y = static_cast<double>(i) * 1.1;
      double e = sqrt(y);
      row << x << y << e;
    }

    TS_ASSERT_THROWS_NOTHING(m_converter->setProperty("InputWorkspace", tws));
    TS_ASSERT_THROWS_NOTHING(
        m_converter->setPropertyValue("OutputWorkspace", "out"));
    TS_ASSERT_THROWS_NOTHING(m_converter->setPropertyValue("ColumnX", "A"));
    TS_ASSERT_THROWS_NOTHING(m_converter->setPropertyValue("ColumnY", "B"));
    TS_ASSERT_THROWS_NOTHING(m_converter->setPropertyValue("ColumnE", "C"));

    TS_ASSERT(m_converter->execute());

    MatrixWorkspace_sptr mws = boost::dynamic_pointer_cast<MatrixWorkspace>(
        API::AnalysisDataService::Instance().retrieve("out"));

    TS_ASSERT(mws);
    TS_ASSERT_EQUALS(mws->getNumberHistograms(), 1);
    TS_ASSERT(!mws->isHistogramData());
    TS_ASSERT_EQUALS(mws->blocksize(), tws->rowCount());

    auto &X = mws->x(0);
    auto &Y = mws->y(0);
    auto &E = mws->e(0);

    for (size_t i = 0; i < tws->rowCount(); ++i) {
      TableRow row = tws->getRow(i);
      std::string x;
      double y, e;
      row >> x >> y >> e;
      TS_ASSERT_EQUALS(std::stod(x), X[i]);
      TS_ASSERT_EQUALS(y, Y[i]);
      TS_ASSERT_EQUALS(e, E[i]);
    }

    auto label =
        boost::dynamic_pointer_cast<Units::Label>(mws->getAxis(0)->unit());
    TS_ASSERT(label);
    TS_ASSERT_EQUALS(label->caption(), "A");
    TS_ASSERT_EQUALS(mws->YUnitLabel(), "B");
    API::AnalysisDataService::Instance().remove("out");
  }
  // test that an error is thrown when a non-numeric string is used
  void testNotANumber() {

    ITableWorkspace_sptr tws = WorkspaceFactory::Instance().createTable();
    tws->addColumn("str", "A");
    tws->addColumn("double", "B");
    tws->addColumn("double", "C");

    size_t n = 10;
    for (size_t i = 0; i < n; ++i) {
      TableRow row = tws->appendRow();
      std::string x = "not a number";
      double y = static_cast<double>(i) * 1.1;
      double e = sqrt(y);
      row << x << y << e;
    }

    TS_ASSERT_THROWS_NOTHING(m_converter->setProperty("InputWorkspace", tws));
    TS_ASSERT_THROWS_NOTHING(
        m_converter->setPropertyValue("OutputWorkspace", "out"));
    TS_ASSERT_THROWS_NOTHING(m_converter->setPropertyValue("ColumnX", "A"));
    TS_ASSERT_THROWS_NOTHING(m_converter->setPropertyValue("ColumnY", "B"));
    TS_ASSERT_THROWS_NOTHING(m_converter->setPropertyValue("ColumnE", "C"));

    TS_ASSERT_THROWS(m_converter->execute(), const std::invalid_argument &);
    API::AnalysisDataService::Instance().remove("out");
  }

private:
  IAlgorithm_sptr m_converter;
  ITableWorkspace_sptr tws;
};

#endif /*CONVERTTABLETOMATRIXWORKSPACETEST_H_*/
