#include "MantidDataHandling/LoadFITS.h"

#include "MantidDataObjects/EventWorkspace.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/MultipleFileProperty.h"
#include "MantidAPI/RegisterFileLoader.h"

#include "MantidAPI/SpectraAxis.h"
#include "MantidAPI/NumericAxis.h"
#include "MantidGeometry/Objects/ShapeFactory.h"
#include "MantidGeometry/Instrument/ReferenceFrame.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidDataObjects/Workspace2D.h"

#include <Poco/BinaryReader.h>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

//#include "MantidAPI/WorkspaceValidators.h"
//#include "MantidKernel/UnitFactory.h"
//#include "MantidGeometry/Instrument.h"
//#include "MantidGeometry/Instrument/RectangularDetector.h"
//#include "MantidGeometry/Objects/ShapeFactory.h"
//
//#include "MantidNexus/NexusClasses.h"
//
//#include <boost/math/special_functions/fpclassify.hpp>
//#include <Poco/File.h>
//#include <iostream>
//#include <fstream>
//#include <iomanip>

using namespace Mantid::DataHandling;
using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;
using namespace std;
using namespace boost::algorithm;
using namespace boost;
using Poco::BinaryReader;


using Mantid::MantidVec;
using Mantid::MantidVecPtr;



namespace Mantid
{
namespace DataHandling
{
	// Register the algorithm into the AlgorithmFactory
	DECLARE_FILELOADER_ALGORITHM(LoadFITS);

	int LoadFITS::confidence(Kernel::FileDescriptor & descriptor) const
	{
		// TODO should improve this to check the file header (of first file at least) to make sure it contains the fields wanted
			return (descriptor.extension() == ".fits" || descriptor.extension() == ".fit") ? 80 : 0; 
	}

	/**
	* Execute the algorithm.
	*/
	void LoadFITS::exec()
	{
		// TODO check that each map check actually has the key required when searched for. raise error if not.
	 
		// Create FITS file information for each file selected
		std::vector<std::string> paths;
		boost::split(paths, getPropertyValue("Filename"), boost::is_any_of(","));

		m_allHeaderInfo.resize(paths.size());

		// Check each header is valid for this loader, - standard (no extension to FITS), and has two axis
		bool headerValid = true;

		for(int i=0; i<paths.size();++i)
		{
			m_allHeaderInfo[i].extension = "";
			m_allHeaderInfo[i].filePath = paths[i];
			// Get various pieces of information from the file header which are used to create the workspace
			if(parseHeader(m_allHeaderInfo[i]))
			{
				// Get and convert specific header values which will help when parsing the data
				// BITPIX, NAXIS, NAXISi (where i = 1..NAXIS, e.g. NAXIS2 for two axis), TOF, TIMEBIN, N_COUNTS, N_TRIGS
				try
				{
					m_allHeaderInfo[i].bitsPerPixel = lexical_cast<int>(m_allHeaderInfo[i].headerKeys["BITPIX"]);
					m_allHeaderInfo[i].numberOfAxis = lexical_cast<int>(m_allHeaderInfo[i].headerKeys["NAXIS"]);
			
					for(int j=0; j<m_allHeaderInfo[i].numberOfAxis; ++j)
					{
						string keyName = "NAXIS" + lexical_cast<string>(j+1);
						m_allHeaderInfo[i].axisPixelLengths.push_back(lexical_cast<int>(m_allHeaderInfo[i].headerKeys[keyName]));
					}

					m_allHeaderInfo[i].tof = lexical_cast<double>(m_allHeaderInfo[i].headerKeys["TOF"]);
					m_allHeaderInfo[i].timeBin = lexical_cast<double>(m_allHeaderInfo[i].headerKeys["TIMEBIN"]);
					m_allHeaderInfo[i].countsInImage = lexical_cast<long int>(m_allHeaderInfo[i].headerKeys["N_COUNTS"]);
					m_allHeaderInfo[i].numberOfTriggers = lexical_cast<long int>(m_allHeaderInfo[i].headerKeys["N_TRIGS"]);
					m_allHeaderInfo[i].extension = m_allHeaderInfo[i].headerKeys["XTENSION"]; // Various extensions are available to the FITS format, and must be parsed differently if this is present
				}
				catch(bad_lexical_cast &)
				{
					//todo write error and fail this load with invalid data in file.
				}

				if(m_allHeaderInfo[i].extension != "") headerValid = false;
				if(m_allHeaderInfo[i].numberOfAxis != 2) headerValid = false;

				// Test current item has same axis values as first item.
				if(m_allHeaderInfo[0].axisPixelLengths[0] != m_allHeaderInfo[i].axisPixelLengths[0]) headerValid = false;
				if(m_allHeaderInfo[0].axisPixelLengths[1] != m_allHeaderInfo[i].axisPixelLengths[1]) headerValid = false;
			}

		}

		// Check the format is correct and create the Workspace  
		if(headerValid)
		{
			// No extension is set, therefore it's the standard format which we can parse.
			
			// Delete the output workspace name if it existed
			std::string outName = getPropertyValue("OutputWorkspace");
			if (AnalysisDataService::Instance().doesExist(outName))   AnalysisDataService::Instance().remove(outName);
						
			MatrixWorkspace_sptr ws;
			
			ws = initAndPopulateHistogramWorkspace();    

			// Assign it to the output workspace property
			setProperty("OutputWorkspace",ws);
		}
		else
		{
			// Invalid files, record error
			// TODO

		}
	}

