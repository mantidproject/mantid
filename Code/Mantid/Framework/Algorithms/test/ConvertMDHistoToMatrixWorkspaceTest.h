/*
 * ConvertMDHistoToMatrixWorkspaceTest.h
 *
 *  Created on: Feb 13, 2014
 *      Author: spu92482
 */

#ifndef CONVERTMDHISTOTOMATRIXWORKSPACETEST_H_
#define CONVERTMDHISTOTOMATRIXWORKSPACETEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/FrameworkManager.h"
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

  MatrixWorkspace_sptr do_execute_on_1D_directly(const size_t n_dims, const double signal, const double error_sq, size_t* nbins, coord_t* min, coord_t* max)
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

  /*
   * Create a 1D slice from a 2D cut. That way there is non-identity rotation available for coordinate transformation.
   */
  MatrixWorkspace_sptr do_execute_on_1D_indirect()
  {
    auto in_ws = MDEventsTestHelper::makeMDEW<2>(2, -10.0, 10, 3);

    // Create a line slice at 45 degrees to the original workspace.
    IAlgorithm_sptr binMDAlg = AlgorithmManager::Instance().create("BinMD");
    binMDAlg->setRethrows(true);
    binMDAlg->initialize();
    binMDAlg->setChild(true);
    binMDAlg->setProperty("InputWorkspace", in_ws);
    binMDAlg->setProperty("AxisAligned", false);
    binMDAlg->setPropertyValue("BasisVector0", "X,units,0.7071,0.7071"); // cos 45 to in_ws x-axis (consistent with a 45 degree anti-clockwise rotation)
    binMDAlg->setPropertyValue("BasisVector1", "Y,units,-0.7071,0.7071"); // cos 45 to in_ws y-axis (consistent with a 45 degree anti-clockwise rotation)
    binMDAlg->setPropertyValue("Translation", "-10,-10");
    binMDAlg->setPropertyValue("OutputExtents", "0,28.284,-1,1"); // x goes from 0 to sqrt((-10-10)^2 + (-10-10)^2) and -1 to 1 in new system, but -10 to 10 in old coordinate axes for both x and y.
    binMDAlg->setPropertyValue("OutputBins", "10,1");
    binMDAlg->setPropertyValue("OutputWorkspace", "_"); // Not really required for child algorithm
    binMDAlg->execute();
    Workspace_sptr temp = binMDAlg->getProperty("OutputWorkspace");
    auto slice = boost::dynamic_pointer_cast<IMDWorkspace>(temp);

    ConvertMDHistoToMatrixWorkspace convert_alg;
    convert_alg.setRethrows(true);
    convert_alg.setChild(true);
    convert_alg.initialize();
    convert_alg.setProperty("InputWorkspace", slice);
    convert_alg.setPropertyValue("OutputWorkspace", "_"); // Not really required for child algorithm
    convert_alg.execute();
    MatrixWorkspace_sptr out_ws = convert_alg.getProperty("OutputWorkspace");
    return out_ws;
  }

public:

  ConvertMDHistoToMatrixWorkspaceTest()
  {
    FrameworkManager::Instance();
  }

  void test_input_workspace_must_be_imdhisto()
  {
    MatrixWorkspace_sptr ws = WorkspaceCreationHelper::Create1DWorkspaceConstant(1,1,0);
    ConvertMDHistoToMatrixWorkspace alg;
    alg.setRethrows(true);
    alg.initialize();
    TS_ASSERT_THROWS(alg.setProperty("InputWorkspace", ws), std::invalid_argument&) ;
  }

  /*
   * Test the conversion where there is no coordinate transformation to apply (no original workspace). So the coordinates are directly
   * translated from the MDHistoWorkspace to the output MDWorkspace.
   */
  void test_direct_conversion()
  {
    const size_t n_dims = 1;
    const double signal = 1;
    const double error_sq = 0;
    size_t nbins[1] = {1};
    coord_t min[1] = {-1};
    coord_t max[1] = {1};

    MatrixWorkspace_sptr out_ws = do_execute_on_1D_directly(n_dims, signal, error_sq, nbins, min, max);
    TS_ASSERT_EQUALS(out_ws->getNumberHistograms(), 1);
    auto first_x_spectra = out_ws->readX(0);
    TS_ASSERT_EQUALS(first_x_spectra.front(), -1);
    TS_ASSERT_EQUALS(first_x_spectra.back(), 1);
  }

  /*
   * Test the conversion where there IS a coordinate transformation to apply. The original coordinates are transformed via the coordinate
   * transformation on the original workspace.
   */
  void test_indirect_conversion()
  {
    MatrixWorkspace_sptr out_ws = do_execute_on_1D_indirect();

    TS_ASSERT_EQUALS(out_ws->getNumberHistograms(), 1);
    auto first_x_spectra = out_ws->readX(0);

    TSM_ASSERT_DELTA("First coordinate in the incorrect position. Incorrect transformation.", first_x_spectra.front(), -10, 1e-3);
    TSM_ASSERT_DELTA( "Last coordinate in the incorrect position. Incorrect transformation.", first_x_spectra.back(), 10, 1e-3);

  }

};


#endif /* CONVERTMDHISTOTOMATRIXWORKSPACETEST_H_ */
