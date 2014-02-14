/*
 * ConvertMDHistoToMatrixWorkspaceTest.h
 *
 *  Created on: Feb 13, 2014
 *      Author: spu92482
 */

#ifndef CONVERTMDHISTOTOMATRIXWORKSPACETEST_H_
#define CONVERTMDHISTOTOMATRIXWORKSPACETEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidAlgorithms/ConvertMDHistoToMatrixWorkspace.h"
#include "MantidMDEvents/MDHistoWorkspace.h"
#include "MantidTestHelpers/MDEventsTestHelper.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::Algorithms;
using namespace Mantid::MDEvents;

class ConvertMDHistoToMatrixWorkspaceTest : public CxxTest::TestSuite
{
public:

  MatrixWorkspace_sptr do_execute(const size_t n_dims, const double signal, const double error_sq, size_t* nbins, coord_t* min, coord_t* max)
  {
    IMDHistoWorkspace_sptr inWS = MDEventsTestHelper::makeFakeMDHistoWorkspaceGeneral(n_dims, signal,
        error_sq, nbins, min, max);

    ConvertMDHistoToMatrixWorkspace alg;
    alg.setChild(true);
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", inWS);
    alg.setPropertyValue("OutputWorkspace", "_");

    alg.execute();
    MatrixWorkspace_sptr outWS = alg.getProperty("OutputWorkspace");
    return outWS;
  }

  void test_input_workspace_must_be_imdhisto()
  {
    MatrixWorkspace_sptr ws = WorkspaceCreationHelper::Create1DWorkspaceConstant(1,1,0);
    ConvertMDHistoToMatrixWorkspace alg;
    alg.setRethrows(true);
    alg.initialize();
    TS_ASSERT_THROWS(alg.setProperty("InputWorkspace", ws), std::invalid_argument&) ;
  }

  void test_conversion()
  {
    const size_t n_dims = 1;
    const double signal = 1;
    const double error_sq = 0;
    size_t nbins[1] = {1};
    coord_t min[1] = {-1};
    coord_t max[1] = {1};

    MatrixWorkspace_sptr outWS = do_execute(n_dims, signal, error_sq, nbins, min, max);
  }

};


#endif /* CONVERTMDHISTOTOMATRIXWORKSPACETEST_H_ */