	/**
	* Initialise the algorithm. Declare properties which can be set before execution (input) or 
	* read from after the execution (output).
	*/
	void LoadFITS::init()
	{
		// Specify file extensions which can be associated with a FITS file.
		std::vector<std::string> exts;

		// Declare the Filename algorithm property. Mandatory. Sets the path to the file to load.
		exts.clear();
		exts.push_back(".fits");
		exts.push_back(".fit");
		
		// Specify as a MultipleFileProperty to alert loader we want multiple selected files to be loaded into a single workspace.
		declareProperty(new MultipleFileProperty("Filename", exts), "The input filename of the stored data");
		
		declareProperty(new API::WorkspaceProperty<API::MatrixWorkspace>("OutputWorkspace", "", Kernel::Direction::Output));    
	}


	bool LoadFITS::parseHeader(FITSInfo &headerInfo)
	{
		// TODO test file exists, test file can be read, make it return false if failed.
		bool ranSuccessfully = true;
		ifstream istr(headerInfo.filePath, ios::binary);
		Poco::BinaryReader reader(istr);
	
		// Iterate 80 bytes at a time until header is parsed | 2880 bytes is the fixed header length of FITS
		// 2880/80 = 36 iterations required
		for(int i=0; i < 36; ++i)
		{   
			// Keep vect of each header item, including comments, and also keep a map of individual keys.
			string part;
			reader.readRaw(80,part);  
			headerInfo.headerItems.push_back(part);
		
			// Add key/values - these are separated by the = symbol. 
			// If it doesn't have an = it's a comment to ignore. All keys should be unique
			int eqPos = part.find('=');
			if(eqPos > 0)
			{        
				string key = part.substr(0, eqPos);
				string value = part.substr(eqPos+1);
				
				// Comments are added after the value separated by a / symbol. Remove.
				int slashPos = value.find('/');
				if(slashPos > 0) value = value.substr(0, slashPos);
 
				boost::trim(key);
				boost::trim(value);
				headerInfo.headerKeys[key] = value;
			}    
		}

		istr.close();

		return ranSuccessfully;
	}

