/*WIKI* 




The algorithm essentially copies the InputFilename into OutputFilename, except that it replaces the data field with whatever the specified workspace contains. The histograms do not need to be the same size (in number of bins), but the number of pixels needs to be the same.

In addition, this only works for instruments that use [[RectangularDetector]]s (SNAP, TOPAZ, POWGEN, for example); in addition, the name in the instrument definition file must match the name in the NXS file.






*WIKI*/
// SaveToSNSHistogramNexus
// @author Freddie Akeroyd, STFC ISIS Faility
// @author Ronald Fowler, STFC eScience. Modified to fit with SaveToSNSHistogramNexusProcessed
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/SaveToSNSHistogramNexus.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"
#include "MantidGeometry/IComponent.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/Timer.h"
#include "MantidKernel/Memory.h"
#include "MantidAPI/MemoryManager.h"
#include "MantidAPI/Progress.h"
#include "MantidAPI/FileProperty.h"

#include <cmath>
#include <numeric>
#include <boost/shared_ptr.hpp>
#include <boost/scoped_array.hpp>
#include <Poco/File.h>
//#include <hdf5.h> //This is troublesome on multiple platforms.

#include <stdlib.h>
#include <string.h>
#include <time.h>



namespace Mantid
{
namespace DataHandling
{

  // Register the algorithm into the algorithm factory
  DECLARE_ALGORITHM(SaveToSNSHistogramNexus)
  
  /// Sets documentation strings for this algorithm
  void SaveToSNSHistogramNexus::initDocs()
  {
    this->setWikiSummary(" Saves a workspace into SNS histogrammed NeXus format, using an original file as the starting point. This only works for instruments with Rectangular Detectors. ");
    this->setOptionalMessage("Saves a workspace into SNS histogrammed NeXus format, using an original file as the starting point. This only works for instruments with Rectangular Detectors.");
  }
  

  using namespace Kernel;
  using namespace API;
  using namespace DataObjects;
  using namespace Geometry;

  /// Empty default constructor
  SaveToSNSHistogramNexus::SaveToSNSHistogramNexus() : Algorithm() {}

  /** Initialisation method.
   *
   */
  void SaveToSNSHistogramNexus::init()
  {
    // Declare required parameters, filename with ext {.nx,.nx5,xml} and input workspac
    std::vector<std::string> exts;
    exts.push_back(".nxs");

    declareProperty(new FileProperty("InputFilename", "", FileProperty::Load, exts),
          "The name of the original Nexus file for this data,\n"
          "as a full or relative path");

    declareProperty(new WorkspaceProperty<MatrixWorkspace> ("InputWorkspace", "", Direction::Input),
        "Name of the workspace to be saved");

    declareProperty(new FileProperty("OutputFilename", "", FileProperty::Save, exts),
          "The name of the Nexus file to write, as a full or relative\n"
          "path");

    declareProperty(new PropertyWithValue<bool>("Compress", false, Direction::Input),
          "Will the output NXS file data be compressed?");

  }

  void myMalloc(::NeXus::Info& info, void **dataBuffer)
  {
    // can't figure out a nice way to replace the c-style malloc/free
    int rank = static_cast<int>(info.dims.size());
    int dims[rank];
    for (int i = 0; i < rank; ++i)
      dims[i] = static_cast<int>(info.dims[i]);
    if (NXmalloc (dataBuffer, rank, dims, info.type) != NX_OK)
      std::runtime_error("Failed to malloc data buffer");
  }

//  /** Execute the algorithm.
//   *
//   *  @throw runtime_error Thrown if algorithm cannot execute
//   */
//  void SaveToSNSHistogramNexus::exec()
//  {
//    throw std::runtime_error("Temporarily disabled because it does not work on RHEL5.\n");
//  }


  //------------------------------------------------------------------------
  /** Append to m_current_path */
  void SaveToSNSHistogramNexus::add_path(std::string& path)
  {
    m_current_path = m_current_path + "/" + path;
  }


  //------------------------------------------------------------------------
  /** Remove the last part of the path */
  void SaveToSNSHistogramNexus::remove_path(std::string& path)
  {
    size_t pos = path.rfind("/");
    m_current_path = m_current_path.substr(0, pos);

//    char *tstr;
//    tstr = strrchr(m_current_path, '/');
//    if (tstr != NULL && !strcmp(path, tstr+1))
//    {
//      *tstr = '\0';
//    }
//    else
//    {
//      printf("path error\n");
//    }
  }




