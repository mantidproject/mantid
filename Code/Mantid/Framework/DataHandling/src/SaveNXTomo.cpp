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
    using namespace DataObjects;
    using Geometry::RectangularDetector;
        
    const std::string SaveNXTomo::NXTOMO_VER = "2.0";

    SaveNXTomo::SaveNXTomo() :  API::Algorithm()
    {
      m_filename = "";
      m_includeError = false;
      m_overwriteFile = false;
      m_spectraCount = 0;
    }

    /**
     * Initialise the algorithm
     */
    void SaveNXTomo::init()
    {
      auto wsValidator = boost::make_shared<CompositeValidator>() ;
      wsValidator->add<API::CommonBinsValidator>();
      wsValidator->add<API::HistogramValidator>();

      declareProperty(new WorkspaceProperty<>("InputWorkspaces",  "", Direction::Input, wsValidator),
        "The name of the workspaces to save.");

      declareProperty(new API::FileProperty("Filename", "", FileProperty::Save, std::vector<std::string>(1,".nxs")),
        "The name of the NXTomo file to write, as a full or relative path");

      declareProperty(new PropertyWithValue<bool>("OverwriteFile", false, Kernel::Direction::Input),
        "Replace any existing file of the same name instead of appending data?");

      declareProperty(new PropertyWithValue<bool>("IncludeError", false, Kernel::Direction::Input),
        "Write the error values to NXTomo file?");
    }

    /**
     * Execute the algorithm : Single workspace
     */
    void SaveNXTomo::exec()
    {
      try
      {
        MatrixWorkspace_sptr m = getProperty("InputWorkspaces");
        m_workspaces.push_back(boost::dynamic_pointer_cast<Workspace2D>(m));
      }
      catch(...)
      {}

      if(m_workspaces.size() != 0)
        processAll();
    }

   /**
    * Main exec routine, called for group or individual workspace processing.
    *
    * @throw NotImplementedError If input workspaces are invalid
    * @throw invalid_argument If workspace doesn't have common binning 
    */
    void SaveNXTomo::processAll()
    {
      m_includeError = getProperty("IncludeError");
      m_overwriteFile = getProperty("OverwriteFile");

       for(auto it=m_workspaces.begin();it!=m_workspaces.end();++it)
      {
        const std::string workspaceID = (*it)->id();
      
        if ((workspaceID.find("Workspace2D") == std::string::npos) &&
          (workspaceID.find("RebinnedOutput") == std::string::npos)) 
            throw Exception::NotImplementedError("SaveNXTomo passed invalid workspaces. Must be Workspace2D");
      
        // Do the full check for common binning
        if (!WorkspaceHelpers::commonBoundaries(*it))
        {
          g_log.error("The input workspace must have common bins");
          throw std::invalid_argument("The input workspace must have common bins");
        }
      }
       
      // Retrieve the filename from the properties
      this->m_filename = getPropertyValue("Filename");
            
      // Populate the array
      m_dimensions = getDimensionsFromDetector(getRectangularDetectors(m_workspaces[0]->getInstrument()));

      // Insert number of bins at front
      m_dimensions.insert(m_dimensions.begin(), m_workspaces.size()); // Number of bins      
       
      m_spectraCount = m_dimensions[1]*m_dimensions[2];

      // What size slabs are we going to write      
      m_slabSize.push_back(1);
      m_slabSize.push_back(m_dimensions[1]);
      m_slabSize.push_back(m_dimensions[2]);
        
      // Init start to first row
      m_slabStart.push_back(0);  
      m_slabStart.push_back(0); 
      m_slabStart.push_back(0);

      ::NeXus::File nxFile = setupFile();

      // Create a progress reporting object
      Progress progress(this,0,1,m_workspaces.size());

      for(auto it=m_workspaces.begin(); it!=m_workspaces.end();++it)
      {
        writeSingle(*it,nxFile);               
        progress.report();              
      }
      
      nxFile.close();
    }

   /**
    * Creates the format for the output file if it doesn't exist
    * @param resizeData should the data structures be resized rather than expanded
    * @returns the structured nexus file to write the data to
    *
    * @throw runtime_error Thrown if nexus file cannot be opened or created
    */
    ::NeXus::File SaveNXTomo::setupFile(bool resizeData)
    {      
      // Try and open the file, if it doesn't exist, create it.     
      NXhandle fileHandle;
      NXstatus status = NXopen(this->m_filename.c_str(), NXACC_RDWR, &fileHandle);
      
      std::vector<double> prevData, prevError, rotationAngles;
      //std::map<std::string, std::string> prevLogVals; 

      // rotations
      int numFiles = 0;

      if(status!= NX_ERROR && (!m_overwriteFile || resizeData))
      {
        // Retrieve all data currently in file so that it can be recreated with it.
        ::NeXus::File nxFile(fileHandle);
                
        nxFile.openPath("/entry1/tomo_entry/data");
      
        nxFile.getAttr<int>("NumFiles",numFiles);
        nxFile.readData<double>("data", prevData);
        
        if(m_includeError)
          nxFile.readData<double>("error", prevError);
             
        nxFile.readData<double>("rotation_angle",rotationAngles);
      }
          
      // Whatever the situation, we're recreating the file now
      if(status!=NX_ERROR) 
      {
        ::NeXus::File f(fileHandle);
        f.close();
      }

      status = NXopen(this->m_filename.c_str(), NXACC_CREATE5, &fileHandle);  
 
      if(status==NX_ERROR)
        throw std::runtime_error("Unable to open or create nexus file.");    
    
      // *********************************
      // Now genererate file and structure

      ::NeXus::File nxFile(fileHandle);    
   
      // Make the top level entry (and open it)
      nxFile.makeGroup("entry1", "NXentry", true);

      // Make an entry to store log values from the original files. Take only from latest one.
      nxFile.makeGroup("log_info","NXsubentry", true);
      
      // Populate from latest
      std::vector<Property*> log = (m_workspaces.back())->run().getLogData();
      for(auto it=log.begin();it!=log.end();++it)
      {
        nxFile.writeData((*it)->name(), (*it)->value());
      }       

      nxFile.closeGroup();

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
      nxFile.writeData("name", m_workspaces[0]->getInstrument()->getName());
             
      // detector group - diamond example file contains {data,distance,image_key,x_pixel_size,y_pixel_size} Only adding image_key for now, 0 filled.
      // TODO: change this when open and dark field files are dealt with
      nxFile.makeGroup("detector", "NXdetector", true);
      std::vector<double> imageKeys((resizeData) ? numFiles : numFiles+m_workspaces.size(),0);
      nxFile.writeData("image_key", imageKeys);
      // Create link to image_key
      nxFile.openData("image_key");
      //NXlink imageKeyLink = nxFile.getDataID();      
      nxFile.closeData();
      nxFile.closeGroup();

      // source group // from diamond file contains {current,energy,name,probe,type} - probe = [neutron | x-ray | electron]      
      
      nxFile.closeGroup(); // NXinstrument

      // ******************************************
      // NXsample
      nxFile.makeGroup("sample", "NXsample", true);
      // TODO: Write sample info
      // name

      std::vector<int64_t> rotDims;
      rotDims.push_back((resizeData) ? numFiles : numFiles+m_workspaces.size());
      nxFile.makeData("rotation_angle", ::NeXus::FLOAT64, rotDims, false);
      if(rotationAngles.size() > 0)
      {
        nxFile.openData("rotation_angle");
          nxFile.putSlab(rotationAngles, 0, static_cast<int>(rotationAngles.size()));
        nxFile.closeData();
      }

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
      // TODO: when FITS changes to include intensity, add intensity info

      std::vector<double> intensity((resizeData) ? numFiles : numFiles+m_workspaces.size(),1);   
      nxFile.makeGroup("control", "NXmonitor", true);         
        nxFile.writeData("data", intensity);
      nxFile.closeGroup(); // NXmonitor          
       
      nxFile.makeGroup("data", "NXdata", true);
        nxFile.putAttr<int>("NumFiles", numFiles);
                
      nxFile.makeLink(rotationLink);

      std::vector<int64_t> dims = m_dimensions;

      dims[0] = (resizeData) ? numFiles : numFiles+m_workspaces.size();
        
      nxFile.makeData("data", ::NeXus::FLOAT64, dims, false);
      if(m_includeError) 
        nxFile.makeData("error", ::NeXus::FLOAT64, dims, false);
              
      if(prevData.size() > 0)
      {
        m_slabStart[0] = 0;
        m_slabSize[0] = numFiles;

        nxFile.openData("data");
          nxFile.putSlab(prevData,m_slabStart, m_slabSize);
        nxFile.closeData();

        if(prevError.size() > 0)
        {
          nxFile.openData("error");
            nxFile.putSlab(prevError,m_slabStart, m_slabSize);
          nxFile.closeData();
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

      return nxFile;
    }


   /**
    * Writes a single workspace into the file
    * @param workspace the workspace to get data from
    * @param nxFile the nexus file to save data into
    */
    void SaveNXTomo::writeSingle(const Workspace2D_sptr workspace, ::NeXus::File &nxFile)
    {
      try
      {     
          nxFile.openPath("/entry1/tomo_entry/data");
      }
      catch(...)
      {        
        throw std::runtime_error("Unable to create a valid NXTomo file");    
      }

      int numFiles = 0;
      nxFile.getAttr<int>("NumFiles",numFiles);

      // Change slab start to after last data position
      m_slabStart[0] = numFiles;
      m_slabSize[0] = 1;
      
      // Set the rotation value for this WS
      std::vector<double> rotValue;
      rotValue.push_back(0);

      if(workspace->run().hasProperty("Rotation"))
      {
        std::string tmpVal = workspace->run().getLogData("Rotation")->value();
        try
        {
          rotValue[0] = boost::lexical_cast<double>(tmpVal);
        }
        catch(...){}
        // Invalid Cast is handled below        
      }      
        
      nxFile.openData("rotation_angle");
        nxFile.putSlab(rotValue, numFiles, 1);
      nxFile.closeData();   

      // Copy data out, remake data with dimension of old size plus new elements. Insert previous data.
      nxFile.openData("data");   
      
      double *dataArr = new double[m_spectraCount];

      for(int64_t i=0; i<m_dimensions[1];++i)
      {
        for(int64_t j=0; j<m_dimensions[2];++j)
        {
          dataArr[i*m_dimensions[1]+j] = workspace->dataY(i*m_dimensions[1]+j)[0];
        }
      }
      
      nxFile.putSlab(dataArr, m_slabStart, m_slabSize);  

      nxFile.closeData();
      
      nxFile.putAttr("NumFiles", ++numFiles);

      nxFile.closeGroup();

      free(dataArr);
    }


    /**
    * Find all RectangularDetector objects in an instrument
    * @param instrument instrument to search for detectors in
    * @returns vector of all Rectangular Detectors
    */
    std::vector<boost::shared_ptr<const RectangularDetector>> SaveNXTomo::getRectangularDetectors(const Geometry::Instrument_const_sptr &instrument)
    {
      std::vector<boost::shared_ptr<const Mantid::Geometry::IComponent>> components;
      instrument->getChildren(components,true);

      std::vector<boost::shared_ptr<const RectangularDetector>> rectDetectors;
    
      for(auto it = components.begin(); it != components.end(); ++it)
      {
        // for all components, compare to RectangularDetector - if it is one, add it to detectors list. 
        auto ptr = boost::dynamic_pointer_cast<const RectangularDetector>(*it);
        if(ptr != NULL)
        {
          rectDetectors.push_back(ptr);
        }
      }
      
      return rectDetectors;
    }

    /**
    * Populates the dimensions vector with number of files, x and y sizes from a specified rectangular detector
    * @param rectDetectors List of rectangular detectors to get axis sizes from
    * @param useDetectorIndex index of the detector to select from the list, default = 0
    * @returns vector of both axis dimensions for specified detector
    *
    *  @throw runtime_error Thrown if there are no rectangular detectors
    */
    std::vector<int64_t> SaveNXTomo::getDimensionsFromDetector(const std::vector<boost::shared_ptr<const RectangularDetector>> &rectDetectors, size_t useDetectorIndex) 
    {
      // Add number of pixels in X and Y from instrument definition

      std::vector<int64_t> dims;

      if(rectDetectors.size() != 0)
      {
        // Assume the first rect detector is the desired one.
        dims.push_back(rectDetectors[useDetectorIndex]->xpixels());
        dims.push_back(rectDetectors[useDetectorIndex]->ypixels());
      }
      else
      {
        // Incorrect workspace : requires the x/y pixel count from the instrument definition
        g_log.error("Unable to retrieve x and y pixel count from an instrument definition associated with this workspace.");
        throw std::runtime_error("Unable to retrieve x and y pixel count from an instrument definition associated with this workspace.");      
      }

      return dims;
    }

   /**
    * Run instead of exec when operating on groups
    */
    bool SaveNXTomo::processGroups()
    {
      try
      {            
        std::string name = getPropertyValue("InputWorkspaces");
        WorkspaceGroup_sptr groupWS = AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(name);     
        
        for(int i = 0; i<groupWS->getNumberOfEntries();++i)
        {
          m_workspaces.push_back(boost::dynamic_pointer_cast<Workspace2D>(groupWS->getItem(i)));
        }   
      }
      catch(...)
      {}

      if(m_workspaces.size() != 0)
        processAll();

      return true;
    }

  } // namespace DataHandling
} // namespace Mantid


