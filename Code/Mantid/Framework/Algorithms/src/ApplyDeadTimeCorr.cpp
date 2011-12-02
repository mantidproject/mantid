/*WIKI* 

Apply deadtime correction to each spectra of a workspace.

*WIKI*/

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/ApplyDeadTimeCorr.h"
#include "MantidKernel/System.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidKernel/PropertyWithValue.h"
#include "boost/lexical_cast.hpp"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidAPI/TableRow.h"

#include <iostream>
#include <cmath>


using std::string;
using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid
{
namespace Algorithms
{

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(ApplyDeadTimeCorr)

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void ApplyDeadTimeCorr::init()
  {    
    declareProperty(new API::WorkspaceProperty<API::MatrixWorkspace>("InputWorkspace", "",
      Direction::Input), "The name of the input workspace containing measured counts");

    declareProperty(new API::WorkspaceProperty<API::ITableWorkspace>("DeadTimeTable", "",
      Direction::Input), "Name of the Dead Time Table");
    
    declareProperty(new API::WorkspaceProperty<API::MatrixWorkspace>("OutputWorkspace","",Direction::Output),
      "The name of the output workspace containing corrected counts");  
  }

  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void ApplyDeadTimeCorr::exec()
  {
    // Get pointers to the workspace and dead time table
    API::MatrixWorkspace_sptr inputWs = getProperty("InputWorkspace");
    API::ITableWorkspace_sptr deadTimeTable = getProperty("DeadTimeTable");

    if (deadTimeTable->rowCount() == static_cast<int>(inputWs->getNumberHistograms()))
    {
      // Duplicate the input workspace. Only need to change Y values based on dead time corrections
      IAlgorithm_sptr duplicate = createSubAlgorithm("CloneWorkspace");
      duplicate->initialize();
      duplicate->setProperty<Workspace_sptr>("InputWorkspace", boost::dynamic_pointer_cast<Workspace>(inputWs));
      duplicate->execute();
      Workspace_sptr temp = duplicate->getProperty("OutputWorkspace");
      MatrixWorkspace_sptr outputWs = boost::dynamic_pointer_cast<MatrixWorkspace>(temp);

      // Presumed to be the same for all data
      double timeBinWidth(inputWs->dataX(0)[1] - inputWs->dataX(0)[0]); 

      if (timeBinWidth != 0)
      {
        // Apply Dead Time
        for(size_t i=0; i<inputWs->getNumberHistograms(); ++i)
        {
          API::TableRow deadTimeRow = deadTimeTable->getRow(static_cast<int>(i));
          for(size_t j=0; j<inputWs->blocksize(); ++j)
          {
            double temp(1 - inputWs->dataY(i)[j] * (deadTimeRow.Double(1)/timeBinWidth));
            if (temp != 0)
            {
              outputWs->dataY(i)[j] = inputWs->dataY(i)[j] / temp;
            }
            else
            {
              g_log.error() << "1 - MeasuredCount * (Deadtime/TimeBin width is currently (" << temp << "). Can't divide by this amount." << std::endl;

              throw std::invalid_argument("Can't divide by 0");
            }
          }
        }

        setProperty("OutputWorkspace", outputWs);

      }
      else
      {
        g_log.error() << "The time bin width is currently (" << timeBinWidth << "). Can't divide by this amount." << std::endl;

        throw std::invalid_argument("Can't divide by 0");
      }
    }
    else
    {
      g_log.error() << "Row count(" << deadTimeTable->rowCount() << ") of Dead time table and Number of Histograms(" 
        << inputWs->getNumberHistograms() << ") did not match." << std::endl;

      throw std::invalid_argument("Row count and Number of Histograms did not match");
    }
  }



} // namespace Mantid
} // namespace Algorithms