  //------------------------------------------------------------------------
  /** Performs the copying from the input to the output file,
   *  while modifying the data and time_of_flight fields.
   */
  void SaveToSNSHistogramNexus::copy_file(const std::string &inFile, int nx_read_access, const std::string &outFile, int nx_write_access)
  {
    links_count = 0;
    m_current_path = "";
    NXlink link;

    // Open NeXus input and output files
    m_inHandle = new ::NeXus::File(inFile, nx_read_access);
    m_outHandle = new ::NeXus::File(outFile, nx_write_access);

    /* Output global attributes */
    WriteAttributes();

    /* Recursively cycle through the groups printing the contents */
    WriteGroup();

    /* close input */
    delete m_inHandle;
    m_inHandle = NULL;

    //HDF5 only
    if (nx_write_access == NXACC_CREATE5)
    {
      /* now create any required links */
      for(int i=0; i<links_count; i++)
      {
        m_outHandle->openPath(links_to_make[i].to);
        link = m_outHandle->getDataID();
        if (strlen(link.targetPath) == 0)
        {
          link = m_outHandle->getGroupID();
          if (strlen(link.targetPath) == 0)
            throw std::runtime_error("Failed to determine link information");
        }
        m_outHandle->openPath(links_to_make[i].from);
        if (boost::algorithm::ends_with(links_to_make[i].to, links_to_make[i].name))
        {
          m_outHandle->makeLink(link);
        }
        else
        {
          m_outHandle->makeNamedLink(links_to_make[i].name, link);
        }
      }
    }
    /* Close the input and output files */
    delete m_outHandle;
    m_outHandle = NULL;
  }

