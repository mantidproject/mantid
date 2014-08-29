#include "MantidDataHandling/LoadFITS.h"
#include "MantidAPI/MultipleFileProperty.h"
#include "MantidAPI/RegisterFileLoader.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/UnitFactory.h"
#include <boost/algorithm/string.hpp>
#include <Poco/BinaryReader.h>

using namespace Mantid::DataHandling;
using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace std;
using namespace boost;
using Poco::BinaryReader;


namespace Mantid
{
namespace DataHandling
{
	// Register the algorithm into the AlgorithmFactory
	DECLARE_FILELOADER_ALGORITHM(LoadFITS);

	/**
	* Return the confidence with with this algorithm can load the file
	* @param descriptor A descriptor for the file
	* @returns An integer specifying the confidence level. 0 indicates it will not be used
	*/
	int LoadFITS::confidence(Kernel::FileDescriptor & descriptor) const
	{
		// Should really improve this to check the file header (of first file at least) to make sure it contains the fields wanted
		return (descriptor.extension() == ".fits" || descriptor.extension() == ".fit") ? 80 : 0; 
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
		declareProperty(new PropertyWithValue<int>("FileChunkSize", 100, Direction::Input), "Number of files to read into memory at a time - use lower values for machines with low memory");
		
		declareProperty(new API::WorkspaceProperty<API::MatrixWorkspace>("OutputWorkspace", "", Kernel::Direction::Output));    
	}

	/**
	* Execute the algorithm.
	*/
	void LoadFITS::exec()
	{	 
		// Create FITS file information for each file selected
		std::vector<std::string> paths;
    string fName = getPropertyValue("Filename");
		boost::split(paths, fName, boost::is_any_of(","));
		m_binChunkSize = getProperty("FileChunkSize");

		// Shrink chunk size to match number of files if it's over the amount (less memory allocated later)
		if(m_binChunkSize > paths.size()) m_binChunkSize = static_cast<int>(paths.size());

		m_allHeaderInfo.resize(paths.size());

		// Check each header is valid for this loader, - standard (no extension to FITS), and has two axis
		bool headerValid = true;

		for(size_t i=0; i<paths.size();++i)
		{
			m_allHeaderInfo[i].extension = "";
			m_allHeaderInfo[i].filePath = paths[i];
			// Get various pieces of information from the file header which are used to create the workspace
			if(parseHeader(m_allHeaderInfo[i]))
			{
				// Get and convert specific standard header values which will help when parsing the data
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
				catch(std::exception &)
				{
					//todo write error and fail this load with invalid data in file.
					g_log.error("Unable to locate one or more valid BITPIX, NAXIS, TOF, TIMEBIN, N_COUNTS or N_TRIGS values in the FITS file header.");
					throw std::runtime_error("Unable to locate one or more valid BITPIX, NAXIS, TOF, TIMEBIN, N_COUNTS or N_TRIGS values in the FITS file header.");
				}

				if(m_allHeaderInfo[i].extension != "") headerValid = false;
				if(m_allHeaderInfo[i].numberOfAxis != 2) headerValid = false;

				// Test current item has same axis values as first item.
				if(m_allHeaderInfo[0].axisPixelLengths[0] != m_allHeaderInfo[i].axisPixelLengths[0]) headerValid = false;
				if(m_allHeaderInfo[0].axisPixelLengths[1] != m_allHeaderInfo[i].axisPixelLengths[1]) headerValid = false;
			}
			else
			{
				// Unable to parse the header, throw.
				g_log.error("Unable to open the FITS file.");
				throw std::runtime_error("Unable to open the FITS file.");
			}

		}

		// Check that the files use bit depths of either 8, 16 or 32
		if(m_allHeaderInfo[0].bitsPerPixel != 8 && m_allHeaderInfo[0].bitsPerPixel != 16 && m_allHeaderInfo[0].bitsPerPixel != 32) 
		{
			 g_log.error("FITS Loader only supports 8, 16 or 32 bits per pixel.");
			 throw std::runtime_error("FITS loader only supports 8, 16 or 32 bits per pixel.");
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
			 g_log.error("Loader currently doesn't support FITS files with non-standard extensions, greater than two axis of data, or has detected that all the files are not similar.");
			 throw std::runtime_error("Loader currently doesn't support FITS files with non-standard extensions, greater than two axis of data, or has detected that all the files are not similar.");
		}    
	}

	/**
	* Read a single files header and populate an object with the information
	* @param headerInfo A FITSInfo file object to parse header information into
	* @returns A bool specifying succes of the operation
	*/
	bool LoadFITS::parseHeader(FITSInfo &headerInfo)
	{		
		bool ranSuccessfully = true;
		try
		{
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
		}
		catch(...)
		{
			// Unable to read the file
			ranSuccessfully = false;
		}

		return ranSuccessfully;
	}

