/*WIKI* 

This algorithm transposes a workspace, so that an N1 x N2 workspace becomes N2 x N1.

The X-vector-values for the new workspace are taken from the axis value of the old workspace, which is generaly the spectra number but can be other values, if say the workspace has gone through ConvertSpectrumAxis.

The new axis values are taken from the previous X-vector-values for the first specrum in the workspace. For this reason, use with ragged workspaces is undefined.

*WIKI*/
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/Transpose.h"
#include "MantidAPI/NumericAxis.h"
#include "MantidAPI/WorkspaceValidators.h"

namespace Mantid
{
  namespace Algorithms
  {

    // Register the algorithm into the AlgorithmFactory
    DECLARE_ALGORITHM(Transpose)

    /// Sets documentation strings for this algorithm
    void Transpose::initDocs()
    {
      this->setWikiSummary("Transposes a workspace, so that an N1 x N2 workspace becomes N2 x N1.");
      this->setOptionalMessage("Transposes a workspace, so that an N1 x N2 workspace becomes N2 x N1.");
    }


    using namespace Kernel;
    using namespace API;

    void Transpose::init()
    {
      declareProperty(new WorkspaceProperty<>("InputWorkspace","",Direction::Input, false, new CommonBinsValidator<>()),"The input workspace.");
      declareProperty(new WorkspaceProperty<>("OutputWorkspace","",Direction::Output), "The output workspace.");
    }

    void Transpose::exec()
    {
      MatrixWorkspace_const_sptr inputWorkspace = getProperty("InputWorkspace");
      MatrixWorkspace_sptr outputWorkspace = createOutputWorkspace(inputWorkspace);

      size_t newNhist = outputWorkspace->getNumberHistograms();
      size_t newXsize = outputWorkspace->readX(0).size();
      size_t newYsize = outputWorkspace->blocksize();

      // Create a shareable X vector for the output workspace
      Axis *inputYAxis = getVerticalAxis(inputWorkspace);
      MantidVecPtr newXVector;
      newXVector.access() = MantidVec(newXsize);
      MantidVec & newXValues = newXVector.access();
      for(size_t i = 0; i < newXsize; ++i)
      {
        newXValues[i] = (*inputYAxis)(i);
      }

      Progress progress(this,0.0,1.0, newNhist*newYsize);
      PARALLEL_FOR2(inputWorkspace, outputWorkspace)
      for( int64_t i = 0; i < static_cast<int64_t>(newNhist); ++i )
      {
        PARALLEL_START_INTERUPT_REGION

        outputWorkspace->setX(i, newXVector);

        MantidVec & outY = outputWorkspace->dataY(i);
        MantidVec & outE = outputWorkspace->dataE(i);

        for(int64_t j = 0; j < int64_t(newYsize); ++j)
        {
          progress.report("Swapping data values");
          outY[j] = inputWorkspace->readY(j)[i];
          outE[j] = inputWorkspace->readE(j)[i];
        }

        PARALLEL_END_INTERUPT_REGION
      }
      PARALLEL_CHECK_INTERUPT_REGION
    }

    /**
     * Creates the output workspace for this algorithm
     * @param :: inputWorkspace A parent workspace to initialize from
     * @return A pointer to the output workspace
     */
    API::MatrixWorkspace_sptr Transpose::createOutputWorkspace(API::MatrixWorkspace_const_sptr inputWorkspace)
    {
      Mantid::API::Axis* yAxis = getVerticalAxis(inputWorkspace);
      const size_t oldNhist = inputWorkspace->getNumberHistograms();
      const MantidVec & inX = inputWorkspace->readX(0);
      const size_t oldXlength = inX.size();
      const size_t oldYlength = inputWorkspace->blocksize();
      const size_t oldVerticalAxislength = yAxis->length();

      // The input Y axis may be binned so the new X data should be too
      size_t newNhist(oldYlength), newXsize(oldVerticalAxislength), newYsize(oldNhist);
      MatrixWorkspace_sptr outputWorkspace = WorkspaceFactory::Instance().create(inputWorkspace,newNhist, newXsize, newYsize);

      // Create a new numeric axis for Y the same length as the old X array
      // Values come from input X
      Mantid::API::NumericAxis *newAxis = new NumericAxis(oldXlength);
      newAxis->unit() = inputWorkspace->getAxis(0)->unit();
      outputWorkspace->getAxis(0)->unit() = inputWorkspace->getAxis(1)->unit();
      for(size_t i = 0; i < oldXlength; ++i)
      {
        newAxis->setValue(i, inX[i]);
      }
      outputWorkspace->replaceAxis(1, newAxis);
      setProperty("OutputWorkspace", outputWorkspace);
      return outputWorkspace;
    }

    /**
     * Returns the input vertical
     * @param workspace :: A pointer to a workspace
     * @return An axis pointer for the vertical axis of the input workspace
     */
    API::Axis *Transpose::getVerticalAxis(API::MatrixWorkspace_const_sptr workspace) const
    {
      API::Axis *yAxis;
      try
      {
        yAxis = workspace->getAxis(1);
      }
      catch ( Kernel::Exception::IndexError& )
      {
        throw std::invalid_argument("Axis(1) not found on input workspace.");
      }
      // Can't put text in dataX
      if( yAxis->isText() )
      {
        throw std::invalid_argument("Axis(1) is a text axis. Transpose is unable to cope with text axes.");
      }
      return yAxis;
    }


  } // namespace Algorithms
} // namespace Mantid