  //------------------------------------------------------------------------
  /** Utility function to write out the
   * data or errors to a field in the group.
   *
   * @param det :: rectangular detector being written
   * @param x_pixel_slab :: size of a slab to write, in number of X pixels. ignored if doBoth
   * @param field_name :: "data" field name
   * @param errors_field_name :: "errors" field name.
   * @param doErrors :: set true if you are writing the errors field this time. field_name should be the "errors" field name
   * @param doBoth :: do both data and errors at once, no slabbing.
   * @param is_definition ::
   * @param bank :: name of the bank being written.
   * @return error code
   */
  void SaveToSNSHistogramNexus::WriteOutDataOrErrors(Geometry::RectangularDetector_const_sptr det,
      int x_pixel_slab,
      const char * field_name, const char * errors_field_name,
      bool doErrors, bool doBoth,
      std::string bank)
  {
    const int dataRank(3);
    std::vector<int64_t> dims(dataRank);
    std::vector<int64_t> slabDims(dataRank);
    std::vector<int64_t> slabStartIndices(dataRank);

    // Dimension 0 = the X pixels
    dims[0] = det->xpixels();
    // Dimension 1 = the Y pixels
    dims[1] = det->ypixels();
    // Dimension 2 = time of flight bins
    dims[2] = static_cast<int>(inputWorkspace->blocksize());

    // ---- Determine slab size -----
    // Number of pixels to collect in X before slabbing
    slabDims[0] = x_pixel_slab;
    slabDims[1] = dims[1];
    slabDims[2] = dims[2];

    if (doBoth)
      slabDims[0] = dims[0];

    g_log.information() << "RectangularDetector " << det->getName()
                        << " being copied. Dimensions : " << dims[0] << ", " << dims[1] << ", " << dims[2] << ".\n";


    // ----- Open the data field -----------------------
    if (m_compress)
    {
      m_outHandle->makeCompData(field_name, ::NeXus::FLOAT32, dims, ::NeXus::LZW, slabDims, true);
    }
    else
    {
      m_outHandle->makeData(field_name, ::NeXus::FLOAT32, dims, true);
    }
    WriteAttributes();

    if (!doErrors)
    {
      // Add an attribute called "errors" with value = the name of the data_errors field.
      m_outHandle->putAttr("errors", errors_field_name);
    }

    // ---- Errors field -----
    if (doBoth)
    {
      m_outHandle->closeData();

      if (m_compress)
      {
        m_outHandle->makeCompData(errors_field_name, ::NeXus::FLOAT32, dims, ::NeXus::LZW, slabDims, true);
      }
      else
      {
        m_outHandle->makeData(errors_field_name, ::NeXus::FLOAT32, dims, true);
      }
      WriteAttributes();
      m_outHandle->closeData();
    }


    double fillTime = 0;
    double saveTime = 0;

    // Make a buffer of floats will all the counts in that bank.
    float * data;
    float * errors;
    data = new float[slabDims[0]*slabDims[1]*slabDims[2]];
    if (doBoth)
      errors = new float[slabDims[0]*slabDims[1]*slabDims[2]];

//    for (size_t i=0; i < slabDimensions[0]*slabDimensions[1]*slabDimensions[2]; i++)
//      data[i]=i;


    for (int x = 0; x < det->xpixels(); x++)
    {
      // Which slab are we in?
      int slabnum = x / x_pixel_slab;

      // X index into the slabbed output array
      int slabx = x % x_pixel_slab;

      Timer tim1;
      int ypixels = static_cast<int>(det->ypixels());

      PARALLEL_FOR1(inputWorkspace)
      for (int y = 0; y < ypixels; y++)
      {
        PARALLEL_START_INTERUPT_REGION
        //Get the workspace index for the detector ID at this spot
        size_t wi = 0;
        try
        {
          wi = (*map)[ det->getAtXY(x,y)->getID() ];
        }
        catch (...)
        {
          g_log.error() << "Error finding " << bank << " x " << x << " y " << y << "\n";
        }

        // Offset into array.
        size_t index = size_t(slabx)*size_t(dims[1])*size_t(dims[2]) + size_t(y)*size_t(dims[2]);

        const MantidVec & Y = inputWorkspace->readY(wi);
        const MantidVec & E = inputWorkspace->readE(wi);

        for ( size_t i = 0; i < Y.size(); ++i )
        {
          if ( doErrors )
          {
            data[i+index] = static_cast<float>(E[i]);
          }
          else
          {
            data[i+index] = static_cast<float>(Y[i]);
            if ( doBoth )
            {
              errors[i+index] = static_cast<float>(E[i]);
            }
          }
        }

        PARALLEL_END_INTERUPT_REGION
      }
      PARALLEL_CHECK_INTERUPT_REGION

      fillTime += tim1.elapsed();

      // Is this the last pixel in the slab?
      if (!doBoth && (x % x_pixel_slab == x_pixel_slab-1))
      {
        Timer tim2;
        //g_log.information() << "starting slab " << x << "\n";
        // This is where the slab is in the greater data array.
        slabStartIndices[0]=slabnum*x_pixel_slab;
        slabStartIndices[1]=0;
        slabStartIndices[2]=0;
        m_outHandle->putSlab(data, slabStartIndices, slabDims);
        saveTime += tim2.elapsed();

        std::ostringstream mess;
        mess << det->getName() << ", " << field_name << " slab " << slabnum << " of " << det->xpixels()/x_pixel_slab;
        this->prog->reportIncrement(x_pixel_slab*det->ypixels(), mess.str());
      }

    }// X loop

    if (doBoth)
    {
      Timer tim2;
      m_outHandle->openData(field_name);
      m_outHandle->putData(data);
      m_outHandle->closeData();
      this->prog->reportIncrement(det->xpixels()*det->ypixels()*1, det->getName() + " data");

      m_outHandle->openData(errors_field_name);
      m_outHandle->putData(errors);
      m_outHandle->closeData();
      this->prog->reportIncrement(det->xpixels()*det->ypixels()*1, det->getName() + " errors");
      saveTime += tim2.elapsed();
    }
    else
    {
      m_outHandle->closeData();
    }

    g_log.information() << "Filling out " << det->getName() << " took " << fillTime << " sec.\n";
    g_log.information() << "Saving      " << det->getName() << " took " << saveTime << " sec.\n";


    delete [] data;
    if (doBoth)
      delete [] errors;
  }



