#include "MantidAPI/FileProperty.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidDataHandling/FindDetectorsPar.h"
#include "MantidDataHandling/SaveNXTomo.h"
#include "MantidGeometry/IComponent.h"
#include "MantidKernel/MantidVersion.h"
#include "MantidNexus/NexusClasses.h"

namespace Mantid
{
	namespace DataHandling
	{    
		// Register the algorithm into the algorithm factory
		DECLARE_ALGORITHM(SaveNXTomo)

		using namespace Kernel;
		using namespace API;
		using Geometry::RectangularDetector;

		const double SaveNXTomo::MASK_FLAG = std::numeric_limits<double>::quiet_NaN();
		const double SaveNXTomo::MASK_ERROR = 0.0;
		const std::string SaveNXTomo::NXTOMO_VER = "2.0";

		SaveNXTomo::SaveNXTomo() :  API::Algorithm()
		{
			m_filename = "";
			m_numberOfRows = 32;
		}

		/**
		 * Initialise the algorithm
		 */
		void SaveNXTomo::init()
		{
			auto wsValidator = boost::make_shared<CompositeValidator>() ;
			//wsValidator->add(boost::make_shared<API::WorkspaceUnitValidator>("DeltaE"));
			wsValidator->add<API::CommonBinsValidator>();
			wsValidator->add<API::HistogramValidator>();

			declareProperty(new WorkspaceProperty<MatrixWorkspace> ("InputWorkspace",	"", Direction::Input, wsValidator),
				"The name of the workspace to save.");

			declareProperty(new API::FileProperty("Filename", "", FileProperty::Save, std::vector<std::string>(1,".nxs")),
				"The name of the NXTomo file to write, as a full or relative path");

			declareProperty(new PropertyWithValue<size_t>("Row chunk size", 32, Kernel::Direction::Input), 
				"Please use an evenly divisible number smaller than the image height");
		}

