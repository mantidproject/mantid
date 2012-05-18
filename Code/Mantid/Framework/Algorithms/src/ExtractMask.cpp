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
#include "MantidDataObjects/MaskWorkspace.h"
#include "MantidKernel/MultiThreaded.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/NullValidator.h"
#include "MantidAPI/ISpectrum.h"

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
          "A workspace containing the masked spectra as zeroes and ones."
          "If there is one detector per spectrum for all spectra, then the output is a MaskWorkspace.");

      declareProperty(
          new ArrayProperty<detid_t>("DetectorList", boost::make_shared<NullValidator>(), Direction::Output),
          "A comma separated list or array containing a list of masked detector ID's" );
    }

    /**
     * Execute the algorithm
     */
    void ExtractMask::exec()
    {
      // 1. Input
      MatrixWorkspace_const_sptr inputWS = getProperty("InputWorkspace");

      // b) Check whether there are any grouped workspaces
      const int nHist = static_cast<int>(inputWS->getNumberHistograms());
      bool groupeddet = false;
      for (size_t ih = 0; ih < size_t(nHist); ++ih)
      {
        const API::ISpectrum *spec = inputWS->getSpectrum(ih);
        if (!spec)
        {
          throw std::invalid_argument("Unable to access some spectrum.");
        }
        std::set<int> detids = spec->getDetectorIDs();
        if (detids.size() > 1)
        {
          groupeddet = true;
          break;
        }
      }

      // c) Input workspace is MaskWorkspace?
      bool inputWSIsSpecial(false);
      {
        DataObjects::MaskWorkspace_const_sptr temp = boost::dynamic_pointer_cast<const DataObjects::MaskWorkspace>(inputWS);
        if (temp)
          inputWSIsSpecial = true;
        g_log.notice() << "Input workspace is a MaskWorkspace.\n";
      }

      // 2. Create a new workspace for the results, copy from the input to ensure that we copy over the instrument and current masking
      MatrixWorkspace_sptr outputWS;

      if (!groupeddet)
      {
        DataObjects::MaskWorkspace* maskWS = new DataObjects::MaskWorkspace(inputWS->getInstrument(), true);
        maskWS->initialize(nHist, 1, 1);
        MatrixWorkspace_sptr tempws(maskWS);
        outputWS = tempws;
      }
      else
      {
        DataObjects::Workspace2D* twodWS = new DataObjects::Workspace2D();
        twodWS->initialize(nHist, 1, 1);
        MatrixWorkspace_sptr tempws(twodWS);
        outputWS = tempws;
      }

      WorkspaceFactory::Instance().initializeFromParent(inputWS, outputWS, false);
      outputWS->setTitle(inputWS->getTitle());

      // 3. Set values
      Progress prog(this,0.0,1.0,nHist);
      MantidVecPtr xValues;
      xValues.access() = MantidVec(1, 0.0);

      // List masked of detector IDs
      std::vector<detid_t> detectorList;
      std::vector<size_t> workspaceindexList;

      PARALLEL_FOR2(inputWS, outputWS)
      for( int i = 0; i < nHist; ++i )
      {
        PARALLEL_START_INTERUPT_REGION

        // Spectrum in the output workspace
        ISpectrum * outSpec = outputWS->getSpectrum(i);
        // Spectrum in the input workspace
        const ISpectrum * inSpec = inputWS->getSpectrum(i);
        // Detector IDs
        std::set<int> detids = inSpec->getDetectorIDs();

        // Copy X, spectrum number and detector IDs
        outSpec->copyInfoFrom(*inSpec);

        bool inputIsMasked(false);
        IDetector_const_sptr inputDet;

        // Set value and detector
        try
        {
          // a) Get effective detector & check masked or not
          inputDet = inputWS->getDetector(i);

          if (inputWSIsSpecial)
          {
            inputIsMasked = (inSpec->dataY()[0] > 0.5);
          }
          // special workspaces can mysteriously have the mask bit set
          // but only check if we haven't already decided to mask the spectrum
          if( !inputIsMasked && inputDet->isMasked() )
          {
            inputIsMasked = true;
          }

          // b) Mask output
          if (inputIsMasked)
          {
            // detid_t id = inputDet->getID();
            PARALLEL_CRITICAL(name)
            {
              for (std::set<int>::iterator detiter = detids.begin(); detiter != detids.end(); ++ detiter)
              {
                detid_t detid = detid_t(*detiter);
                detectorList.push_back(detid);
              }
            }
            workspaceindexList.push_back(i);
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
      /*
      Geometry::ParameterMap & pmap = outputWS->instrumentParameters();
      pmap.clearParametersByName("masked");
      */

      // 4. Mask all detectors listed
      /*
      Geometry::Instrument_const_sptr outinstr = outputWS->getInstrument();
      for (size_t id = 0; id < detectorList.size(); ++id)
      {
        Geometry::IDetector_sptr det = outinstr->getDetector(detectorList[id]);
      }
      */

      g_log.information() << detectorList.size() << " detectors are masked. " <<
          workspaceindexList.size() << " spectra are masked. " << std::endl;
      setProperty("OutputWorkspace", outputWS);
      setProperty("DetectorList", detectorList);
    }
    
  }
}

