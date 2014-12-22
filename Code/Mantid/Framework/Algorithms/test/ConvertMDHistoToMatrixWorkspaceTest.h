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

#include <boost/lexical_cast.hpp>

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


  /**
    * Test convesion of a MD workspace to a 2D MatrixWorkspace.
    * 
    * @param ndims :: Number of dimensions in the input MDHistoWorkspace. ndims >= 2.
    * @param nonIntegr :: Indices of the non-integrated dimensions. There must be 2 of them to pass the test.
    */
  void do_test_2D_slice(size_t ndims, std::vector<size_t> nonIntegr)
  {
    // prepare input workspace

    // create an MD histo workspace
    size_t size = 1;
    // property values for CreateMDHistoWorkspace
    std::vector<double> extents(ndims*2);
    std::vector<int> numberOfBins(ndims);
    std::vector<std::string> names(ndims);
    std::vector<std::string> units(ndims);
    // property values for SliceMDHisto
    std::vector<int> start(ndims);
    std::vector<int> end(ndims);
    for(size_t i = 0; i < ndims; ++i)
    {
      size_t nbins = 3 + i;
      size *= nbins;
      numberOfBins[i] = static_cast<int>(nbins);
      extents[2*i] = 0.0;
      extents[2*i+1] = static_cast<double>(nbins);
      names[i] = "x_" + boost::lexical_cast<std::string>(i);
      if ( nonIntegr.end() != std::find( nonIntegr.begin(), nonIntegr.end(), i) )
      {
        // if it's a non-integrated dimension - don't slice
        end[i] = static_cast<int>(nbins);
      }
      else
      {
        end[i] = 1;
      }
    }
    std::vector<signal_t> data(size);
    std::vector<signal_t> error(size);

    auto alg = AlgorithmManager::Instance().create("CreateMDHistoWorkspace");
    alg->initialize();
    alg->setRethrows(true);
    alg->setChild(true);
    alg->setProperty("SignalInput", data);
    alg->setProperty("ErrorInput", error);
    alg->setProperty("Dimensionality", static_cast<int>(ndims));
    alg->setProperty("Extents", extents);
    alg->setProperty("NumberOfBins", numberOfBins);
    alg->setProperty("Names", names);
    alg->setProperty("Units", units);
    alg->setPropertyValue("OutputWorkspace", "_"); // Not really required for child algorithm

    try
    {
      alg->execute();
    }
    catch(std::exception& e)
    {
      TS_FAIL(e.what());
    }

    // slice the md ws to make it acceptable by ConvertMDHistoToMatrixWorkspace
    IMDHistoWorkspace_sptr ws = alg->getProperty("OutputWorkspace");
    TS_ASSERT( ws );

    alg = AlgorithmManager::Instance().create("SliceMDHisto");
    alg->initialize();
    alg->setRethrows(true);
    alg->setChild(true);
    alg->setProperty("InputWorkspace", ws);
    alg->setProperty("Start", start);
    alg->setProperty("End", end);
    alg->setPropertyValue("OutputWorkspace", "_1"); // Not really required for child algorithm

    try
    {
      alg->execute();
    }
    catch(std::exception& e)
    {
      TS_FAIL(e.what());
    }

    IMDHistoWorkspace_sptr slice = alg->getProperty("OutputWorkspace");
    TS_ASSERT( slice );

    // test ConvertMDHistoToMatrixWorkspace

    alg = AlgorithmManager::Instance().create("ConvertMDHistoToMatrixWorkspace");
    alg->initialize();
    alg->setRethrows(true);
    alg->setChild(true);
    alg->setProperty("InputWorkspace", slice);
    alg->setPropertyValue("OutputWorkspace", "_2"); // Not really required for child algorithm

    if ( nonIntegr.size() > 2 || nonIntegr.empty() )
    {
      TS_ASSERT_THROWS( alg->execute(), std::invalid_argument );
    }
    else
    {
      try
      {
        alg->execute();
      }
      catch(std::exception& e)
      {
        TS_FAIL(e.what());
      }
    

      MatrixWorkspace_sptr matrix = alg->getProperty("OutputWorkspace");
      TS_ASSERT( matrix );

      if ( nonIntegr.size() == 1 )
      {
        TS_ASSERT_EQUALS( matrix->getNumberHistograms(), 1 );
      }

      if ( nonIntegr.size() >= 1 )
      {
        auto xDim = slice->getDimension(nonIntegr[0]);
        TS_ASSERT_EQUALS( xDim->getNBins(), matrix->blocksize() );
        for(size_t i = 0; i < matrix->getNumberHistograms(); ++i)
        {
          TS_ASSERT_EQUALS( matrix->readX(i).front(), xDim->getMinimum() );
          TS_ASSERT_EQUALS( matrix->readX(i).back(), xDim->getMaximum() );
        }
      }
      else if ( nonIntegr.size() == 2 )
      {
        auto yDim = slice->getDimension(nonIntegr[1]);
        TS_ASSERT_EQUALS( yDim->getNBins(), matrix->getNumberHistograms() );
        auto axis = matrix->getAxis(1);
        TS_ASSERT_EQUALS( axis->getMin(), yDim->getMinimum() );
        TS_ASSERT_EQUALS( axis->getMax(), yDim->getMaximum() );
      }

    }

    AnalysisDataService::Instance().clear();
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
    size_t nbins[1] = {2};
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

    TS_ASSERT_EQUALS(out_ws->getNumberHistograms(), 1);
    auto first_x_spectra = out_ws->readX(0);

    TSM_ASSERT_DELTA("First coordinate in the incorrect position. Incorrect transformation.", first_x_spectra.front(), -10, 1e-3);
    TSM_ASSERT_DELTA( "Last coordinate in the incorrect position. Incorrect transformation.", first_x_spectra.back(), 10, 1e-3);
  }

  /*
  What we are attempting to show here is that the dimension that gets selected as the x-axis for the
  output matrix workspace, should be the one that has the greatest delta (start - end point) in the original coordinates.

  In this case, that corresponds the the X-axis of the input workspace, so we should see the extents of the output matrix workspace
  corresponding to -10 to 10, because that is how the cut was made.


  Here's a schematic of the input MD workspace
                y
                |       x (10,5) 
                |     .
                |   .
          (0,0) | .
  ----------------------------- x 
              . |
            .   |
          .     |
 (-10,-5)x      |

  */
  void test_indirect_conversion_axis_selection_where_zeroth_dim_of_original_is_used()
  {
    auto in_ws = MDEventsTestHelper::makeMDEW<2>(2, -10.0, 10, 3);

    // Create a line slice at degrees to the original workspace.
    IAlgorithm_sptr binMDAlg = AlgorithmManager::Instance().create("BinMD");
    binMDAlg->setRethrows(true);
    binMDAlg->initialize();
    binMDAlg->setChild(true);
    binMDAlg->setProperty("InputWorkspace", in_ws);
    binMDAlg->setProperty("AxisAligned", false);
    binMDAlg->setPropertyValue("BasisVector0", "X,units,0.6666, 0.3333"); // Line set up to intersects 0,0 and 10,5 in original coords. Basis0 vector is therefore 10/(10+5) and 5/(10+5)
    binMDAlg->setPropertyValue("BasisVector1", "Y,units,-0.3333, 0.6666"); // Line set up to intersects 0,0 and -10,5 in original coords. Basis1 vector is therefore 5/(10+5) and 10/(10+5) 
    binMDAlg->setPropertyValue("Translation", "-10,-5");
    binMDAlg->setPropertyValue("OutputExtents", "0,22.36,-1,1"); // x goes from 0 to sqrt((-10-10)^2 + (-5-5)^2) and -1 to 1 in original coords
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
    // Should select the first dimension in this case.
    TSM_ASSERT_EQUALS("Wrong dimension auto selected for output x-axis", in_ws->getDimension(0)->getName(), out_ws->getDimension(0)->getName());
    TS_ASSERT_EQUALS(out_ws->getNumberHistograms(), 1);
    auto first_x_spectra = out_ws->readX(0);

    TSM_ASSERT_DELTA("First coordinate in the incorrect position. Incorrect transformation.", first_x_spectra.front(), -10, 1e-3);
    TSM_ASSERT_DELTA( "Last coordinate in the incorrect position. Incorrect transformation.", first_x_spectra.back(), 10, 1e-3);
  }

  /*
  What we are attempting to show here is that the dimension that gets selected as the x-axis for the
  output matrix workspace, should be the one that has the greatest delta (start - end point) in the original coordinates.

  In this case, that corresponds the the Y-axis of the input MD workspace, so we should see the extents of the output matrix workspace
  corresponding to -10 to 10, because that is how the cut was made.

  Here's a schematic of the input MD workspace
                y
                |       x (5,8) 
                |     .
                |   .
          (0,0) | .
  ----------------------------- x 
              . |
            .   |
          .     |
 (-5,-8)x      |

  */
  void test_indirect_conversion_axis_selection_where_y_dim_of_original_is_used()
  {
    auto in_ws = MDEventsTestHelper::makeMDEW<2>(2, -10.0, 10, 3);

    // Create a line slice at degrees to the original workspace.
    IAlgorithm_sptr binMDAlg = AlgorithmManager::Instance().create("BinMD");
    binMDAlg->setRethrows(true);
    binMDAlg->initialize();
    binMDAlg->setChild(true);
    binMDAlg->setProperty("InputWorkspace", in_ws);
    binMDAlg->setProperty("AxisAligned", false);
    binMDAlg->setPropertyValue("BasisVector0", "X,units,0.3846, 0.6154"); // Line set up to intersect 0,0 and 5,10 in original coords. Basis0 vector is therefore  and 5/(5+8), 8/(5+8)
    binMDAlg->setPropertyValue("BasisVector1", "Y,units,-0.6154, 0.3846"); // Line set up to intersect 0,0 and -5,10 in original coords. Basis1 vector is therefore -8/(5+8) and 5/(5+8) 
    binMDAlg->setPropertyValue("Translation", "-5,-8");
    binMDAlg->setPropertyValue("OutputExtents", "0,18.867,-1,1"); // x goes from 0 to sqrt((-8-8)^2 + (-5-5)^2) and -1 to 1 in original coords
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
    // Should select the 2nd dimension for the x-axis in this case.
    TSM_ASSERT_EQUALS("Wrong dimension auto selected for output x-axis", in_ws->getDimension(1)->getName(), out_ws->getDimension(0)->getName());
    TS_ASSERT_EQUALS(out_ws->getNumberHistograms(), 1);
    auto first_x_spectra = out_ws->readX(0);
    TSM_ASSERT_DELTA("First coordinate in the incorrect position. Incorrect transformation.", first_x_spectra.front(), -8, 1e-3);
    TSM_ASSERT_DELTA("Last coordinate in the incorrect position. Incorrect transformation.", first_x_spectra.back(), 8, 1e-3);

    // Run it again, this time with FindXAxis set off.
    convert_alg.setProperty("FindXAxis", false);
    convert_alg.execute();
    out_ws = convert_alg.getProperty("OutputWorkspace");
    TSM_ASSERT_EQUALS("FindXAxis if off", "X", out_ws->getDimension(0)->getName());
    TS_ASSERT_EQUALS(out_ws->getNumberHistograms(), 1);
    first_x_spectra = out_ws->readX(0);
    TSM_ASSERT_DELTA("First coordinate in the incorrect position. Incorrect transformation.", first_x_spectra.front(), -5, 1e-3);
    TSM_ASSERT_DELTA("Last coordinate in the incorrect position. Incorrect transformation.", first_x_spectra.back(), 5, 1e-3);
  }


  void test_2D_slice_0()
  {
    // 4D sliced to 2D
    std::vector<size_t> nonIntegr(2);
    nonIntegr[0] = 0;
    nonIntegr[1] = 1;
    do_test_2D_slice(4,nonIntegr);
  }

  void test_2D_slice_1()
  {
    // 4D unsliced
    do_test_2D_slice(4,std::vector<size_t>());
  }

  void test_2D_slice_2()
  {
    // 4D sliced to 3D
    std::vector<size_t> nonIntegr(3);
    nonIntegr[0] = 0;
    nonIntegr[1] = 1;
    nonIntegr[2] = 2;
    do_test_2D_slice(4,nonIntegr);
  }

  void test_2D_slice_3()
  {
    // 4D sliced to 2D
    std::vector<size_t> nonIntegr(2);
    nonIntegr[0] = 0;
    nonIntegr[1] = 2;
    do_test_2D_slice(4,nonIntegr);
  }

  void test_2D_slice_4()
  {
    // 4D sliced to 2D
    std::vector<size_t> nonIntegr(2);
    nonIntegr[0] = 0;
    nonIntegr[1] = 3;
    do_test_2D_slice(4,nonIntegr);
  }

  void test_2D_slice_5()
  {
    // 4D sliced to 2D
    std::vector<size_t> nonIntegr(2);
    nonIntegr[0] = 1;
    nonIntegr[1] = 3;
    do_test_2D_slice(4,nonIntegr);
  }

  void test_2D_slice_6()
  {
    // 3D sliced to 2D
    std::vector<size_t> nonIntegr(2);
    nonIntegr[0] = 1;
    nonIntegr[1] = 2;
    do_test_2D_slice(3,nonIntegr);
  }

  void test_2D_slice_7()
  {
    // 4D sliced to 1D
    std::vector<size_t> nonIntegr(1,0);
    do_test_2D_slice(4,nonIntegr);
  }

  void test_2D_slice_8()
  {
    // 4D sliced to 1D
    std::vector<size_t> nonIntegr(1,1);
    do_test_2D_slice(4,nonIntegr);
  }

  void test_2D_slice_9()
  {
    // 4D sliced to 1D
    std::vector<size_t> nonIntegr(1,2);
    do_test_2D_slice(4,nonIntegr);
  }

  void test_2D_slice_10()
  {
    // 4D sliced to 1D
    std::vector<size_t> nonIntegr(1,3);
    do_test_2D_slice(4,nonIntegr);
  }

  void test_2D_slice_11()
  {
    // 2D unsliced
    do_test_2D_slice(2,std::vector<size_t>());
  }

  void test_2D_slice_12()
  {
    // 1D unsliced
    do_test_2D_slice(1,std::vector<size_t>());
  }

};


#endif /* CONVERTMDHISTOTOMATRIXWORKSPACETEST_H_ */
