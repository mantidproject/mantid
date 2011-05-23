//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/SolidAngle.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidAPI/AlgorithmFactory.h"
#include "MantidKernel/UnitFactory.h"
#include <cfloat>
#include <iostream>

namespace Mantid
{
  namespace Algorithms
  {

    // Register with the algorithm factory
    DECLARE_ALGORITHM(SolidAngle)
    
    /// Sets documentation strings for this algorithm
    void SolidAngle::initDocs()
    {
      this->setWikiSummary("The SolidAngle algorithm calculates the solid angle in steradians for each of the detectors in an instrument and outputs the data in a workspace.  This can then be used to normalize a data workspace using the divide algorithm should you wish. ");
      this->setOptionalMessage("The SolidAngle algorithm calculates the solid angle in steradians for each of the detectors in an instrument and outputs the data in a workspace.  This can then be used to normalize a data workspace using the divide algorithm should you wish.");
    }
    

    using namespace Kernel;
    using namespace API;
    using Geometry::IInstrument_const_sptr;

    /// Default constructor
    SolidAngle::SolidAngle() : Algorithm()
    {
    }

    /// Destructor
    SolidAngle::~SolidAngle()
    {
    }

    /// Initialisation method
    void SolidAngle::init()
    {
      declareProperty(
        new WorkspaceProperty<API::MatrixWorkspace>("InputWorkspace","",Direction::Input,new InstrumentValidator<>),
        "This workspace is used to identify the instrument to use and also which\n"
        "spectra to create a solid angle for. If the Max and Min spectra values are\n"
        "not provided one solid angle will be created for each spectra in the input\n"
        "workspace" );
      declareProperty(
        new WorkspaceProperty<API::MatrixWorkspace>("OutputWorkspace","",Direction::Output),
        "The name of the workspace to be created as the output of the algorithm" );

      BoundedValidator<int> *mustBePositive = new BoundedValidator<int>();
      mustBePositive->setLower(0);
      declareProperty("StartWorkspaceIndex",0, mustBePositive,
        "The index number of the first spectrum for which to find the solid angle\n"
        "(default: 0)" );
      // As the property takes ownership of the validator pointer, have to take care to pass in a unique
      // pointer to each property.
      declareProperty("EndWorkspaceIndex",EMPTY_INT(), mustBePositive->clone(),
        "The index of the last spectrum whose solid angle is to be found (default: the\n"
        "last spectrum in the workspace)");
    }

    /** Executes the algorithm
    */
    void SolidAngle::exec()
    {
      // Get the workspaces
      API::MatrixWorkspace_const_sptr inputWS = getProperty("InputWorkspace");	
      int m_MinSpec = getProperty("StartWorkspaceIndex");
      int m_MaxSpec = getProperty("EndWorkspaceIndex");

      const int numberOfSpectra = static_cast<int>(inputWS->getNumberHistograms());

      // Check 'StartSpectrum' is in range 0-numberOfSpectra
      if ( m_MinSpec > numberOfSpectra )
      {
        g_log.warning("StartWorkspaceIndex out of range! Set to 0.");
        m_MinSpec = 0;
      }
      if ( isEmpty(m_MaxSpec) ) m_MaxSpec = static_cast<int>(numberOfSpectra-1);
      if ( m_MaxSpec > numberOfSpectra-1 || m_MaxSpec < m_MinSpec )
      {
        g_log.warning("EndWorkspaceIndex out of range! Set to max detector number");
        m_MaxSpec = static_cast<int>(numberOfSpectra-1);
      }

      API::MatrixWorkspace_sptr outputWS = WorkspaceFactory::Instance().create(inputWS,m_MaxSpec-m_MinSpec+1,2,1);
      // The result of this will be a distribution
      outputWS->isDistribution(true);
      outputWS->setYUnit("");
      outputWS->setYUnitLabel("Steradian");
      setProperty("OutputWorkspace",outputWS);

      // Get a pointer to the instrument contained in the workspace
      IInstrument_const_sptr instrument = inputWS->getInstrument();

      // Get the distance between the source and the sample (assume in metres)
      Geometry::IObjComponent_const_sptr sample = instrument->getSample();
      if ( !sample )
      {
        g_log.information(
          "There doesn't appear to be any sample location information in the workspace");
        throw std::logic_error(
          "Sample location not found, aborting algorithm SoildAngle");
      }
      Geometry::V3D samplePos = sample->getPos();
      g_log.debug() << "Sample position is " << samplePos << std::endl;

      int loopIterations = m_MaxSpec-m_MinSpec;
      int failCount=0;
      Progress prog(this,0.0,1.0,numberOfSpectra);

      // Loop over the histograms (detector spectra)
      PARALLEL_FOR2(outputWS,inputWS)
      for (int j = 0; j <= loopIterations; ++j) 
      {
        PARALLEL_START_INTERUPT_REGION
        int i = j + m_MinSpec;
        try {
          // Get the spectrum number for this histogram
          outputWS->getAxis(1)->spectraNo(j) = inputWS->getAxis(1)->spectraNo(i);
          // Now get the detector to which this relates
          Geometry::IDetector_const_sptr det = inputWS->getDetector(i);
          // Solid angle should be zero if detector is masked ('dead')
          double solidAngle = det->isMasked() ? 0.0 : det->solidAngle(samplePos);

          outputWS->dataX(j)[0] = inputWS->readX(i).front();
          outputWS->dataX(j)[1] = inputWS->readX(i).back();
          outputWS->dataY(j)[0] = solidAngle;
          outputWS->dataE(j)[0] = 0;
        } 
        catch (Exception::NotFoundError e)
        {
          // Get to here if exception thrown when calculating distance to detector
          failCount++;
          outputWS->dataX(j).assign(outputWS->dataX(j).size(),0.0);
          outputWS->dataY(j).assign(outputWS->dataY(j).size(),0.0);
          outputWS->dataE(j).assign(outputWS->dataE(j).size(),0.0);
        }

        prog.report();
        PARALLEL_END_INTERUPT_REGION
      } // loop over spectra
      PARALLEL_CHECK_INTERUPT_REGION

      if (failCount != 0)
      {
        g_log.information() << "Unable to calculate solid angle for " << failCount << " spectra. Zeroing spectrum." << std::endl;
      }

    }

  } // namespace Algorithm
} // namespace Mantid
