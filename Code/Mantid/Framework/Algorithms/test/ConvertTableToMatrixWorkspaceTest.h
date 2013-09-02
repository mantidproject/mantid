#ifndef CONVERTTABLETOMATRIXWORKSPACETEST_H_
#define CONVERTTABLETOMATRIXWORKSPACETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/ConvertTableToMatrixWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/TableRow.h"
#include "MantidKernel/Unit.h"

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::Kernel;

class ConvertTableToMatrixWorkspaceTest : public CxxTest::TestSuite
{
public:
  void testName()
  {
    TS_ASSERT_EQUALS( m_converter->name(), "ConvertTableToMatrixWorkspace" )
  }

  void testVersion()
  {
    TS_ASSERT_EQUALS( m_converter->version(), 1 )
  }

  void testInit()
  {
    TS_ASSERT_THROWS_NOTHING( m_converter->initialize() )
    TS_ASSERT( m_converter->isInitialized() )
  }


  void testExec()
  {
    
    ITableWorkspace_sptr tws = WorkspaceFactory::Instance().createTable();
    tws->addColumn("int","A");
    tws->addColumn("double","B");
    tws->addColumn("double","C");

    size_t n = 10;
    for (size_t i = 0; i < n; ++i)
    {
      TableRow row = tws->appendRow();
      int x = int(i);
      double y = x * 1.1;
      double e = sqrt(y);
      row << x << y << e;
    }


    TS_ASSERT_THROWS_NOTHING( m_converter->setProperty("InputWorkspace",tws) );
    TS_ASSERT_THROWS_NOTHING( m_converter->setPropertyValue("OutputWorkspace","out") );
    TS_ASSERT_THROWS_NOTHING( m_converter->setPropertyValue("ColumnX","A") );
    TS_ASSERT_THROWS_NOTHING( m_converter->setPropertyValue("ColumnY","B") );
    TS_ASSERT_THROWS_NOTHING( m_converter->setPropertyValue("ColumnE","C") );

    TS_ASSERT( m_converter->execute() );

    MatrixWorkspace_sptr mws = boost::dynamic_pointer_cast<MatrixWorkspace>(
    API::AnalysisDataService::Instance().retrieve("out"));

    TS_ASSERT( mws );
    TS_ASSERT_EQUALS( mws->getNumberHistograms() , 1);
    TS_ASSERT( !mws->isHistogramData() );
    TS_ASSERT_EQUALS( mws->blocksize(), tws->rowCount() );

    const Mantid::MantidVec& X = mws->readX(0);
    const Mantid::MantidVec& Y = mws->readY(0);
    const Mantid::MantidVec& E = mws->readE(0);

    for(size_t i = 0; i < tws->rowCount(); ++i)
    {
      TableRow row = tws->getRow(i);
      int x;
      double y,e;
      row >> x >> y >> e;
      TS_ASSERT_EQUALS( double(x), X[i] );
      TS_ASSERT_EQUALS( y, Y[i] );
      TS_ASSERT_EQUALS( e, E[i] );
    }

    boost::shared_ptr<Units::Label> label = boost::dynamic_pointer_cast<Units::Label>(mws->getAxis(0)->unit());
    TS_ASSERT(label);
    TS_ASSERT_EQUALS(label->caption(), "A");
    TS_ASSERT_EQUALS(mws->YUnitLabel(), "B");

    API::AnalysisDataService::Instance().remove("out");
  }

  void test_Default_ColumnE()
  {
    size_t n = 10;
    for (size_t i = 0; i < n; ++i)
    {
      TableRow row = tws->appendRow();
      double x = double(i);
      double y = x * 1.1;
      row << x << y;
    }

    TS_ASSERT( m_converter->execute() );

    MatrixWorkspace_sptr mws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("out");

    TS_ASSERT( mws );
    TS_ASSERT_EQUALS( mws->getNumberHistograms() , 1);
    TS_ASSERT( !mws->isHistogramData() );
    TS_ASSERT_EQUALS( mws->blocksize(), tws->rowCount() );

    const Mantid::MantidVec& X = mws->readX(0);
    const Mantid::MantidVec& Y = mws->readY(0);
    const Mantid::MantidVec& E = mws->readE(0);

    for(size_t i = 0; i < tws->rowCount(); ++i)
    {
      TableRow row = tws->getRow(i);
      double x,y;
      row >> x >> y;
      TS_ASSERT_EQUALS( double(x), X[i] );
      TS_ASSERT_EQUALS( y, Y[i] );
      TS_ASSERT_EQUALS( 0.0, E[i] );
    }

    API::AnalysisDataService::Instance().remove("out");
  }

  void test_fail_on_empty_table()
  {
    TS_ASSERT_THROWS( m_converter->execute(), std::runtime_error );
  }

  void setUp()
  {
    tws = WorkspaceFactory::Instance().createTable();
    tws->addColumn("double","A");
    tws->addColumn("double","B");

    m_converter = boost::make_shared<Mantid::Algorithms::ConvertTableToMatrixWorkspace>();
    m_converter->setRethrows(true);
    m_converter->initialize();
    TS_ASSERT_THROWS_NOTHING( m_converter->setProperty("InputWorkspace",tws) );
    TS_ASSERT_THROWS_NOTHING( m_converter->setPropertyValue("OutputWorkspace","out") );
    TS_ASSERT_THROWS_NOTHING( m_converter->setPropertyValue("ColumnX","A") );
    TS_ASSERT_THROWS_NOTHING( m_converter->setPropertyValue("ColumnY","B") );
  }

private:
    IAlgorithm_sptr m_converter;
    ITableWorkspace_sptr tws;
};

#endif /*CONVERTTABLETOMATRIXWORKSPACETEST_H_*/
