//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/LoadRawBin0.h"
#include "MantidDataHandling/ManagedRawFileWorkspace2D.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidAPI/XMLlogfile.h"
#include "MantidAPI/MemoryManager.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/FileProperty.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "LoadRaw/isisraw2.h"
#include "MantidDataHandling/LoadLog.h"

#include <boost/shared_ptr.hpp>
#include "Poco/Path.h"
#include <cmath>
#include <cstdio> //Required for gcc 4.4

namespace Mantid
{
namespace DataHandling
{
// Register the algorithm into the algorithm factory
DECLARE_ALGORITHM(LoadRawBin0)

using namespace Kernel;
using namespace API;

/// Constructor
LoadRawBin0::LoadRawBin0() :
   m_filename(), m_numberOfSpectra(0), m_numberOfPeriods(0),
   m_specTimeRegimes(), m_prog(0.0), m_bmspeclist(false)
{
}

LoadRawBin0::~LoadRawBin0()
{
}

/// Initialisation method.
void LoadRawBin0::init()
{
  LoadRawHelper::init();
  BoundedValidator<int> *mustBePositive = new BoundedValidator<int> ();
  mustBePositive->setLower(1);
  declareProperty("SpectrumMin", 1, mustBePositive,
      "The index number of the first spectrum to read.  Only used if\n"
      "spectrum_max is set.");
  declareProperty("SpectrumMax", unSetInt, mustBePositive->clone(),
      "The number of the last spectrum to read. Only used if explicitly\n"
      "set.");
  declareProperty(new ArrayProperty<int> ("SpectrumList"),
      "A comma-separated list of individual spectra to read.  Only used if\n"
      "explicitly set.");

 }

/** Executes the algorithm. Reading in the file and creating and populating
 *  the output workspace
 *
 *  @throw Exception::FileError If the RAW file cannot be found/opened
 *  @throw std::invalid_argument If the optional properties are set to invalid values
 */
void LoadRawBin0::exec()
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

	///
	setOptionalProperties();

	// to validate the optional parameters, if set
	checkOptionalProperties();

    // Calculate the size of a workspace, given its number of periods & spectra to read
     m_total_specs = calculateWorkspaceSize();

 //no real X values for bin 0,so initialize this to zero
  boost::shared_ptr<MantidVec> channelsVec(new MantidVec(1,0));
  m_timeChannelsVec.push_back(channelsVec);
 
  // Need to extract the user-defined output workspace name
  const std::string wsName = getPropertyValue("OutputWorkspace");

  int histTotal = m_total_specs * m_numberOfPeriods;
  int histCurrent = -1;
 
  // Create the 2D workspace for the output xlength and ylength is one 
  DataObjects::Workspace2D_sptr localWorkspace = createWorkspace(m_total_specs, 1,1,title);
  Sample& sample = localWorkspace->mutableSample();
  if (bLoadlogFiles)
  {
    runLoadLog(m_filename,localWorkspace);
    Property* log = createPeriodLog(1);
    if (log) sample.addLogData(log);
  }
  // Set the total proton charge for this run
  setProtonCharge(sample);

  WorkspaceGroup_sptr ws_grp = createGroupWorkspace();
  setWorkspaceProperty("OutputWorkspace", title, ws_grp, localWorkspace,m_numberOfPeriods, false);
    
  // Loop over the number of periods in the raw file, putting each period in a separate workspace
  for (int period = 0; period < m_numberOfPeriods; ++period)
  {
    if (period > 0)
    {
      localWorkspace=createWorkspace(localWorkspace);

      if (bLoadlogFiles)
      {
        //remove previous period data
        std::stringstream prevPeriod;
        prevPeriod << "PERIOD " << (period);
        Sample& sampleObj = localWorkspace->mutableSample();
        sampleObj.removeLogData(prevPeriod.str());
        //add current period data
        Property* log = createPeriodLog(period+1);
        if (log) sampleObj.addLogData(log);
      }

    }
    skipData(file, period * (m_numberOfSpectra + 1));
    int wsIndex = 0;
    for (int i = 1; i <= m_numberOfSpectra; ++i)
    {
      int histToRead = i + period * (m_numberOfSpectra + 1);
      if ((i >= m_spec_min && i < m_spec_max) || (m_list && find(m_spec_list.begin(), m_spec_list.end(),
          i) != m_spec_list.end()))
      {
        progress(m_prog, "Reading raw file data...");
        //readData(file, histToRead);
		//read spectrum 
		if (!readData(file, histToRead))
		{
			throw std::runtime_error("Error reading raw file");
		}
		int binStart=0;
		setWorkspaceData(localWorkspace, m_timeChannelsVec, wsIndex, i, m_noTimeRegimes,1,binStart);
		++wsIndex;

        if (m_numberOfPeriods == 1)
        {
          if (++histCurrent % 100 == 0)
          {
            m_prog = double(histCurrent) / histTotal;
          }
          interruption_point();
        }

      }
	  else
	  {
		  skipData(file, histToRead);
	  }
    
    }
	
	if(m_numberOfPeriods>1)
	{
		setWorkspaceProperty(localWorkspace, ws_grp, period, false);
		// progress for workspace groups 
		m_prog = (double(period) / (m_numberOfPeriods - 1));
	}
  
  } // loop over periods
  // Clean up
  isisRaw.reset();
  fclose(file);
}

/// This sets the optional property to the LoadRawHelper class
void LoadRawBin0::setOptionalProperties()
{
  //read in the settings passed to the algorithm
  m_spec_list = getProperty("SpectrumList");
  m_spec_max = getProperty("SpectrumMax");
  m_spec_min = getProperty("SpectrumMin");

}


} // namespace DataHandling
} // namespace Mantid
