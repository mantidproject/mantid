#ifndef MANTID_CUSTOMINTERFACES_ALCPEAKFITTINGMODELTEST_H_
#define MANTID_CUSTOMINTERFACES_ALCPEAKFITTINGMODELTEST_H_

#include <cxxtest/TestSuite.h>

#include <boost/assign.hpp>

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/FrameworkManager.h"

#include "MantidQtCustomInterfaces/Muon/ALCPeakFittingModel.h"

#include <QtTest/QSignalSpy>

using namespace Mantid::API;
using namespace MantidQt::CustomInterfaces;

class ALCPeakFittingModelTest : public CxxTest::TestSuite
{
  ALCPeakFittingModel* m_model;

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ALCPeakFittingModelTest *createSuite() { return new ALCPeakFittingModelTest(); }
  static void destroySuite( ALCPeakFittingModelTest *suite ) { delete suite; }

  ALCPeakFittingModelTest()
  {
    FrameworkManager::Instance(); // To make sure everything is initialized
  }

  void setUp()
  {
    m_model = new ALCPeakFittingModel();
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
  }

};


#endif /* MANTID_CUSTOMINTERFACES_ALCPEAKFITTINGMODELTEST_H_ */
