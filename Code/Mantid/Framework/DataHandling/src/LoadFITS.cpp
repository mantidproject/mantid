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
		m_binChunkSize = getProperty("FileChunkSize");

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
					m_allHeaderInfo[i].extension = m_allHeaderInfo[i].headerKeys["XTENSION"]; // Various extensions are available to the FITS format, and must be parsed differently if this is present. Loader doesn't support this.
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

		// Check that the files use bit depths of either 8, 16 or 32
		if(m_allHeaderInfo[0].bitsPerPixel != 8 && m_allHeaderInfo[0].bitsPerPixel != 16 && m_allHeaderInfo[0].bitsPerPixel != 32) 
		{
			 g_log.error("FITS Loader only supports 8, 16 or 32 bits per pixel1.");
			 throw std::runtime_error("FITS loader only supports 8, 16 or 32 bits per pixel1.");
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
		declareProperty(new PropertyWithValue<int>("FileChunkSize", 1000, Direction::Input), "Number of files to read into memory at a time - use lower values for machines with low memory");
		
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
			auto eqPos = part.find('=');
			if(eqPos > 0)
			{        
				string key = part.substr(0, eqPos);
				string value = part.substr(eqPos+1);
				
				// Comments are added after the value separated by a / symbol. Remove.
				auto slashPos = value.find('/');
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

		// Init time bins
		double binCount = 0;
		for(int i=0;i<m_allHeaderInfo.size() + 1; ++i)
		{
      x.access()[i] = binCount;
			if(i != m_allHeaderInfo.size()) binCount += m_allHeaderInfo[i].timeBin;
		}

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

		int bitsPerPixel = m_allHeaderInfo[0].bitsPerPixel; // assumes all files have the same, which they should. 
		vector<vector<double> > yVals(spectraCount, std::vector<double>(m_binChunkSize));
		vector<vector<double> > eVals(spectraCount, std::vector<double>(m_binChunkSize));
		
		// allocate memory to contain the data section of the file:
		void * bufferAny = NULL;        
		bufferAny = malloc ((bitsPerPixel/8)*spectraCount);

		if (bufferAny == NULL) 
		{
				fputs ("Memory error",stderr); exit (2);
		}

		int steps = ceil(m_allHeaderInfo.size()/m_binChunkSize);
		Progress prog(this,0.0,1.0,steps);
		

		// Load a chunk of files at a time into workspace
		try
		{
			for(int i=0; i<m_allHeaderInfo.size(); i+=m_binChunkSize)
			{
				loadChunkOfBinsFromFile(retVal, yVals, eVals, bufferAny, x, spectraCount, bitsPerPixel, i);
				prog.report(name());
			}
		}
		catch(...)
		{
			// Exceptions should be handled internally, but catch here to free any memory. Belt and braces.
			free(bufferAny);
			g_log.error("FITS Loader unable to correctly parse files.");
			throw std::runtime_error("FITS loader unable to correctly parse files.");
		}

		// Memory no longer needed
		free (bufferAny);  
		
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

 

	void LoadFITS::loadChunkOfBinsFromFile(MatrixWorkspace_sptr &workspace, vector<vector<double> > &yVals, vector<vector<double> > &eVals, void *&bufferAny, MantidVecPtr &x, long spectraCount, int bitsPerPixel, long binChunkStartIndex)
	{
		int binsThisChunk = m_binChunkSize;
		if((binChunkStartIndex + m_binChunkSize) > m_allHeaderInfo.size())
		{
			// No need to do extra processing if number of bins to process is lower than m_binChunkSize
			binsThisChunk = m_allHeaderInfo.size() - binChunkStartIndex;
		}       

		int8_t *buffer8 = NULL;
		int16_t *buffer16 = NULL;
		int32_t *buffer32 = NULL;
		
		// create pointer of correct data type to void pointer of the buffer:
		buffer8 = static_cast<int8_t*>(bufferAny);
		buffer16 = static_cast<int16_t*>(bufferAny);
		buffer32 = static_cast<int32_t*>(bufferAny);

		FILE *currFile = NULL;
		size_t result = 0;
		double val = 0;

		for(int i=binChunkStartIndex; i < binChunkStartIndex+binsThisChunk ; ++i)
		{      
			// Read Data
			currFile = fopen ( m_allHeaderInfo[i].filePath.c_str(), "rb" );
			if (currFile==NULL) 
			{
				fputs ("File error",stderr); exit (1);
			}            
			
			fseek (currFile , FIXED_HEADER_SIZE , SEEK_CUR);
			result = fread(bufferAny, bitsPerPixel/8, spectraCount, currFile);
		 
			if (result != spectraCount) 
			{
				fputs ("Reading error",stderr); exit (3);
			}
			
			for(int j=0; j<spectraCount;++j)
			{
				if(bitsPerPixel == 8) val = static_cast<double>(buffer8[j]);
				if(bitsPerPixel == 16) val = static_cast<double>(buffer16[j]);
				if(bitsPerPixel == 32) val = static_cast<double>(buffer32[j]);

				yVals[j][i-binChunkStartIndex] = val;
				eVals[j][i-binChunkStartIndex] = sqrt(val);
			}				
			
			// Clear memory associated with the file load
			fclose (currFile);
		}

		// Now load chunk into workspace 
		PARALLEL_FOR1(workspace)
		for (int wi = 0; wi < spectraCount; ++wi)
		{
			workspace->setX(wi, x);
			//MantidVecPtr y, e;
			//y.access() = yVals[wi];
			//e.access() = eVals[wi];
			//retVal->setData(wi,y,e);

			//workspace.get()->dataY(wi)..insert(yVals[wi]);

			MantidVec *currY = &workspace->dataY(wi);
			MantidVec *currE = &workspace->dataE(wi);
			//currY->insert(currY->begin()+binChunkStartIndex, yVals[wi].begin(), yVals[wi].end());
			//currE->insert(currE->begin()+binChunkStartIndex, eVals[wi].begin(), eVals[wi].end());
			
			std::copy(yVals[wi].begin(), yVals[wi].end()-(m_binChunkSize-binsThisChunk), currY->begin()+binChunkStartIndex );
			std::copy(eVals[wi].begin(), eVals[wi].end()-(m_binChunkSize-binsThisChunk), currE->begin()+binChunkStartIndex );
			//workspace->dataY(wi).insert(push_back(yVals[wi]);//.push_back(y);
			//workspace->setData(wi, y, e); 
			//  workspace->getSpectrum(currPixel)->setDetectorID(detid_t(currPixel));
			//  workspace->getSpectrum(currPixel)->setSpectrumNo(specid_t(currPixel+1));
		}           
	}		
}
}






// TODO: Correctly populate X values.

// TODO: make buffer/malloc work with multiple bitdepths
// about 12 seconds creating child algorithm
// about 18 s loading idf

// can probably create a vect for each bin and populate them all at once. faster?
//
//		std::clock_t start;
//		double duration;
//		start = std::clock();
//duration = ( std::clock() - start ) / (double) CLOCKS_PER_SEC;

		//workspace->setData(currPixel,y,e);
				//workspace->dataX(currPixel)[binIndex] = x[0];


				
				//currPixel = currRow + j;
				//workspace->dataY(currPixel)[binIndex] = val;
				//workspace->dataE(currPixel)[binIndex] = sqrt(val);

			//int bytesPerRow = (m_allHeaderInfo[i].bitsPerPixel*m_allHeaderInfo[i].axisPixelLengths[0])/8;		
			//int allDataSizeBytes = (m_allHeaderInfo[i].bitsPerPixel*m_allHeaderInfo[i].axisPixelLengths[0]*m_allHeaderInfo[i].axisPixelLengths[1])/8 ;