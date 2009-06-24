//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/SolidAngle.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidAPI/AlgorithmFactory.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidDataObjects/Workspace2D.h"
#include <cfloat>
#include <iostream>

namespace Mantid
{
	namespace Algorithms
	{

		// Register with the algorithm factory
		DECLARE_ALGORITHM(SolidAngle)

		using namespace Kernel;
		using namespace API;
		using namespace DataObjects;

		// Get a reference to the logger
		Logger& SolidAngle::g_log = Logger::get("SolidAngle");

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
        new WorkspaceProperty<API::MatrixWorkspace>("InputWorkspace","",Direction::Input),
        "This workspace is used to identify the instrument to use and also which\n"
        "spectra to create a solid angle for. If the Max and Min spectra values are\n"
        "not provided one solid angle will be created for each spectra in the input\n"
        "workspace" );
			declareProperty(
        new WorkspaceProperty<API::MatrixWorkspace>("OutputWorkspace","",Direction::Output),
        "The name of the workspace to be created as the output of the algorithm" );

			BoundedValidator<int> *mustBePositive = new BoundedValidator<int>();
			mustBePositive->setLower(0);
			declareProperty("StartSpectrum",0, mustBePositive,
        "The index number of the first spectrum to use (default 0)" );
			// As the property takes ownership of the validator pointer, have to take care to pass in a unique
			// pointer to each property.
			declareProperty("EndSpectrum",0, mustBePositive->clone(),
        "The index number of the last spectrum to use (default 0)");
		}

		/** Executes the algorithm
		*/
		void SolidAngle::exec()
		{
			// Get the workspaces
			API::MatrixWorkspace_const_sptr inputWS = getProperty("InputWorkspace");	
			int m_MinSpec = getProperty("StartSpectrum");
			int m_MaxSpec = getProperty("EndSpectrum");

			const int numberOfSpectra = inputWS->getNumberHistograms();

			// Check 'StartSpectrum' is in range 0-numberOfSpectra
			if ( m_MinSpec > numberOfSpectra )
			{
				g_log.warning("StartSpectrum out of range! Set to 0.");
				m_MinSpec = 0;
			}
			if ( !m_MaxSpec ) m_MaxSpec = numberOfSpectra-1;
			if ( m_MaxSpec > numberOfSpectra-1 || m_MaxSpec < m_MinSpec )
			{
				g_log.warning("EndSpectrum out of range! Set to max detector number");
				m_MaxSpec = numberOfSpectra-1;
			}

			API::MatrixWorkspace_sptr outputWS = WorkspaceFactory::Instance().create(inputWS,m_MaxSpec-m_MinSpec+1,2,1);
			setProperty("OutputWorkspace",outputWS);

			// Get a pointer to the instrument contained in the workspace
			IInstrument_const_sptr instrument = inputWS->getInstrument();

			outputWS->setYUnit("Steradian");

			// Get the distance between the source and the sample (assume in metres)
			Geometry::IObjComponent_const_sptr sample = instrument->getSample();
      if ( !sample )
      {
        g_log.information(
          "There appears to be no instrument information in the workspace, ");
        g_log.information("aborting SoildAngle" );
        throw std::logic_error("Instrument information not found");
      }
			Geometry::V3D samplePos = sample->getPos();
      g_log.debug() << "Sample position is " << samplePos << std::endl;

			const int notFailed = -99;
			int failedDetectorIndex = notFailed;


			int iprogress_step = (m_MaxSpec-m_MinSpec+1) / 100;
			if (iprogress_step == 0) iprogress_step = 1;
			// Loop over the histograms (detector spectra)
			for (int i = m_MinSpec, j = 0; i <= m_MaxSpec; ++i,++j) {
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
					if (failedDetectorIndex != notFailed)
					{
						g_log.information() << "Unable to calculate solid angle[" << failedDetectorIndex << "-" << i-1 << "]. Zeroing spectrum." << std::endl;
						failedDetectorIndex = notFailed;
					}


				} catch (Exception::NotFoundError e) {
					// Get to here if exception thrown when calculating distance to detector
					if (failedDetectorIndex == notFailed)
					{
						failedDetectorIndex = i;
					}
					outputWS->dataX(j).assign(outputWS->dataX(j).size(),0.0);
					outputWS->dataY(j).assign(outputWS->dataY(j).size(),0.0);
					outputWS->dataE(j).assign(outputWS->dataE(j).size(),0.0);
				}

				if ( j % 100 == 0)
				{
					progress( double(j)/numberOfSpectra );
					interruption_point();
				}
			} // loop over spectra

			if (failedDetectorIndex != notFailed)
			{
				g_log.information() << "Unable to calculate solid angle[" << failedDetectorIndex << "-" << numberOfSpectra-1 << "]. Zeroing spectrum." << std::endl;
			}

		}


	} // namespace Algorithm
} // namespace Mantid