  //=================================================================================================
  /** Write the group labeled "data"
   *
   * @param bank :: name of the bank
   * @return error code
   */
  void SaveToSNSHistogramNexus::WriteDataGroup(std::string bank)
  {
    std::string name;
    void *dataBuffer;

    ::NeXus::Info info = m_inHandle->getInfo();

    // Get the rectangular detector
    IComponent_const_sptr det_comp = inputWorkspace->getInstrument()->getComponentByName( std::string(bank) );
    RectangularDetector_const_sptr det = boost::dynamic_pointer_cast<const RectangularDetector>(det_comp);
    if (!det)
    {
      g_log.information() << "Detector '" + bank + "' not found, or it is not a rectangular detector!\n";
      //Just copy that then.
      myMalloc(info, &dataBuffer);
      m_inHandle->getData(dataBuffer);
      m_outHandle->makeCompData(name, info.type, info.dims, ::NeXus::LZW, info.dims, true);
      WriteAttributes();
      m_outHandle->putData(dataBuffer);
      if (NXfree((void**)&dataBuffer) != NX_OK)
        throw std::runtime_error("Failed to free memory");
      m_outHandle->closeData();
    }
    else
    {
      //YES it is a rectangular detector.

      // --- Memory requirements ----
      size_t memory_required = size_t(det->xpixels()*det->ypixels())*size_t(inputWorkspace->blocksize())*2*sizeof(float);
      // Make sure you free as much memory as possible if you need a huge block.
      if (memory_required > 1000000000)
        API::MemoryManager::Instance().releaseFreeMemory();

      Kernel::MemoryStats mem;
      mem.update();
      size_t memory_available = mem.availMem()*1024;

      g_log.information() << "Memory available: " << memory_available/1024 << " kb. ";
      g_log.information() << "Memory required: " << memory_required/1024 << " kb. ";

      // Give a 50% margin of error in allocating the memory
      memory_available = memory_available/2;
      if (memory_available > (size_t)5e9) memory_available = (size_t)5e9;

      if (memory_available < memory_required)
      {
        // Compute how large of a slab you can still use.
        int x_slab;
        x_slab = static_cast<int>(memory_available/(det->ypixels()*inputWorkspace->blocksize()*2*sizeof(float)));
        if (x_slab <= 0) x_slab = 1;
        // Look for a slab size that evenly divides the # of pixels.
        while (x_slab > 1)
        {
          if ((det->xpixels() % x_slab) == 0) break;
          x_slab--;
        }

        g_log.information() << "Saving in slabs of " << x_slab << " X pixels.\n";
        this->WriteOutDataOrErrors(det, x_slab, "data", "data_errors", false, false, bank);
        this->WriteOutDataOrErrors(det, x_slab, "errors", "", true, false, bank);
      }
      else
      {
        g_log.information() << "Saving in one block.\n";
        this->WriteOutDataOrErrors(det, det->xpixels(), "data", "data_errors", false, true, bank);
      }

    }
  }