	/**
	* Create histogram workspace
	* @returns Created workspace
	*/
	MatrixWorkspace_sptr LoadFITS::initAndPopulateHistogramWorkspace()
	{		
		MantidVecPtr x;
		x.access().resize(m_allHeaderInfo.size() + 1);

		// Init time bins
		double binCount = 0;
		for(size_t i=0;i<m_allHeaderInfo.size() + 1; ++i)
		{
			x.access()[i] = binCount;
			if(i != m_allHeaderInfo.size()) binCount += m_allHeaderInfo[i].timeBin;
		}

		size_t spectraCount = 0;
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
			throw std::runtime_error("FITS loader couldn't allocate enough memory to run. Try a smaller chunk size.");	
		}

		size_t steps = static_cast<size_t>(ceil(m_allHeaderInfo.size()/m_binChunkSize));
		Progress prog(this,0.0,1.0,steps);
		
		// Load a chunk of files at a time into workspace
		try
		{
			for(size_t i=0; i<m_allHeaderInfo.size(); i+=m_binChunkSize)
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

	/**
	* Loads data from a selection of the FITS files into the workspace
	* @param workspace The workspace to insert data into
	* @param yVals Reference to a pre-allocated vector to hold data values for the workspace
	* @param eVals Reference to a pre-allocated vector to hold error values for the workspace
	* @param bufferAny Pointer to an allocated memory region which will hold a files worth of data
	* @param x Vector holding the X bin values
	* @param spectraCount Number of data points in each file
	* @param bitsPerPixel Number of bits used to represent one data point 
	* @param binChunkStartIndex Index for the first file to be processed in this chunk 
	*/
	void LoadFITS::loadChunkOfBinsFromFile(MatrixWorkspace_sptr &workspace, vector<vector<double> > &yVals, vector<vector<double> > &eVals, void *&bufferAny, MantidVecPtr &x, size_t spectraCount, int bitsPerPixel, long binChunkStartIndex)
	{
		size_t binsThisChunk = m_binChunkSize;
		if((binChunkStartIndex + m_binChunkSize) > m_allHeaderInfo.size())
		{
			// No need to do extra processing if number of bins to process is lower than m_binChunkSize
			// Also used to prevent out of bounds error where a greater number of elements have been reserved.
			binsThisChunk = static_cast<size_t>(m_allHeaderInfo.size() - binChunkStartIndex);
		}       

		uint8_t *buffer8 = NULL;
		uint16_t *buffer16 = NULL;
		uint32_t *buffer32 = NULL;
		
		// create pointer of correct data type to void pointer of the buffer:
		buffer8 = static_cast<uint8_t*>(bufferAny);
		buffer16 = static_cast<uint16_t*>(bufferAny);
		buffer32 = static_cast<uint32_t*>(bufferAny);

		FILE *currFile = NULL;
		size_t result = 0;
		double val = 0;
		bool fileErr;

		for(size_t i=binChunkStartIndex; i < binChunkStartIndex+binsThisChunk ; ++i)
		{      
			// Read Data
			fileErr = false;
			currFile = fopen ( m_allHeaderInfo[i].filePath.c_str(), "rb" );
			if (currFile==NULL) fileErr = true;    
			
			if(!fileErr)
			{
				fseek (currFile , FIXED_HEADER_SIZE , SEEK_CUR);
				result = fread(bufferAny, bitsPerPixel/8, spectraCount, currFile);
			}

			if (result != spectraCount) fileErr = true;			

			if(fileErr)
			{
				throw std::runtime_error("Error reading file; possibly invalid data.");	
			}

			for(size_t j=0; j<spectraCount;++j)
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
		for (int64_t wi = 0; wi < static_cast<int64_t>(spectraCount); ++wi)
		{
			workspace->setX(wi, x);
			MantidVec *currY = &workspace->dataY(wi);
			MantidVec *currE = &workspace->dataE(wi);
			
			std::copy(yVals[wi].begin(), yVals[wi].end()-(m_binChunkSize-binsThisChunk), currY->begin()+binChunkStartIndex );
			std::copy(eVals[wi].begin(), eVals[wi].end()-(m_binChunkSize-binsThisChunk), currE->begin()+binChunkStartIndex );

			// I expect this will be wanted once IDF is in a more useful state.
			//workspace->getSpectrum(wi)->setDetectorID(detid_t(wi));
			//workspace->getSpectrum(wi)->setSpectrumNo(specid_t(wi+1));
		}           
	}		
}
}

