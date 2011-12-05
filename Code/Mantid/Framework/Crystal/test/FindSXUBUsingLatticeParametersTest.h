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
     row << M_PI * 2 / 3.186 << 0.0 << 0.0; // 1,1,1
     row = ws->getRow(rowIndex++);
     row << M_PI * 2 /1.933 << 0.0 << 0.0; // 2,2,0
     row = ws->getRow(rowIndex++);
     row << M_PI * 2 /1.669 << 0.0 << 0.0; // 3,1,1
     row = ws->getRow(rowIndex++);
     row << M_PI * 2 /1.361 << 0.0 << 0.0; // 4,0,0
     row = ws->getRow(rowIndex++);
     row << M_PI * 2 /1.238 << 0.0 << 0.0; // 3,3,1
     row = ws->getRow(rowIndex++);
     row << M_PI * 2 /1.110 << 0.0 << 0.0; // 4,2,2
     row = ws->getRow(rowIndex++);
     row << M_PI * 2 /1.046 << 0.0 << 0.0; // 3,3,3
     row = ws->getRow(rowIndex++);
     row << M_PI * 2 /0.935 << 0.0 << 0.0; // 4,4,0


     FindSXUBUsingLatticeParameters alg;
     alg.setRethrows(true);
     alg.initialize();
     alg.setProperty("PeaksTable", ws);
     alg.setPropertyValue("UnitCell", "5.43, 5.43, 5.43, 90, 90, 90");
     alg.setProperty("PeakIndices", "3, 4, 5, 6, 7");
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