  //------------------------------------------------------------------------
  /** Prints the contents of each group as XML tags and values */
  void SaveToSNSHistogramNexus::WriteGroup()
  {
    std::string name, theClass;
    void *dataBuffer;
    NXlink link;

    std::map<std::string, std::string> entries = m_inHandle->getEntries();
    for (auto entry = entries.begin(); entry != entries.end(); ++entry)
    {
      name = entry->first;
      theClass = entry->second;

      if (boost::algorithm::starts_with(theClass, "NX"))
      {
        m_inHandle->openGroup(name, theClass);
        add_path(name);

        link = m_inHandle->getGroupID();
        if (m_current_path == link.targetPath)
        {
          // Create a copy of the group
          m_outHandle->makeGroup(name, theClass, true);
          WriteAttributes();
          WriteGroup();
          remove_path(name);
        }
        else
        {
          remove_path(name);
          links_to_make[links_count].from = m_current_path;
          links_to_make[links_count].to = link.targetPath;
          links_to_make[links_count].name = name;
          links_count++;
          m_inHandle->closeGroup();
        }
      }
      else if (theClass == "SDS")
        //        else if (!strncmp(theClass,"SDS",3))
      {
        add_path(name);
        m_inHandle->openData(name);
        link = m_inHandle->getDataID();

        std::string data_label(name);

        if (m_current_path == link.targetPath)
        {
          // Look for the bank name
          std::string path(m_current_path);
          std::string bank("");

          size_t a = path.rfind('/');
          if (a != std::string::npos && a > 0)
          {
            size_t b = path.rfind('/',a-1);
            if (b != std::string::npos && (b < a) && (a-b-1) > 0)
            {
              bank = path.substr(b+1,a-b-1);
              //g_log.information() << m_current_path << ":bank " << bank << "\n";
            }
          }

          //---------------------------------------------------------------------------------------
          if (data_label=="data" && (bank != ""))
          {
            this->WriteDataGroup(bank);
          }
          //---------------------------------------------------------------------------------------
          else if (data_label=="time_of_flight" && (bank != ""))
          {
            // Get the original info
            ::NeXus::Info info = m_inHandle->getInfo();

            // Get the X bins
            const MantidVec & X = inputWorkspace->readX(0);
            std::vector<float> tof_data(X.begin(), X.end());

            // And fill it with the X data
            for (size_t i=0; i < X.size(); i++)
              tof_data[i] = float(X[i]);

            std::vector<int64_t> dims(1, tof_data.size());
            m_outHandle->writeCompData(name, tof_data, dims, ::NeXus::LZW, dims);
            m_outHandle->openData(name);
            WriteAttributes();
            m_outHandle->closeData();

          }

          //---------------------------------------------------------------------------------------
          else
          {
            //Everything else gets copies
            ::NeXus::Info info = m_inHandle->getInfo();
            myMalloc(info, &dataBuffer);

            m_inHandle->getData(dataBuffer);
            m_outHandle->makeCompData(name, info.type, info.dims, ::NeXus::LZW, info.dims, true);
            m_outHandle->putData(dataBuffer);
            if (NXfree((void**)&dataBuffer) != NX_OK)
              std::runtime_error("Failed to malloc data buffer");
            WriteAttributes();
            m_outHandle->closeData();
          }

          remove_path(name);
        }
        else
        {
          //Make a link
          remove_path(name);
          links_to_make[links_count].from = m_current_path;
          links_to_make[links_count].to = link.targetPath;
          links_to_make[links_count].name = name;
          links_count++;
        }
        m_inHandle->closeData();
      }
    }
  }



  //------------------------------------------------------------------------
  /** Copy the attributes from input to output */
  void SaveToSNSHistogramNexus::WriteAttributes()
  {
    std::vector<AttrInfo> attrs = m_inHandle->getAttrInfos();
    for (auto attr = attrs.begin(); attr!= attrs.end(); ++attr)
    {
      if (attr->name == "NeXus_version")
        continue;
      if (attr->name == "XML_version")
        continue;
      if (attr->name == "HDF_version")
        continue;
      if (attr->name == "HDF5_Version")
        continue;
      if (attr->name == "file_name")
        continue;
      if (attr->name == "file_time")
        continue;
      std::string temp = m_inHandle->getStrAttr(*attr);
      m_outHandle->putAttr(attr->name, temp);
    }
  }


  void nexus_print_error(void *pD, char *text)
  {
    (void) pD;
    std::cout << "Nexus Error: " << text << "\n";
  }



  //------------------------------------------------------------------------
  /** Execute the algorithm.
   *
   *  @throw runtime_error Thrown if algorithm cannot execute
   */
  void SaveToSNSHistogramNexus::exec()
  {
    //NXMSetError(NULL, nexus_print_error);
    NXMEnableErrorReporting();

    // Retrieve the filename from the properties
    m_inputFilename = getPropertyValue("InputFileName");
    m_outputFilename = getPropertyValue("OutputFileName");
    m_compress = getProperty("Compress");

    inputWorkspace = getProperty("InputWorkspace");

    // We'll need to get workspace indices
    map = inputWorkspace->getDetectorIDToWorkspaceIndexMap( false );

    // Start the progress bar. 3 reports per histogram.
    prog = new Progress(this, 0, 1.0, inputWorkspace->getNumberHistograms()*3);

    EventWorkspace_const_sptr eventWorkspace = boost::dynamic_pointer_cast<const EventWorkspace>(inputWorkspace);
    if (eventWorkspace)
    {
      eventWorkspace->sortAll(TOF_SORT, prog);
    }

    this->copy_file(m_inputFilename.c_str(),  NXACC_READ, m_outputFilename.c_str(),  NXACC_CREATE5);

    // Free map memory
    delete map;

    return;
  }


} // namespace NeXus
} // namespace Mantid
