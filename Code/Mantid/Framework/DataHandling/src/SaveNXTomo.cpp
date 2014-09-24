#include "MantidAPI/FileProperty.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidDataHandling/FindDetectorsPar.h"
#include "MantidDataHandling/SaveNXTomo.h"
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

    const double SaveNXTomo::MASK_FLAG = std::numeric_limits<double>::quiet_NaN();
    const double SaveNXTomo::MASK_ERROR = 0.0;
    const std::string SaveNXTomo::NXTOMO_VER = "2.0";

    SaveNXTomo::SaveNXTomo() :  API::Algorithm()
    {
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

      declareProperty(new WorkspaceProperty<MatrixWorkspace> ("InputWorkspace",
          "", Direction::Input, wsValidator),
          "The name of the workspace to save.");

      declareProperty(new API::FileProperty("Filename", "", FileProperty::Save, std::vector<std::string>(1,".nxs")),
          "The name of the NXTomo file to write, as a full or relative path");
    }

    /**
     * Execute the algorithm
     */
    void SaveNXTomo::exec()
    {
      // Retrieve the input workspace
      const MatrixWorkspace_const_sptr inputWS = getProperty("InputWorkspace");
            
      const std::string workspaceID = inputWS->id();
      
      if ((workspaceID.find("Workspace2D") == std::string::npos) &&
        (workspaceID.find("RebinnedOutput") == std::string::npos)) 
          throw Exception::NotImplementedError("SaveNexusProcessed passed invalid workspaces. Must be Workspace2D, EventWorkspace, ITableWorkspace, or OffsetsWorkspace.");
      
      // Do the full check for common binning
      if (!WorkspaceHelpers::commonBoundaries(inputWS))
      {
        g_log.error("The input workspace must have common bins");
        throw std::invalid_argument("The input workspace must have common bins");
      }

      // Number of spectra
      const int nHist = static_cast<int>(inputWS->getNumberHistograms());
      // Number of energy bins
      this->m_nBins = inputWS->blocksize();

      // Get a pointer to the sample
      Geometry::IComponent_const_sptr sample =
          inputWS->getInstrument()->getSample();

      // Retrieve the filename from the properties
      this->m_filename = getPropertyValue("Filename");

      // Create some arrays for the nexus api to use
      std::vector<int> array_dims;
      array_dims.push_back((int)nHist);
      array_dims.push_back((int)m_nBins);

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
      std::vector<double> imageKey(array_dims[1],0);
      nxFile.writeData("image_key", imageKey);
      nxFile.closeGroup();

      // source group // from diamond file contains {current,energy,name,probe,type} - probe = neutron | x-ray | electron
      
      
      nxFile.closeGroup(); // NXinstrument

      // ******************************************
      // NXsample
      nxFile.makeGroup("sample", "NXsample", true);
      // TODO: Write sample info
      // name
      // nxFile.writeData("rotation_angle", 0.0);
      // x_translation
      // y_translation
      // z_translation
      nxFile.closeGroup(); // NXsample
      
      // ******************************************
      // Make the NXmonitor group - Holds base beam intensity for each image
      // If information is not present, set as 1

      std::vector<double> intensity(array_dims[1],1);   
      nxFile.makeGroup("control", "NXmonitor", true);         
      nxFile.writeData("data", intensity);
      nxFile.closeGroup(); // NXmonitor          

      // ******************************************
      // Make the NXdata group
      nxFile.makeGroup("data", "NXdata", true);

      // Energy bins
      // Get the Energy Axis (X) of the first spectra (they are all the same - checked above)
      const MantidVec& X = inputWS->readX(0);
      nxFile.writeData("energy", X);
      nxFile.openData("energy");
      nxFile.putAttr("units", "meV");
      nxFile.closeData();  

      nxFile.makeData("data", ::NeXus::FLOAT64, array_dims, false);
      nxFile.makeData("error", ::NeXus::FLOAT64, array_dims, false);

      // Add the axes attributes to the data
      //nxFile.openData("data");
      //nxFile.putAttr("signal", 1);
      //nxFile.putAttr("axes", "polar:energy");
      //nxFile.closeData();

      std::vector<int64_t> slab_start;
      std::vector<int64_t> slab_size;

      // What size slabs are we going to write...
      slab_size.push_back(1);
      slab_size.push_back((int64_t)m_nBins);

      // And let's start at the beginning
      slab_start.push_back(0);
      slab_start.push_back(0);

      // define the data and error vectors for masked detectors
      std::vector<double> masked_data (m_nBins, MASK_FLAG);
      std::vector<double> masked_error (m_nBins, MASK_ERROR);
       
      // Create a progress reporting object
      Progress progress(this,0,1,100);
      const int progStep = (int)(ceil(nHist/100.0));
      Geometry::IDetector_const_sptr det;
      // Loop over spectra
      for (int i = 0; i < nHist; i++)
      {
        try
        {  // detector exist
          det = inputWS->getDetector(i);
          // Check that we aren't writing a monitor...
          if (!det->isMonitor())
          {
            Geometry::IDetector_const_sptr det = inputWS->getDetector(i);

            if (!det->isMasked())
            {
              // no masking...
              // Open the data
              nxFile.openData("data");
              slab_start[0] = i;
              nxFile.putSlab(const_cast<MantidVec&> (inputWS->readY(i)),
                  slab_start, slab_size);
              // Close the data
              nxFile.closeData();

              // Open the error
              nxFile.openData("error");
              //MantidVec& tmparr = const_cast<MantidVec&>(inputWS->dataE(i));
              //nxFile.putSlab((void*)(&(tmparr[0])), slab_start, slab_size);
              nxFile.putSlab(const_cast<MantidVec&> (inputWS->readE(i)),
                  slab_start, slab_size);
              // Close the error
              nxFile.closeData();
            }
            else
            {
              // Write a masked value...
              // Open the data
              nxFile.openData("data");
              slab_start[0] = i;
              nxFile.putSlab(masked_data, slab_start, slab_size);
              // Close the data
              nxFile.closeData();

              // Open the error
              nxFile.openData("error");
              nxFile.putSlab(masked_error, slab_start, slab_size);
              // Close the error
              nxFile.closeData();
            }
          }
        }
        catch(Exception::NotFoundError&)
        {
          // Catch if no detector. Next line tests whether this happened - test placed
          // outside here because Mac Intel compiler doesn't like 'continue' in a catch
          // in an openmp block.
        }
        // If no detector found, skip onto the next spectrum
        if ( !det ) continue;

        // make regular progress reports and check for canceling the algorithm
        if ( i % progStep == 0 )
        {
          progress.report();
        }
      }
      // execute the ChildAlgorithm to calculate the detector's parameters;
     /* IAlgorithm_sptr   spCalcDetPar = this->createChildAlgorithm("FindDetectorsPar", 0, 1, true, 1);

      spCalcDetPar->initialize();
      spCalcDetPar->setPropertyValue("InputWorkspace", inputWS->getName());
      std::string parFileName = this->getPropertyValue("ParFile");
      if(!(parFileName.empty()||parFileName=="not_used.par")){
          spCalcDetPar->setPropertyValue("ParFile",parFileName);
      }
      spCalcDetPar->execute();*/

      //
      //FindDetectorsPar * pCalcDetPar = dynamic_cast<FindDetectorsPar *>(spCalcDetPar.get());
      //if(!pCalcDetPar){	  // "can not get pointer to FindDetectorsPar algorithm"
      //    throw(std::bad_cast());
      //}
   /*   const std::vector<double> & azimuthal           = pCalcDetPar->getAzimuthal();
      const std::vector<double> & polar               = pCalcDetPar->getPolar();
      const std::vector<double> & azimuthal_width     = pCalcDetPar->getAzimWidth();
      const std::vector<double> & polar_width         = pCalcDetPar->getPolarWidth();
      const std::vector<double> & secondary_flightpath= pCalcDetPar->getFlightPath();*/


      //// Write the Polar (2Theta) angles
      //nxFile.writeData("polar", polar);

      //// Write the Azimuthal (phi) angles
      //nxFile.writeData("azimuthal", azimuthal);

      //// Now the widths...
      //nxFile.writeData("polar_width", polar_width);
      //nxFile.writeData("azimuthal_width", azimuthal_width);

      //// Secondary flight path
      //nxFile.writeData("distance", secondary_flightpath);

      nxFile.closeGroup(); // NXdata

      nxFile.closeGroup(); // tomo_entry sub-group

      nxFile.closeGroup(); // Top level NXentry

      // Validate the file against the schema

    }

    //void someRoutineToAddDataToExisting()
    //{
    //  //TODO:
    //}

  } // namespace DataHandling
} // namespace Mantid


// TODO: don't allow a multi-file
// Follow mtd conventions
// Add comments / function descriptions etc