		/**
		 * Execute the algorithm
		 */
		void SaveNXTomo::exec()
		{
			// Retrieve the input workspace
			const MatrixWorkspace_const_sptr inputWS = getProperty("InputWorkspace");
			
			m_numberOfRows = getProperty("Row chunk size");

			const std::string workspaceID = inputWS->id();
			
			if ((workspaceID.find("Workspace2D") == std::string::npos) &&
				(workspaceID.find("RebinnedOutput") == std::string::npos)) 
					throw Exception::NotImplementedError("SaveNXTomo passed invalid workspaces. Must be Workspace2D");
			
			// Do the full check for common binning
			if (!WorkspaceHelpers::commonBoundaries(inputWS))
			{
				g_log.error("The input workspace must have common bins");
				throw std::invalid_argument("The input workspace must have common bins");
			}

			// Number of spectra
			const size_t nHist = inputWS->getNumberHistograms();
			// Number of energy bins
			//this->m_nBins = inputWS->blocksize();

			// Get a pointer to the sample
			//Geometry::IComponent_const_sptr sample =
			//    inputWS->getInstrument()->getSample();

			// Retrieve the filename from the properties
			this->m_filename = getPropertyValue("Filename");
				 
			// Dimensions for axis in nxTomo file.
			std::vector<int64_t> dims_array;
			dims_array.push_back(inputWS->blocksize()); // Number of bins

			// Populate the array
			getDimensionsFromDetector(getRectangularDetectors(inputWS->getInstrument()), dims_array);
			 
			// Create the file.
			::NeXus::File nxFile(this->m_filename, NXACC_CREATE5);
			
			// Make the top level entry (and open it)
			nxFile.makeGroup("entry1", "NXentry", true);

			// Make a sub-group for the entry to work with DAWN software (and open it)
			nxFile.makeGroup("tomo_entry", "NXsubentry", true);

			// Title
			nxFile.writeData("title", this->m_filename);
			
			// Start Time; Format ISO8601 | unused but part of NXtomo schema
			//nxFile.writeData("start_time", );

			// End Time; Format ISO8601 | unused but part of NXtomo schema
			//nxFile.writeData("end_time", );

			// Definition name and version
			nxFile.writeData("definition", "NXtomo");
			nxFile.openData("definition");
			nxFile.putAttr("version", NXTOMO_VER);
			nxFile.closeData();

			// Originating program name and version
			nxFile.writeData("program_name", "mantid");
			nxFile.openData("program_name");
			nxFile.putAttr("version", Mantid::Kernel::MantidVersion::version());
			nxFile.closeData();            

			// ******************************************
			// NXinstrument
			nxFile.makeGroup("instrument", "NXinstrument", true);
			// Write the instrument name | could add short_name attribute to name
			nxFile.writeData("name", inputWS->getInstrument()->getName());
						 
			// detector group - diamond example file contains {data,distance,image_key,x_pixel_size,y_pixel_size} Only adding image_key for now, 0 filled.
			nxFile.makeGroup("detector", "NXdetector", true);
			std::vector<double> imageKeys(dims_array[0],0);
			nxFile.writeData("image_key", imageKeys);
			// Create link to image_key
			nxFile.openData("image_key");
			NXlink imageKeyLink = nxFile.getDataID();      
			nxFile.closeData();
			nxFile.closeGroup();

			// source group // from diamond file contains {current,energy,name,probe,type} - probe = [neutron | x-ray | electron]			
			
			nxFile.closeGroup(); // NXinstrument

			// ******************************************
			// NXsample
			nxFile.makeGroup("sample", "NXsample", true);
			// TODO: Write sample info
			// name
			
			std::vector<double> rotationAngles(dims_array[0]);
			// Initialise rotations - if unknown, fill with equal steps from 0 to 180 over all frames.
			// TODO: collect and use actual rotation values

			double step = static_cast<double>(180/dims_array[0]);
			rotationAngles[0] = step;
			
			for(auto it = rotationAngles.begin()+1; it != rotationAngles.end(); ++it)
			{
				*it = (*(it-1)) + step;
			}

			nxFile.writeData("rotation_angle", rotationAngles);
			
			// Create a link object for rotation_angle to use later
			nxFile.openData("rotation_angle");
				NXlink rotationLink = nxFile.getDataID();
			nxFile.closeData();
			// x_translation
			// y_translation
			// z_translation
			nxFile.closeGroup(); // NXsample
			
			// ******************************************
			// Make the NXmonitor group - Holds base beam intensity for each image
			// If information is not present, set as 1

			std::vector<double> intensity(dims_array[0],1);   
			nxFile.makeGroup("control", "NXmonitor", true);         
				nxFile.writeData("data", intensity);
			nxFile.closeGroup(); // NXmonitor          
			 
			nxFile.makeGroup("data", "NXdata", true);
								
			nxFile.makeLink(rotationLink);

			nxFile.makeData("data", ::NeXus::FLOAT64, dims_array, false);
			nxFile.makeData("error", ::NeXus::FLOAT64, dims_array, false);

			std::vector<int64_t> slabStart;
			std::vector<int64_t> slabSize;

			// What size slabs are we going to write			
			slabSize.push_back(dims_array[0]);
			slabSize.push_back((int64_t)dims_array[1]);
			slabSize.push_back((int64_t)m_numberOfRows);
				
			// Init start to first row
			slabStart.push_back(0);
			slabStart.push_back(0);
			slabStart.push_back(0);

			// define the data and error vectors for masked detectors
			std::vector<double> masked_data (dims_array[0], MASK_FLAG);
			std::vector<double> masked_error (dims_array[0], MASK_ERROR);	

			// Create a progress reporting object
			Progress progress(this,0,1,100);
			const size_t progStep = static_cast<size_t>(ceil(nHist/100.0));
			Geometry::IDetector_const_sptr det;
			
			double *dataArr = new double[dims_array[0]*dims_array[2]*m_numberOfRows];
			double *errorArr = new double[dims_array[0]*dims_array[2]*m_numberOfRows];

			int currY = 0;
			size_t rowIndForSlab = 0; // as we're creating slabs of multiple rows, this says which y index we're at in current slab

			// Loop over detectors
			for (size_t i = 0; i < nHist; ++i)
			{      
				try
				{ 
					// detector exist
					//det = inputWS->getDetector(i);
					// Check that we aren't writing a monitor
					//if (!det->isMonitor())
					//{
						//Geometry::IDetector_const_sptr det = inputWS->getDetector(i);
				
					// Figure out where this pixel is supposed to be going and set the correct slab start.
							
					if(i!=0 && (i)%dims_array[1] == 0){ // When this iteration matches end of a row
						currY += 1;
					}
					size_t currX = (i) - (currY*dims_array[1]);
									
					const MantidVec & thisY = inputWS->readY(i);
					// No masking - Set the data and error as is
					for(int j=0; j<dims_array[0];++j)
					{        
						const size_t currInd = j*dims_array[2]*m_numberOfRows + currX*m_numberOfRows + rowIndForSlab;

					 // if(!det->isMasked())
					 // {              
							dataArr[currInd] = thisY.at(j);
							errorArr[currInd] = inputWS->readE(i).at(j);
						//}
						//else
						//{
						//  dataArr[currInd] = masked_data[j];
						//  errorArr[currInd] = masked_error[j];
						//}
					}   
				
					// If end of the row has been reached, check for end of slab and write data/error
					if(((i+1)%dims_array[2]) == 0)
					{
						rowIndForSlab += 1;

						// if a slab has been collected. Put it into the file
						if(rowIndForSlab >= m_numberOfRows)
						{                       
							slabStart[2] = currY-(rowIndForSlab-1);    
						
							// Write Data
							nxFile.openData("data");                                  
								nxFile.putSlab(dataArr, slabStart, slabSize);                
							nxFile.closeData();
							// Write Error
							nxFile.openData("error");                                
								nxFile.putSlab(errorArr, slabStart, slabSize);                
							nxFile.closeData();
						
							// Reset slab index count
							rowIndForSlab = 0;
						}
					}			
				}
				catch(Exception::NotFoundError&)
				{
						/*Catch if no detector. Next line tests whether this happened - test placed
						outside here because Mac Intel compiler doesn't like 'continue' in a catch
						in an openmp block.*/
				}
				// If no detector found, skip onto the next spectrum
				if ( !det ) continue;
	
				// Make regular progress reports and check for canceling the algorithm
				if ( i % progStep == 0 )
				{
					progress.report();
				}        
			}
			
			// Create a link object for the data
			nxFile.openData("data");
				NXlink dataLink = nxFile.getDataID();
			nxFile.closeData();       

			nxFile.closeGroup(); // Close Data group

			// Put a link to the data in instrument/detector
			nxFile.openGroup("instrument","NXinstrument");
				nxFile.openGroup("detector","NXdetector");
					nxFile.makeLink(dataLink);
				nxFile.closeGroup();
			nxFile.closeGroup();		


			nxFile.closeGroup(); // tomo_entry sub-group
			nxFile.closeGroup(); // Top level NXentry          

			// Clean up memory
			delete [] dataArr;
			delete [] errorArr;
		}

