//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/UnaryOperation.h"
#include "MantidAPI/WorkspaceProperty.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;

namespace Mantid
{
  namespace Algorithms
  {
    UnaryOperation::UnaryOperation() : API::Algorithm() {}
    
    UnaryOperation::~UnaryOperation() {}
    
    /** Initialisation method.
     *  Defines input and output workspace properties
     */
    void UnaryOperation::init()
    {
      declareProperty(new WorkspaceProperty<MatrixWorkspace>(inputPropName(),"",Direction::Input));
      declareProperty(new WorkspaceProperty<MatrixWorkspace>(outputPropName(),"",Direction::Output));
      
      // Call the virtual defineProperties functions to declare any properties defined in concrete algorithm
      defineProperties();
    }

    /// Executes the algorithm
    void UnaryOperation::exec()
    {
      // get the input workspaces
      MatrixWorkspace_const_sptr in_work = getProperty(inputPropName());

      MatrixWorkspace_sptr out_work = getProperty(outputPropName());
      // Only create output workspace if different to input one
      if ( out_work != in_work ) 
      {
        out_work = WorkspaceFactory::Instance().create(in_work);
        setProperty(outputPropName(),out_work);
      }

      // Now fetch any properties defined by concrete algorithm
      retrieveProperties();
      
      const int numSpec = in_work->getNumberHistograms();
      const int specSize = in_work->blocksize();
      const bool isHist = in_work->isHistogramData();

      // Initialise the progress reporting object
      Progress progress(this,0.0,1.0,numSpec);
      
      // Loop over every cell in the workspace, calling the abstract correction function
      PARALLEL_FOR2(in_work,out_work)
      for (int i = 0; i < numSpec; ++i)
      {
        PARALLEL_START_INTERUPT_REGION
        // Copy the X values over
        out_work->setX(i,in_work->refX(i));
        // Get references to the data
        // Output (non-const) ones first because they may copy the vector
        // if it's shared, which isn't thread-safe.
        MantidVec& YOut = out_work->dataY(i);
        MantidVec& EOut = out_work->dataE(i);
        const MantidVec& X = in_work->readX(i);
        const MantidVec& Y = in_work->readY(i);
        const MantidVec& E = in_work->readE(i);
        
        for (int j = 0; j < specSize; ++j)
        {
          // Use the bin centre for the X value if this is histogram data
          const double XIn = isHist ? (X[j]+X[j+1])/2.0 : X[j];
          // Call the abstract function, passing in the current values
          performUnaryOperation(XIn,Y[j],E[j],YOut[j],EOut[j]);
        }
        
        progress.report();
        PARALLEL_END_INTERUPT_REGION
      }
      PARALLEL_CHECK_INTERUPT_REGION
      return;
    }
       
  }
}