		//----------------------------------------------------------------------------------------------
		/** Create histogram workspace
	 */
	MatrixWorkspace_sptr LoadFITS::initAndPopulateHistogramWorkspace()
	{
		// TODO will take vector of FITSInfo for multiple files and load all into workspace

		MantidVecPtr x;
		x.access().resize(m_allHeaderInfo.size() + 1);

		// X = TIMEBIN value
		x.access()[0] = m_allHeaderInfo[0].timeBin;
		x.access()[1] = m_allHeaderInfo[0].timeBin + m_allHeaderInfo[0].tof;

		long spectraCount = 0;
		if(m_allHeaderInfo[0].numberOfAxis > 0) spectraCount += m_allHeaderInfo[0].axisPixelLengths[0];

		// Presumably 2 axis, but futureproofing.
		for(int i=1;i<m_allHeaderInfo[0].numberOfAxis;++i)
		{
			spectraCount *= m_allHeaderInfo[0].axisPixelLengths[i];
		}

		MatrixWorkspace_sptr retVal(new DataObjects::Workspace2D);
		retVal->initialize(spectraCount, m_allHeaderInfo.size()+1, m_allHeaderInfo.size());

		IAlgorithm_sptr loadInst = createChildAlgorithm("LoadInstrument");

		try 
		{
			std::string directoryName = Kernel::ConfigService::Instance().getInstrumentDirectory();
			directoryName = directoryName + "/IMAT_Definition.xml";
			
			loadInst->setPropertyValue("Filename", directoryName);
			loadInst->setProperty<MatrixWorkspace_sptr>("Workspace", retVal);
			loadInst->execute();
		} 
		catch (std::exception ex) 
		{
			g_log.information("Cannot load the instrument definition. " + string(ex.what()) );
		}

		for(int i=0; i<m_allHeaderInfo.size();++i)
		{
			loadSingleBinFromFile(retVal, m_allHeaderInfo[i], x, spectraCount, i);
		}
		
		retVal->mutableRun().addProperty("Filename", m_allHeaderInfo[0].filePath);

		// Set the Unit of the X Axis
		try
		{
			retVal->getAxis(0)->unit() = UnitFactory::Instance().create("TOF");
		}
		catch ( Exception::NotFoundError & )
		{
			retVal->getAxis(0)->unit() = UnitFactory::Instance().create("Label");
			Unit_sptr unit = retVal->getAxis(0)->unit();
			boost::shared_ptr<Units::Label> label = boost::dynamic_pointer_cast<Units::Label>(unit);
			label->setLabel("TOF", "TOF");
		}

		retVal->setYUnit("Counts");
		retVal->setTitle("Test Workspace");

		return retVal;
	}

 

	void LoadFITS::loadSingleBinFromFile(MatrixWorkspace_sptr &workspace, FITSInfo &fitsInfo, MantidVecPtr &x, long spectraCount, long binIndex)
	{
		// READ DATA
		ifstream istr(fitsInfo.filePath, ios::binary);
		Poco::BinaryReader reader(istr);
		string tmp;
		reader.readRaw(2880, tmp); // Read header as full block to skip to data
		// read bitdepth*naxis1 num of bits for first row? repeat naxis2 number of times.
		int bytesPerRow = (fitsInfo.bitsPerPixel*fitsInfo.axisPixelLengths[0])/8;
		
		int allDataSizeBytes = (fitsInfo.bitsPerPixel*fitsInfo.axisPixelLengths[0]*fitsInfo.axisPixelLengths[1])/8 ;
		string allData;
		reader >> allData;
		stringstream ss;
		ss << allData;

		int currPixel, currRow = 0;
		int8_t tmp8; 
		int16_t tmp16; 
		int32_t tmp32;    
		
		for(int i=0;i<fitsInfo.axisPixelLengths[1]; ++i) // loop rows
		{
			// Read all columns on this row. As MantidVecPt->setData expects vectors, populate data vectors for y and e.
			//data.push_back(vector<double>());
			currRow = i*fitsInfo.axisPixelLengths[0];

			for(int j=0; j<fitsInfo.axisPixelLengths[0];++j)
			{
				double val = 0;
				switch(fitsInfo.bitsPerPixel)
				{
				case 8:          
					ss >> tmp8;         
					val = static_cast<double>(tmp8);
				case 16: // 2 bytes uint_16           
					ss >> tmp16;
					val = static_cast<double>(tmp16);
					break;
				case 32:          
					ss >> tmp32;
					val = static_cast<double>(tmp32);
					break;
				default:
					// TODO unhandled, report error.
					break;
				}      
				
				currPixel = currRow + j;
				workspace->setX(currPixel,x); 
			 
				//workspace->setData(currPixel,y,e);
				workspace->dataY(currPixel)[binIndex] = val;
				workspace->dataE(currPixel)[binIndex] = sqrt(val);
				//workspace->dataX(currPixel)[binIndex] = x[0];

				workspace->getSpectrum(currPixel)->setDetectorID(detid_t(currPixel));
				workspace->getSpectrum(currPixel)->setSpectrumNo(specid_t(currPixel+1));
			}
			
		}



	}

}
}

// TODO: Correctly populate X values.

// about 12 seconds creating child algorithm
// about 18 s loading idf

// can probably create a vect for each bin and populate them all at once. faster?
//
//		std::clock_t start;
//		double duration;
//		start = std::clock();
//duration = ( std::clock() - start ) / (double) CLOCKS_PER_SEC;