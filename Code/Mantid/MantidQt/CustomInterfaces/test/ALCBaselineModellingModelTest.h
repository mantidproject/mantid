#ifndef MANTID_CUSTOMINTERFACES_ALCBASELINEMODELLINGMODELTEST_H_
#define MANTID_CUSTOMINTERFACES_ALCBASELINEMODELLINGMODELTEST_H_

#include <cxxtest/TestSuite.h>

#include <boost/assign.hpp>

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/FrameworkManager.h"

#include "MantidQtCustomInterfaces/Muon/ALCBaselineModellingModel.h"

#include <QtTest/QSignalSpy>

using namespace Mantid::API;
using namespace MantidQt::CustomInterfaces;

class ALCBaselineModellingModelTest : public CxxTest::TestSuite
{
  ALCBaselineModellingModel* m_model;

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ALCBaselineModellingModelTest *createSuite() { return new ALCBaselineModellingModelTest(); }
  static void destroySuite( ALCBaselineModellingModelTest *suite ) { delete suite; }

  ALCBaselineModellingModelTest()
  {
    FrameworkManager::Instance(); // To make sure everything is initialized
  }

  void setUp()
  {
    m_model = new ALCBaselineModellingModel();
  }

  void tearDown()
  {
    delete m_model;
  }

  void test_setData()
  {
    MatrixWorkspace_sptr data = WorkspaceFactory::Instance().create("Workspace2D", 1, 1, 1);

    QSignalSpy spy(m_model, SIGNAL(dataChanged()));

    TS_ASSERT_THROWS_NOTHING(m_model->setData(data));

    TS_ASSERT_EQUALS(spy.size(), 1);
    TS_ASSERT_EQUALS(m_model->data(), data);
  }

  void test_fit()
  {
    std::vector<double> y = boost::assign::list_of(100)(1)(2)(100)(100)(3)(4)(5)(100);
    std::vector<double> x = boost::assign::list_of(1)(2)(3)(4)(5)(6)(7)(8)(9);

    MatrixWorkspace_sptr data = WorkspaceFactory::Instance().create("Workspace2D", 1, y.size(), y.size());
    data->dataY(0) = y;
    data->dataX(0) = x;

    m_model->setData(data);

    IFunction_const_sptr func = FunctionFactory::Instance().createInitialized("name=FlatBackground,A0=0");

    std::vector<IALCBaselineModellingModel::Section> sections;
    sections.push_back(std::make_pair(2,3));
    sections.push_back(std::make_pair(6,8));

    // TODO: test that the appropriate signals are thrown
    TS_ASSERT_THROWS_NOTHING(m_model->fit(func, sections));

    IFunction_const_sptr fittedFunc = m_model->fittedFunction();
    TS_ASSERT(fittedFunc);

    if (fittedFunc)
    {
      TS_ASSERT_EQUALS(fittedFunc->name(), "FlatBackground");
      TS_ASSERT_DELTA(fittedFunc->getParameter("A0"), 3, 1E-8);
      TS_ASSERT_DELTA(fittedFunc->getError(0),0.447214,1E-6);
    }

    MatrixWorkspace_const_sptr corrected = m_model->correctedData();
    TS_ASSERT(corrected);

    if (corrected)
    {
      TS_ASSERT_EQUALS(corrected->getNumberHistograms(), 1);
      TS_ASSERT_EQUALS(corrected->blocksize(), 9);

      TS_ASSERT_DELTA(corrected->readY(0)[0], 97, 1E-8);
      TS_ASSERT_DELTA(corrected->readY(0)[2], -1, 1E-8);
      TS_ASSERT_DELTA(corrected->readY(0)[5], 0.0, 1E-8);
      TS_ASSERT_DELTA(corrected->readY(0)[8], 97, 1E-8);
    }

    TS_ASSERT_EQUALS(m_model->sections(), sections);
  }

  void test_exportWorkspace()
  {
    // TODO: implement
  }

  void test_exportTable()
  {
    // TODO: implement
  }

  void test_exportModel()
  {
    // TODO: implement
  }

};


#endif /* MANTID_CUSTOMINTERFACES_ALCBASELINEMODELLINGMODELTEST_H_ */