		/**
		* Find all RectangularDetector objects in an instrument
		* @param instrument instrument to search for detectors in
		* @returns vector of all Rectangular Detectors
		*/
		std::vector<RectangularDetector> SaveNXTomo::getRectangularDetectors(const Geometry::Instrument_const_sptr &instrument)
		{
			std::vector<boost::shared_ptr<const Mantid::Geometry::IComponent>> components;
			instrument->getChildren(components,true);

			std::vector<RectangularDetector> rectDetectors;
		
			for(auto it = components.begin(); it != components.end(); ++it)
			{
				// for all components, compare to RectangularDetector - if it is one, add it to detectors list. 
				const Geometry::IComponent* c = dynamic_cast<const Geometry::IComponent*>(&(**it));
					
				if(dynamic_cast<const RectangularDetector*>(c))
				{
					rectDetectors.push_back(*(dynamic_cast<const RectangularDetector *>(&(**it))));       
				}     
			}
			
			return rectDetectors;
		}

		/**
		* Populates the dimensions vector with number of files, x and y sizes from a specified rectangular detector
		* @param rectDetectors List of rectangular detectors to get axis sizes from
		* @param dims_array vector which is populated with the different dimension values for the nxTomo file
		* @param useDetectorIndex index of the detector to select from the list, default = 0
		* 
		*  @throw runtime_error Thrown if there are no rectangular detectors
		*/
		void SaveNXTomo::getDimensionsFromDetector(std::vector<RectangularDetector> &rectDetectors, std::vector<int64_t> &dims_array, size_t useDetectorIndex) 
		{
			// Add number of pixels in X and Y from instrument definition
			// Throws if no rectangular detector is present.

			if(rectDetectors.size() != 0)
			{
				// Assume the first rect detector is the desired one.
				dims_array.push_back(rectDetectors[useDetectorIndex].xpixels());
				dims_array.push_back(rectDetectors[useDetectorIndex].ypixels());
			}
			else
			{
				// Incorrect workspace : requires the x/y pixel count from the instrument definition
				g_log.error("Unable to retrieve x and y pixel count from an instrument definition associated with this workspace.");
				throw std::runtime_error("Unable to retrieve x and y pixel count from an instrument definition associated with this workspace.");			
			}
		}

		//void someRoutineToAddDataToExisting()
		//{
		//  //TODO:
		//}

	} // namespace DataHandling
} // namespace Mantid


