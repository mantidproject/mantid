
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/LoadRawSpectrum0.h"
#include "LoadRaw/isisraw2.h"
#include "MantidDataHandling/LoadLog.h"
#include "MantidAPI/FileProperty.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidGeometry/Instrument/XMLlogfile.h"
#include "MantidAPI/MemoryManager.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include <boost/shared_ptr.hpp>
#include <Poco/Path.h>
#include <cmath>
#include <cstdio> //Required for gcc 4.4

namespace Mantid
{
	namespace DataHandling
	{
		// Register the algorithm into the algorithm factory
		DECLARE_ALGORITHM(LoadRawSpectrum0)

/// Sets documentation strings for this algorithm
void LoadRawSpectrum0::initDocs()
{
  this->setWikiSummary("Loads spectrum zero  from  ISIS [[RAW_File | raw]] file and stores it in a 2D [[workspace]] ([[Workspace2D]] class). ");
  this->setOptionalMessage("Loads spectrum zero  from  ISIS  raw file and stores it in a 2D workspace (Workspace2D class).");
}


		using namespace Kernel;
		using namespace API;

		/// Constructor
		LoadRawSpectrum0::LoadRawSpectrum0() :
		 m_filename(), m_numberOfSpectra(0), m_numberOfPeriods(0),
			m_specTimeRegimes(), m_prog(0.0)
		{
		}

		LoadRawSpectrum0::~LoadRawSpectrum0()
		{
		}

		/// Initialisation method.
		void LoadRawSpectrum0::init()
		{
							LoadRawHelper::init();
			
		}

		/** Executes the algorithm. Reading in the file and creating and populating
		*  the output workspace
		*
		*  @throw Exception::FileError If the RAW file cannot be found/opened
		*  @throw std::invalid_argument If the optional properties are set to invalid values
		*/
		void LoadRawSpectrum0::exec()
		{
			// Retrieve the filename from the properties
			m_filename = getPropertyValue("Filename");
			
			bool bLoadlogFiles = getProperty("LoadLogFiles");

			//open the raw file
            FILE* file=openRawFile(m_filename);

			// Need to check that the file is not a text file as the ISISRAW routines don't deal with these very well, i.e 
			// reading continues until a bad_alloc is encountered.
			if( isAscii(file) )
			{
				g_log.error() << "File \"" << m_filename << "\" is not a valid RAW file.\n";
				throw std::invalid_argument("Incorrect file type encountered.");
			}

			std::string title;
			readTitle(file,title);
  
            readworkspaceParameters(m_numberOfSpectra,m_numberOfPeriods,m_lengthIn,m_noTimeRegimes);

			// Calculate the size of a workspace, given its number of periods & spectra to read
			const int64_t total_specs = 1;

			// Get the time channel array(s) and store in a vector inside a shared pointer
			std::vector<boost::shared_ptr<MantidVec> > timeChannelsVec =
				getTimeChannels(m_noTimeRegimes, m_lengthIn);

			// Need to extract the user-defined output workspace name
			const std::string localWSName = getPropertyValue("OutputWorkspace");

			double histTotal = static_cast<double>(total_specs * m_numberOfPeriods);
			int64_t histCurrent = -1;

			// Create the 2D workspace for the output
			DataObjects::Workspace2D_sptr localWorkspace =createWorkspace(total_specs, m_lengthIn,m_lengthIn-1,title);
					
			Run& run = localWorkspace->mutableRun();

			if(bLoadlogFiles)
			{
				runLoadLog(m_filename,localWorkspace);
				Property* log = createPeriodLog(1);
				if (log) run.addLogData(log);
			}
			// Set the total proton charge for this run
			setProtonCharge(run);
			
			WorkspaceGroup_sptr wsgrp_sptr = createGroupWorkspace();
			setWorkspaceProperty("OutputWorkspace", title, wsgrp_sptr, localWorkspace,m_numberOfPeriods, false);
			
			// Loop over the number of periods in the raw file, putting each period in a separate workspace
			for (int64_t period = 0; period < m_numberOfPeriods; ++period)
			{
				if (period > 0)
				{
					localWorkspace = boost::dynamic_pointer_cast<DataObjects::Workspace2D>(
						WorkspaceFactory::Instance().create(localWorkspace));

					if(bLoadlogFiles)
					{
						//remove previous period data
						std::stringstream prevPeriod;
						prevPeriod << "PERIOD " << (period);
						Run& runObj = localWorkspace->mutableRun();
						runObj.removeLogData(prevPeriod.str());
						//add current period data
						Property* log = createPeriodLog(static_cast<int>(period+1));
						if (log) 
						{ 
						  runObj.addLogData(log);
						}
					}
					//skip all spectra except the first one in each period
					for(int i=1;i<=m_numberOfSpectra;++i)
					{
						skipData(file, i+ (period-1)*(m_numberOfSpectra+1) );
					}

				}
				int64_t wsIndex = 0;
				// for each period read first spectrum
				int64_t histToRead = period*(m_numberOfSpectra+1);

				progress(m_prog, "Reading raw file data...");
				//isisRaw->readData(file, histToRead);
				//readData(file, histToRead);
				//read spectrum 
				if (!readData(file, histToRead))
				{
					throw std::runtime_error("Error reading raw file");
				}

				//set the workspace data
			     setWorkspaceData(localWorkspace, timeChannelsVec, wsIndex, 0, m_noTimeRegimes,m_lengthIn,1);

				if (m_numberOfPeriods == 1)
				{
					if (++histCurrent % 100 == 0)
					{
						m_prog = double(histCurrent) / histTotal;
					}
					interruption_point();
				}
				if(m_numberOfPeriods>1)
				{
					setWorkspaceProperty(localWorkspace, wsgrp_sptr, period, false);
					// progress for workspace groups 
					m_prog = static_cast<double>(period) / static_cast<double>(m_numberOfPeriods - 1);
				}

			} // loop over periods

			// Clean up
			reset();
			fclose(file);

		}
		
	
	}
}
