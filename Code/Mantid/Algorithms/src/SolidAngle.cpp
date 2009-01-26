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
			declareProperty(new WorkspaceProperty<API::MatrixWorkspace>("InputWorkspace","",Direction::Input));
			declareProperty(new WorkspaceProperty<API::MatrixWorkspace>("OutputWorkspace","",Direction::Output));

			BoundedValidator<int> *mustBePositive = new BoundedValidator<int>();
			mustBePositive->setLower(0);
			declareProperty("StartSpectrum",0, mustBePositive);
			// As the property takes ownership of the validator pointer, have to take care to pass in a unique
			// pointer to each property.
			declareProperty("EndSpectrum",0, mustBePositive->clone());
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
				m_MaxSpec = numberOfSpectra;
			}

			API::MatrixWorkspace_sptr outputWS = WorkspaceFactory::Instance().create(inputWS,m_MaxSpec-m_MinSpec+1,2,1);
			setProperty("OutputWorkspace",outputWS);

			// Get a pointer to the instrument contained in the workspace
			IInstrument_const_sptr instrument = inputWS->getInstrument();
			// And one to the SpectraDetectorMap
			SpectraMap_const_sptr specMap = inputWS->getSpectraMap();

			outputWS->setYUnit("Steradian");

			// Get the distance between the source and the sample (assume in metres)
			Geometry::IObjComponent_const_sptr sample = instrument->getSample();
			Geometry::V3D samplePos = sample->getPos();

			const int notFailed = -99;
			int failedDetectorIndex = notFailed;


			int iprogress_step = (m_MaxSpec-m_MinSpec+1) / 100;
			if (iprogress_step == 0) iprogress_step = 1;
			// Loop over the histograms (detector spectra)
			for (int i = m_MinSpec, j = 0; i <= m_MaxSpec; ++i,++j) {
				try {
					// Get the spectrum number for this histogram
					const int spec = inputWS->getAxis(1)->spectraNo(i);
					outputWS->getAxis(1)->spectraNo(j) = spec;
					// Now get the detector to which this relates
					Geometry::IDetector_const_sptr det = specMap->getDetector(spec);
					double solidAngle = det->solidAngle(samplePos);
					
	
					outputWS->dataX(j)[0] = inputWS->readX(i)[0];
					outputWS->dataX(j)[1] = inputWS->readX(i)[inputWS->readX(i).size()-1];
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
