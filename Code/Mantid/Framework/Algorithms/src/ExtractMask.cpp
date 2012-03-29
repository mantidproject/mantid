/*WIKI*

The masking from the InputWorkspace property is extracted by creating a new MatrixWorkspace with a single X bin where:
* 0 = masked;
* 1 = unmasked.

The spectra containing 0 are also marked as masked and the instrument link is preserved so that the instrument view functions correctly.


*WIKI*/
//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include "MantidAlgorithms/ExtractMask.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "MantidDataObjects/SpecialWorkspace2D.h"
#include "MantidKernel/MultiThreaded.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/NullValidator.h"

namespace Mantid
{
  namespace Algorithms
  {

    // Register the algorithm into the AlgorithmFactory
    DECLARE_ALGORITHM(ExtractMask)
    
    /// Sets documentation strings for this algorithm
    void ExtractMask::initDocs()
    {
      this->setWikiSummary("Extracts the masking from a given workspace and places it in a new workspace. ");
      this->setOptionalMessage("Extracts the masking from a given workspace and places it in a new workspace.");
    }
    
    using DataObjects::SpecialWorkspace2D;
    using DataObjects::SpecialWorkspace2D_const_sptr;
    using DataObjects::SpecialWorkspace2D_sptr;
    using Kernel::Direction;
    using Geometry::IDetector_const_sptr;
    using namespace API;
    using namespace Kernel;

    //--------------------------------------------------------------------------
    // Private methods
    //--------------------------------------------------------------------------

    /**
     * Declare the algorithm properties
     */
    void ExtractMask::init()
    {
      declareProperty(
          new WorkspaceProperty<MatrixWorkspace>("InputWorkspace","", Direction::Input),
          "A workspace whose masking is to be extracted"
      );
      declareProperty(
          new WorkspaceProperty<MatrixWorkspace>("OutputWorkspace","", Direction::Output),
          "A workspace containing the masked spectra as zeroes and ones.");

      declareProperty(
          new ArrayProperty<detid_t>("DetectorList", boost::make_shared<NullValidator>(), Direction::Output),
          "A comma separated list or array containing a list of masked detector ID's" );
    }

    /**
     * Execute the algorithm
     */
    void ExtractMask::exec()
    {
      MatrixWorkspace_const_sptr inputWS = getProperty("InputWorkspace");

      const int nHist = static_cast<int>(inputWS->getNumberHistograms());

      // Create a new workspace for the results, copy from the input to ensure that we copy over the instrument and current masking
      DataObjects::SpecialWorkspace2D* maskWS = new DataObjects::SpecialWorkspace2D();
      maskWS->initialize(nHist, 1, 1);
      MatrixWorkspace_sptr outputWS(maskWS);
      WorkspaceFactory::Instance().initializeFromParent(inputWS, outputWS, false);
      outputWS->setTitle(inputWS->getTitle());

      bool inputWSIsSpecial(false);
      {
        SpecialWorkspace2D_const_sptr temp = boost::dynamic_pointer_cast<const SpecialWorkspace2D>(inputWS);
        if (temp)
          inputWSIsSpecial = true;
        g_log.notice() << "Input workspace is special\n";
      }

      Progress prog(this,0.0,1.0,nHist);
      MantidVecPtr xValues;
      xValues.access() = MantidVec(1, 0.0);

      // List masked of detector IDs
      std::vector<detid_t> detectorList;

      PARALLEL_FOR2(inputWS, outputWS)
      for( int i = 0; i < nHist; ++i )
      {
        PARALLEL_START_INTERUPT_REGION
        // Spectrum in the output workspace
        ISpectrum * outSpec = outputWS->getSpectrum(i);
        // Spectrum in the input workspace
        const ISpectrum * inSpec = inputWS->getSpectrum(i);

        // Copy X, spectrum number and detector IDs
        outSpec->copyInfoFrom(*inSpec);

        bool inputIsMasked(false);

        IDetector_const_sptr inputDet;
        try
        {
          inputDet = inputWS->getDetector(i);
          if (inputWSIsSpecial) {
            inputIsMasked = (inSpec->dataY()[0] > 0.5);
          }
          // special workspaces can mysteriously have the mask bit set
          // but only check if we haven't already decided to mask the spectrum
          if( !inputIsMasked && inputDet->isMasked() )
          {
            inputIsMasked = true;
          }

          if (inputIsMasked)
          {
            detid_t id = inputDet->getID();
            PARALLEL_CRITICAL(name)
            {
              detectorList.push_back(id);
            }
          }
        }
        catch(Kernel::Exception::NotFoundError &)
        {
          inputIsMasked = false;
        }

        if( inputIsMasked )
        {
          outSpec->dataY()[0] = 1.0;
          outSpec->dataE()[0] = 1.0;
        }
        else
        {
          outSpec->dataY()[0] = 0.0;
          outSpec->dataE()[0] = 0.0;
        }
        prog.report();

        PARALLEL_END_INTERUPT_REGION
      }
      PARALLEL_CHECK_INTERUPT_REGION


      // Clear all the "masked" bits on the output masked workspace
      Geometry::ParameterMap & pmap = outputWS->instrumentParameters();
      pmap.clearParametersByName("masked");

      g_log.information() << detectorList.size() << " spectra are masked\n";
      setProperty("OutputWorkspace", outputWS);
      setProperty("DetectorList", detectorList);
    }
    
  }
}

