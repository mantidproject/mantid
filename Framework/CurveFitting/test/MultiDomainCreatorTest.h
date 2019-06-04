// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MULTIDOMAINCREATORTEST_H_
#define MULTIDOMAINCREATORTEST_H_

#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/FunctionDomain1D.h"
#include "MantidAPI/FunctionValues.h"
#include "MantidAPI/IFunction1D.h"
#include "MantidAPI/JointDomain.h"
#include "MantidAPI/MultiDomainFunction.h"
#include "MantidAPI/ParamFunction.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidCurveFitting/FitMW.h"
#include "MantidCurveFitting/Functions/UserFunction.h"
#include "MantidCurveFitting/MultiDomainCreator.h"
#include "MantidKernel/PropertyManager.h"

#include "MantidTestHelpers/FakeObjects.h"

#include <algorithm>
#include <boost/make_shared.hpp>
#include <cxxtest/TestSuite.h>

using namespace Mantid;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::CurveFitting;
using namespace Mantid::CurveFitting::Functions;

class MultiDomainCreatorTest_Fun : public IFunction1D, public ParamFunction {
public:
  size_t m_wsIndex;
  boost::shared_ptr<const MatrixWorkspace> m_workspace;
  std::string name() const override { return "MultiDomainCreatorTest_Fun"; }
  void function1D(double *, const double *, const size_t) const override {}
  void setMatrixWorkspace(boost::shared_ptr<const API::MatrixWorkspace>,
                          size_t wi, double, double) override {
    m_wsIndex = wi;
  }
  void setWorkspace(boost::shared_ptr<const Workspace> ws) override {
    m_workspace = boost::dynamic_pointer_cast<const MatrixWorkspace>(ws);
  }
};

class MultiDomainCreatorTest_Manager : public Kernel::PropertyManager {
public:
  void store(const std::string &propName) {
    dynamic_cast<IWorkspaceProperty *>(getPointerToProperty(propName))->store();
  }
};

class MultiDomainCreatorTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static MultiDomainCreatorTest *createSuite() {
    return new MultiDomainCreatorTest();
  }
  static void destroySuite(MultiDomainCreatorTest *suite) { delete suite; }

  MultiDomainCreatorTest() {
    ws1.reset(new WorkspaceTester);
    ws1->initialize(1, 10, 10);
    {
      auto &x = ws1->mutableX(0);
      // Mantid::MantidVec& e = ws1->dataE(0);
      for (size_t i = 0; i < x.size(); ++i) {
        x[i] = 0.1 * static_cast<double>(i);
      }
      ws1->mutableY(0) = 1.;
    }

    ws2.reset(new WorkspaceTester);
    ws2->initialize(1, 10, 10);
    {
      auto &x = ws2->mutableX(0);
      // Mantid::MantidVec& e = ws2->dataE(0);
      for (size_t i = 0; i < x.size(); ++i) {
        x[i] = 1. + 0.1 * static_cast<double>(i);
      }
      ws2->mutableY(0) = 2.;
    }

    ws3.reset(new WorkspaceTester);
    ws3->initialize(1, 10, 10);
    {
      auto &x = ws3->mutableX(0);
      // Mantid::MantidVec& e = ws3->dataE(0);
      for (size_t i = 0; i < x.size(); ++i) {
        x[i] = 2. + 0.1 * static_cast<double>(i);
      }
      ws3->mutableY(0) = 3.;
    }
  }

  void test_creator() {
    Mantid::Kernel::PropertyManager manager;
    manager.declareProperty(
        std::make_unique<WorkspaceProperty<Workspace>>("WS1", "", Direction::Input));
    manager.declareProperty(
        std::make_unique<WorkspaceProperty<Workspace>>("WS2", "", Direction::Input));
    manager.declareProperty(
        std::make_unique<WorkspaceProperty<Workspace>>("WS3", "", Direction::Input));

    std::vector<std::string> propNames{"WS1", "WS2", "WS3"};
    MultiDomainCreator multi(&manager, propNames);

    TS_ASSERT_EQUALS(multi.getNCreators(), 3);
    TS_ASSERT(!multi.hasCreator(0));
    TS_ASSERT(!multi.hasCreator(1));
    TS_ASSERT(!multi.hasCreator(2));

    manager.setProperty("WS1", ws1);
    manager.setProperty("WS2", ws2);
    manager.setProperty("WS3", ws3);

    FitMW *creator = new FitMW(&manager, "WS1");
    creator->declareDatasetProperties("1");
    multi.setCreator(0, creator);
    creator = new FitMW(&manager, "WS2");
    creator->declareDatasetProperties("2");
    multi.setCreator(1, creator);
    creator = new FitMW(&manager, "WS3");
    creator->declareDatasetProperties("3");
    multi.setCreator(2, creator);

    TS_ASSERT(multi.hasCreator(0));
    TS_ASSERT(multi.hasCreator(1));
    TS_ASSERT(multi.hasCreator(2));

    manager.setProperty("WorkspaceIndex1", 0);
    manager.setProperty("WorkspaceIndex2", 0);
    manager.setProperty("WorkspaceIndex3", 0);

    FunctionDomain_sptr domain;
    FunctionValues_sptr values;

    multi.createDomain(domain, values);

    TS_ASSERT(domain);
    TS_ASSERT(values);

    auto jointDomain = boost::dynamic_pointer_cast<JointDomain>(domain);
    TS_ASSERT(jointDomain);
    TS_ASSERT_EQUALS(jointDomain->getNParts(), 3);

    auto d1 =
        dynamic_cast<const FunctionDomain1D *>(&jointDomain->getDomain(0));
    auto d2 =
        dynamic_cast<const FunctionDomain1D *>(&jointDomain->getDomain(1));
    auto d3 =
        dynamic_cast<const FunctionDomain1D *>(&jointDomain->getDomain(2));

    TS_ASSERT(d1);
    TS_ASSERT(d2);
    TS_ASSERT(d3);

    TS_ASSERT_EQUALS(d1->size(), 10);
    TS_ASSERT_EQUALS(d2->size(), 10);
    TS_ASSERT_EQUALS(d3->size(), 10);

    TS_ASSERT_EQUALS(values->size(), 30);
  }

  void test_output_workspace() {
    MultiDomainCreatorTest_Manager manager;
    manager.declareProperty(
        std::make_unique<WorkspaceProperty<Workspace>>("WS1", "", Direction::Input));
    manager.declareProperty(
        std::make_unique<WorkspaceProperty<Workspace>>("WS2", "", Direction::Input));
    manager.declareProperty(
        std::make_unique<WorkspaceProperty<Workspace>>("WS3", "", Direction::Input));

    std::vector<std::string> propNames{"WS1", "WS2", "WS3"};
    MultiDomainCreator multi(&manager, propNames);

    manager.setProperty("WS1", ws1);
    manager.setProperty("WS2", ws2);
    manager.setProperty("WS3", ws3);

    FitMW *creator = new FitMW(&manager, "WS1");
    creator->declareDatasetProperties("1");
    multi.setCreator(0, creator);
    creator = new FitMW(&manager, "WS2");
    creator->declareDatasetProperties("2");
    multi.setCreator(1, creator);
    creator = new FitMW(&manager, "WS3");
    creator->declareDatasetProperties("3");
    multi.setCreator(2, creator);

    manager.setProperty("WorkspaceIndex1", 0);
    manager.setProperty("WorkspaceIndex2", 0);
    manager.setProperty("WorkspaceIndex3", 0);

    FunctionDomain_sptr domain;
    FunctionValues_sptr values;

    auto mdfun = boost::make_shared<MultiDomainFunction>();

    auto f1 = boost::make_shared<UserFunction>();
    f1->setAttributeValue("Formula", "1.1 + 0*x");
    mdfun->addFunction(f1);
    mdfun->setDomainIndex(0, 0);

    auto f2 = boost::make_shared<UserFunction>();
    f2->setAttributeValue("Formula", "2.1 + 0*x");
    mdfun->addFunction(f2);
    mdfun->setDomainIndex(1, 1);

    auto f3 = boost::make_shared<UserFunction>();
    f3->setAttributeValue("Formula", "3.1 + 0*x");
    mdfun->addFunction(f3);
    mdfun->setDomainIndex(2, 2);

    auto ws = multi.createOutputWorkspace("out_", mdfun, FunctionDomain_sptr(),
                                          FunctionValues_sptr(), "OUT_WS");
    TS_ASSERT(ws);

    auto group = boost::dynamic_pointer_cast<WorkspaceGroup>(ws);
    TS_ASSERT(group);

    TS_ASSERT_EQUALS(group->size(), 3);

    auto ows1 = boost::dynamic_pointer_cast<MatrixWorkspace>(group->getItem(0));
    doTestOutputSpectrum(ows1, 0);
    auto ows2 = boost::dynamic_pointer_cast<MatrixWorkspace>(group->getItem(1));
    doTestOutputSpectrum(ows2, 1);
    auto ows3 = boost::dynamic_pointer_cast<MatrixWorkspace>(group->getItem(2));
    doTestOutputSpectrum(ows3, 2);

    WorkspaceGroup_sptr outWS = manager.getProperty("OUT_WS");
    TS_ASSERT(outWS);
    TS_ASSERT_EQUALS(outWS->getItem(0)->getName(), "out_Workspace_0");
    TS_ASSERT_EQUALS(outWS->getItem(1)->getName(), "out_Workspace_1");
    TS_ASSERT_EQUALS(outWS->getItem(2)->getName(), "out_Workspace_2");
    manager.store("OUT_WS");
    TS_ASSERT_EQUALS(outWS->getName(), "out_Workspaces");
    AnalysisDataService::Instance().clear();
  }

  void test_setMatrixWorkspace_and_setWorkspace() {
    Mantid::Kernel::PropertyManager manager;
    manager.declareProperty(
        std::make_unique<WorkspaceProperty<Workspace>>("WS1", "", Direction::Input));
    manager.declareProperty(
        std::make_unique<WorkspaceProperty<Workspace>>("WS2", "", Direction::Input));
    manager.declareProperty(
        std::make_unique<WorkspaceProperty<Workspace>>("WS3", "", Direction::Input));

    std::vector<std::string> propNames{"WS1", "WS2", "WS3"};

    MultiDomainCreator multi(&manager, propNames);

    manager.setProperty("WS1", ws1);
    manager.setProperty("WS2", ws2);
    manager.setProperty("WS3", ws3);

    FitMW *creator = new FitMW(&manager, "WS1");
    creator->declareDatasetProperties("1");
    multi.setCreator(0, creator);
    creator = new FitMW(&manager, "WS2");
    creator->declareDatasetProperties("2");
    multi.setCreator(1, creator);
    creator = new FitMW(&manager, "WS3");
    creator->declareDatasetProperties("3");
    multi.setCreator(2, creator);

    manager.setProperty("WorkspaceIndex1", 0);
    manager.setProperty("WorkspaceIndex2", 1);
    manager.setProperty("WorkspaceIndex3", 2);

    FunctionDomain_sptr domain;
    FunctionValues_sptr values;

    auto mdfun = boost::make_shared<MultiDomainFunction>();

    auto f1 = boost::make_shared<MultiDomainCreatorTest_Fun>();
    mdfun->addFunction(f1);
    mdfun->setDomainIndex(0, 0);

    auto f2 = boost::make_shared<MultiDomainCreatorTest_Fun>();
    mdfun->addFunction(f2);
    mdfun->setDomainIndex(1, 1);

    auto f3 = boost::make_shared<MultiDomainCreatorTest_Fun>();
    mdfun->addFunction(f3);
    mdfun->setDomainIndex(2, 2);

    multi.initFunction(mdfun);
    TS_ASSERT_EQUALS(f1->m_wsIndex, 0);
    TS_ASSERT_EQUALS(f2->m_wsIndex, 1);
    TS_ASSERT_EQUALS(f3->m_wsIndex, 2);

    TS_ASSERT_EQUALS(f1->m_workspace, ws1);
    TS_ASSERT_EQUALS(f2->m_workspace, ws2);
    TS_ASSERT_EQUALS(f3->m_workspace, ws3);
  }

private:
  void doTestOutputSpectrum(MatrixWorkspace_sptr ws, size_t index) {
    TS_ASSERT(ws);
    TS_ASSERT_EQUALS(ws->getNumberHistograms(), 3);
    auto &data = ws->readY(0);
    auto &calc = ws->readY(1);
    auto &diff = ws->readY(2);

    for (size_t i = 0; i < data.size(); ++i) {
      TS_ASSERT_EQUALS(data[i], static_cast<double>(index + 1));
      TS_ASSERT_EQUALS(data[i] - calc[i], diff[i]);
      TS_ASSERT_DELTA(-0.1, diff[i], 1e-10);
    }
  }

  MatrixWorkspace_sptr ws1, ws2, ws3;
};

#endif /*MULTIDOMAINCREATORTEST_H_*/
