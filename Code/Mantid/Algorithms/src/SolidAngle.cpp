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
		}

		/** Executes the algorithm
		*/
		void SolidAngle::exec()
		{
			// Get the workspaces
			API::MatrixWorkspace_const_sptr inputWS = getProperty("InputWorkspace");

			API::MatrixWorkspace_sptr outputWS = WorkspaceFactory::Instance().create(inputWS,inputWS->getNumberHistograms(),2,1);
			setProperty("OutputWorkspace",outputWS);



			// Get a pointer to the instrument contained in the workspace
			IInstrument_const_sptr instrument = outputWS->getInstrument();
			// And one to the SpectraDetectorMap
			SpectraMap_const_sptr specMap = outputWS->getSpectraMap();

			outputWS->setYUnit("Steradian");

			// Get the distance between the source and the sample (assume in metres)
			Geometry::IObjComponent_const_sptr sample = instrument->getSample();
			Geometry::V3D samplePos = sample->getPos();

			const int notFailed = -99;
			int failedDetectorIndex = notFailed;

			const int numberOfSpectra = outputWS->getNumberHistograms();

			int iprogress_step = numberOfSpectra / 100;
			if (iprogress_step == 0) iprogress_step = 1;
			// Loop over the histograms (detector spectra)
			for (int i = 0; i < numberOfSpectra; ++i) {
				try {
					// Get the spectrum number for this histogram
					const int spec = inputWS->getAxis(1)->spectraNo(i);
					outputWS->getAxis(1)->spectraNo(i) = spec;
					// Now get the detector to which this relates
					Geometry::IDetector_const_sptr det = specMap->getDetector(spec);
					double solidAngle = det->solidAngle(samplePos);
					
	
					outputWS->dataX(i)[0] = inputWS->readX(i)[0];
					outputWS->dataX(i)[1] = inputWS->readX(i)[inputWS->readX(i).size()-1];
					outputWS->dataY(i)[0] = solidAngle;
					outputWS->dataE(i)[0] = 0;
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
					outputWS->dataX(i).assign(outputWS->dataX(i).size(),0.0);
					outputWS->dataY(i).assign(outputWS->dataY(i).size(),0.0);
					outputWS->dataE(i).assign(outputWS->dataE(i).size(),0.0);
				}

				if ( i % 100 == 0)
				{
					progress( double(i)/numberOfSpectra );
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
