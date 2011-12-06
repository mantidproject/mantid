#ifndef FIND_SX_UB_USING_LATTICE_PARAMETERS_TEST_H
#define FIND_SX_UB_USING_LATTICE_PARAMETERS_TEST_H

#include <cxxtest/TestSuite.h>
#include "MantidAPI/ITableWorkspace.h";
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/TableRow.h"
//#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidCrystal/FindSXUBUsingLatticeParameters.h"
//#include "MantidAPI/IPeak.h"
#include <cmath>

using namespace Mantid::API;
using namespace Mantid::Crystal;



//=====================================================================================
// Functional tests
//=====================================================================================
class FindSXUBUsingLatticeParameterTest : public CxxTest::TestSuite
{

private:

public:

  void testX()
  {
    try
    {
      ITableWorkspace_sptr ws = Mantid::API::WorkspaceFactory::Instance().createTable();
      ws->addColumn("double","Qx");
      ws->addColumn("double","Qy");
      ws->addColumn("double","Qz");

      ws->setRowCount(10); //Ten Rows for each peak.

      TableRow row = ws->getRow(0);
      int rowIndex = 0;
     // row << 0.219654<<0.0108024<<1.11382;
      row = ws->getRow(rowIndex++);
      row << 0.486429<<-0.100521<<0.322103;
      row = ws->getRow(rowIndex++);
      row << 0.635444<<-0.103101<<0.624199;
      row = ws->getRow(rowIndex++);
      row << 0.330549<<-0.0835759<<1.3411;
      //row = ws->getRow(rowIndex++);
      //row << 0.148992<<0.00117305<<0.302677;
      //row = ws->getRow(rowIndex++);
      //row << 0.450068<<-0.00456059<<0.24572;
      row = ws->getRow(rowIndex++);
      row << 0.180382<<-0.0824794<<1.04059;

      FindSXUBUsingLatticeParameters alg;
      alg.setRethrows(true);
      alg.initialize();
      alg.setProperty("PeaksTable", ws);
      alg.setPropertyValue("UnitCell", "10.02, 11.852, 3.38, 90, 90, 90");
      alg.setProperty("PeakIndices", "1, 2, 3, 4, 5, 6, 7");
      alg.setProperty("dTolerance", 0.01);
      alg.execute();
      TS_ASSERT(alg.isExecuted());
    }
    catch(std::exception& ex)
    {
      std::string msg = ex.what();
    }
  }
};

#endif
