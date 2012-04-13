/*WIKI* 


The algorithm takes every pixel defined in the SQW horace file and converts it into an event. SQW DND/Image data is used to format dimension, with which to work.After the algorithm completes a fully formed [[MDEventWorkspace]] is provided.
If the OutputWorkspace does NOT already exist, a default one is created. This is not the only route to generating MDEventWorkspaces.


*WIKI*/
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/MemoryManager.h"
#include "MantidAPI/Progress.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidGeometry/MDGeometry/MDHistoDimensionBuilder.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidKernel/CPUTimer.h"
#include "MantidKernel/DiskBuffer.h"
#include "MantidKernel/Matrix.h"
#include "MantidKernel/PropertyWithValue.h"
#include "MantidKernel/ThreadPool.h"
#include "MantidKernel/ThreadScheduler.h"
#include "MantidMDEvents/LoadSQW.h"
#include <iostream>
#include "MantidMDEvents/MDBox.h"
#include "MantidKernel/Memory.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;
using Mantid::Geometry::OrientedLattice;

namespace Mantid
{
  namespace MDEvents
  {

    DECLARE_ALGORITHM(LoadSQW)

    /// Constructor
    LoadSQW::LoadSQW()
    : m_prog(new Mantid::API::Progress(this, 0.05, 0.95, 100))
    {
    }

    /// Destructor
    LoadSQW::~LoadSQW()
    {
      delete m_prog;
    }

    /// Provide wiki documentation.
    void LoadSQW::initDocs()
    {
      this->setWikiSummary("Create a MDEventWorkspace with events in reciprocal space (Qx, Qy, Qz, Energy) from a SQW file.");
      this->setOptionalMessage("Create a MDEventWorkspace with events in reciprocal space (Qx, Qy, Qz, Energy) from a SQW file.");
    }

    /// Initalize the algorithm
    void LoadSQW::init()
    {
      std::vector<std::string> fileExtensions(1);
      fileExtensions[0]=".sqw";
      declareProperty(new API::FileProperty("Filename","", API::FileProperty::Load,fileExtensions), "File of type SQW format");
      declareProperty(new API::WorkspaceProperty<API::IMDEventWorkspace>("OutputWorkspace","", Kernel::Direction::Output),
        "Name of the output MDEventWorkspace that will contain the SQW data read in.");
      declareProperty(new Kernel::PropertyWithValue<bool>("MetadataOnly", false),
        "Load Metadata without events.");
      std::vector<std::string> fileExtensions2(1);
      fileExtensions[0]=".nxs";
      declareProperty(new API::FileProperty("OutputFilename","", API::FileProperty::OptionalSave, fileExtensions2),
          "If the input SQW file is too large to fit in memory, specify an output NXS file.\n"
          "The MDEventWorkspace will be create with this file as its back-end.");
    }



    //------------------------------------------------------------------------------------------------
    /// Execute the algorithm
    void LoadSQW::exec()
    {
      std::string filename = getProperty("Filename");
      m_fileStream.open(filename.c_str(), std::ios::binary);

      // Parse Extract metadata. Including data locations.
      parseMetadata();

      // Create a new output workspace.
      MDEventWorkspace<MDEvent<4>,4>* pWs = new MDEventWorkspace<MDEvent<4>,4>;
      Mantid::API::IMDEventWorkspace_sptr ws(pWs);

      // Add dimensions onto workspace.
      addDimensions(pWs);
      
      // Set some reasonable values for the box controller
      BoxController_sptr bc = pWs->getBoxController();
      bc->setSplitInto(3);
      bc->setSplitThreshold(3000);

      // Initialize the workspace.
      pWs->initialize();

      // Add oriented lattice.
      addLattice(pWs);

      // Start with a MDGridBox.
      pWs->splitBox();

      // Save the empty WS and turn it into a file-backed MDEventWorkspace (on option)
      m_outputFile = getPropertyValue("OutputFilename");

      if (!m_outputFile.empty())
      {
        MemoryStats stat;
        if ((m_nDataPoints * sizeof(MDEvent<4>) * 2 / 1024) < stat.availMem())
          g_log.notice() << "You have enough memory available to load the " << m_nDataPoints << " points into memory; this would be faster than using a file back-end." << std::endl;

        IAlgorithm_sptr saver = this->createSubAlgorithm("SaveMD" ,0.01, 0.05, true);
        saver->setProperty("InputWorkspace", ws);
        saver->setPropertyValue("Filename", m_outputFile);
        saver->setProperty("MakeFileBacked", true);
        saver->executeAsSubAlg();
        m_prog->resetNumSteps(100, 0.05, 0.75);
      }
      else
      {
        MemoryStats stat;
        if ( (size_t(double(m_nDataPoints) * 1.5) * sizeof(MDEvent<4>) / 1024) > stat.availMem()  )
          g_log.warning() << "You may not have enough physical memory available to load the " << m_nDataPoints << " points into memory. You can cancel and specify OutputFilename to load to a file back-end." << std::endl;
      }

      bc = pWs->getBoxController();
      bc->setCacheParameters( sizeof(MDEvent<4>), 1000000);
      std::cout << "File backed? " << bc->isFileBacked() << ". Cache " << bc->getDiskBuffer().getMemoryStr() << std::endl;

      //Persist the workspace.
      API::IMDEventWorkspace_sptr i_out = getProperty("OutputWorkspace");
      if(i_out)
        throw std::runtime_error("Cannot currently handle overwriting of an existing workspace.");
      setProperty("OutputWorkspace", ws);

      // Read events into the workspace.
      bool bMetadataOnly = getProperty("MetadataOnly");
      if(!bMetadataOnly)
        addEvents(pWs);

      progress(m_outputFile.empty() ? 0.96 : 0.76, "Refreshing cache");
      pWs->refreshCache();

      if (!m_outputFile.empty())
      {
        g_log.notice() << "Starting SaveMD to update the file back-end." << std::endl;
        IAlgorithm_sptr saver = this->createSubAlgorithm("SaveMD" ,0.76, 1.00);
        saver->setProperty("InputWorkspace", ws);
        saver->setProperty("UpdateFileBackEnd", true);
        saver->executeAsSubAlg();
      }
    }




    /// Add events after reading pixels/datapoints from file.
    void LoadSQW::addEvents(Mantid::MDEvents::MDEventWorkspace<MDEvent<4>,4>* ws)
    {
      CPUTimer tim;

      size_t maxNPix = ~size_t(0);
      if(m_nDataPoints > maxNPix)
      {
        throw new std::runtime_error("Not possible to fit all datapoints into memory");
      }

      const size_t ncolumns = 9; //qx, qy, qz, en, s, err, irunid, idetid, ien
      const size_t column_size = 4; //types stored as either float32 or unsigned integer (both 4byte).
      const size_t column_size_2 = column_size * 2; //offset, gives qz
      const size_t column_size_3 = column_size * 3; //offset, gives en
      const size_t column_size_4 = column_size * 4; //offset, gives idet
      //const size_t column_size_5 = column_size * 5; //offset, gives ien
      const size_t column_size_6 = column_size * 6; //offset, gives irun
      const size_t column_size_7 = column_size * 7; //offset, gives signal
      const size_t column_size_8 = column_size * 8; //offset, gives error

      const size_t pixel_width = ncolumns * column_size;
      const size_t data_buffer_size = pixel_width * m_nDataPoints;
      g_log.information() << m_nDataPoints << " data points in this SQW file." << std::endl;

      // Load from the input file is smallish blocks
      size_t blockSize = pixel_width * 1000000;

      // Report progress once per block
      int numBlocks = int((data_buffer_size + blockSize-1) / blockSize) ;
      m_prog->setNumSteps( numBlocks );
      m_prog->setNotifyStep(0.1);

      // For tracking when to split boxes
      size_t eventsAdded = 0;
      BoxController_sptr bc = ws->getBoxController();
      DiskBuffer & dbuf = bc->getDiskBuffer();

      for (int blockNum=0; blockNum < numBlocks; blockNum++)
      {
        // Start point in the file
        size_t inputFileOffset = size_t(blockNum) * blockSize;

        // Limit the size of the block
        size_t currentBlockSize = blockSize;
        if ((inputFileOffset + currentBlockSize) > data_buffer_size)
          currentBlockSize = data_buffer_size - inputFileOffset;

        // Load the block from the file
        std::vector<char> vecBuffer = std::vector<char>(currentBlockSize);
        char* pData = &vecBuffer[0];
        this->m_fileStream.seekg(this->m_dataPositions.pix_start + inputFileOffset, std::ios::beg);
        this->m_fileStream.read(pData,currentBlockSize);

        // Go through each pixel in the input
        int currentNumPixels = int(currentBlockSize / pixel_width);
        eventsAdded += size_t(currentNumPixels);

        // Add the events in parallel
        PARALLEL_FOR_NO_WSP_CHECK()
        for (int i=0; i < currentNumPixels; i++)
        {
          size_t current_pix = size_t(i*pixel_width);
          coord_t centers[4] =        //for(size_t current_pix = 0; current_pix < currentBlockSize; current_pix += pixel_width)

          {
              *(reinterpret_cast<float*>(pData + current_pix)),
              *(reinterpret_cast<float*>(pData + current_pix + column_size)),
              *(reinterpret_cast<float*>(pData + current_pix + column_size_2)),
              *(reinterpret_cast<float*>(pData + current_pix + column_size_3))
          };
          float error = *reinterpret_cast<float*>(pData + current_pix + column_size_8);
          ws->addEvent(MDEvent<4>( 
            *reinterpret_cast<float*>(pData + current_pix + column_size_7),     // Signal
            error*error,                                                        // Error sq 
            *reinterpret_cast<uint16_t*>(pData + current_pix + column_size_6),  // run Index
            *reinterpret_cast<uint32_t*>(pData + current_pix + column_size_4),  // Detector Id
            centers));
        }


//        MemoryStats stat;
//        size_t bytesAvail = stat.availMem() * 1024;
//        // Estimate how many extra bytes will (temporarily) be used when splitting events
//        size_t bytesNeededToSplit = eventsAdded * sizeof(MDEvent<4>) / 2;

        // Split:
        // 1. When < 1 GB of memory is free
        // 2. When too little memory might be available for the splitting operation
        // 3. When enough events have been added that it makes sense
        // 4. At the last block being added


//        if ((bytesAvail < 1000000000) || (bytesAvail < bytesNeededToSplit) ||
//            bc->shouldSplitBoxes(eventsAdded*2, lastNumBoxes) || (blockNum == numBlocks-1) )


        if (eventsAdded > 19000000 )
        {
          g_log.information() << "Splitting boxes after " << eventsAdded << " events added." << std::endl;
          Mantid::API::MemoryManager::Instance().releaseFreeMemory();

          // This splits up all the boxes according to split thresholds and sizes.
          Kernel::ThreadScheduler * ts = new ThreadSchedulerFIFO();
          ThreadPool tp(ts);
          ws->splitAllIfNeeded(ts);
          tp.joinAll();

          // Flush the cache - this will save things out to disk
          dbuf.flushCache();
          // Flush memory
          Mantid::API::MemoryManager::Instance().releaseFreeMemory();
          eventsAdded = 0;
        }

//
//        if (false)
//        {
//          std::vector<MDBoxBase<MDEvent<4>,4>*> boxes;
//          ws->getBox()->getBoxes(boxes, 100, true);
//          size_t modified = 0;
//          size_t inmem = 0;
//          size_t ondisk = 0;
//          size_t events = 0;
//          for (size_t i=0; i<boxes.size(); i++)
//          {
//            MDBox<MDEvent<4>,4>* box = dynamic_cast<MDBox<MDEvent<4>,4>*>(boxes[i]);
//            if (box)
//            {
//              //box->save();
//              if (box->dataAdded() || box->dataModified())
//                modified++;
//              if (box->getInMemory())
//                inmem++;
//              if (box->getOnDisk())
//                ondisk++;
//              events += box->getEventVectorSize();
//            }
//          }
//          g_log.information() << modified << " of " << boxes.size() << " MDBoxes have data added or modified." << std::endl;
//          g_log.information() << inmem << " MDBoxes are in memory." << std::endl;
//          //g_log.information() << ondisk << " MDBoxes are on disk." << std::endl;
//          g_log.information() << double(events)/1e6 << " million events in memory." << std::endl;
//        }

        // Report progress once per block.
        m_prog->report();
      }
    }

    /**
    Extract the b-matrix from a SQW file. Create experiment info with oriented lattice and add to workspace.
    @param ws : Workspace to modify.
    */
    void LoadSQW::addLattice(Mantid::MDEvents::MDEventWorkspace<MDEvent<4>,4>* ws)
    {
      std::vector<char> buf(4*(3+3)); //Where 4 = size_of(float) and 3 * 3 is size of b-matrix.
      this->m_fileStream.seekg(this->m_dataPositions.geom_start, std::ios::beg);
      this->m_fileStream.read(&buf[0],buf.size());

      double a = (double)(*((float *)(&buf[0])));
      double b = (double)(*((float *)(&buf[4])));
      double c = (double)(*((float *)(&buf[8])));
      double aa = (double)(*((float *)(&buf[12])));
      double bb = (double)(*((float *)(&buf[16])));
      double cc = (double)(*((float *)(&buf[20])));
      
      ExperimentInfo_sptr info(new ExperimentInfo());
      info->mutableSample().setOrientedLattice(new OrientedLattice(a,b,c,aa,bb,cc));
      ws->addExperimentInfo(info);
    }


    /// Add a dimension after reading info from file.
    void LoadSQW::addDimensions(Mantid::MDEvents::MDEventWorkspace<MDEvent<4>,4>* ws)
    {
      using Mantid::Geometry::MDHistoDimensionBuilder;
      Mantid::Geometry::Vec_MDHistoDimensionBuilder dimensionVec(4);
      MDHistoDimensionBuilder& qx = dimensionVec[0];
      MDHistoDimensionBuilder& qy = dimensionVec[1];
      MDHistoDimensionBuilder& qz = dimensionVec[2];
      MDHistoDimensionBuilder& en = dimensionVec[3];

      //Horace Tags.
      qx.setId("qx");
      qx.setUnits("A^(-1)");
      qy.setId("qy");
      qy.setUnits("A^(-1)");
      qz.setId("qz");
      qz.setUnits("A^(-1)");
      en.setId("en");
      en.setUnits("MeV");

      std::vector<char> buf(4*(3+3+4+16+4+2));
      this->m_fileStream.seekg(this->m_dataPositions.geom_start, std::ios::beg);

      this->m_fileStream.read(&buf[0],buf.size());
      // skip allat and adlngldef
      size_t i0 = 4*(3+3) ; 
      for(size_t i=0;i<this->m_nDims;i++){
        //double val = (double)*((float*)(&buf[i0+i*4]));
        //dscrptn.pDimDescription(i)->data_shift = val;
      }

      //TODO: how to use it in our framework?
      std::vector<double> u_to_Rlu(this->m_nDims*this->m_nDims);
      i0 += this->m_nDims*4;
      // [data.u_to_rlu, count, ok, mess] = fread_catch(fid,[4,4],'float32'); if ~all(ok); return; end;
      size_t ic = 0;
      for(size_t i=0;i<this->m_nDims;i++){
        for(size_t j=0;j<this->m_nDims;j++){
          u_to_Rlu[ic]=(double)*((float*)(&buf[i0+4*(i*4+j)]));
          ic++;
        }
      }
      i0 += ic*4;
      Mantid::Kernel::DblMatrix UEmat(u_to_Rlu);
      Mantid::Kernel::DblMatrix Rot(3,3);
      for(int i=0;i<3;i++){
        for(int j=0;j<3;j++){
          Rot[i][j]=UEmat[i][j];
        }
      }
      //dscrptn.setRotationMatrix(Rot);

      // axis labels size 
      i0 += m_nDims*4;
      unsigned int nRows = *((uint32_t*)(&buf[i0]));
      unsigned int nCols = *((uint32_t*)(&buf[i0+4]));


      // read axis labelsg
      buf.resize(nRows*nCols);
      // [ulabel, count, ok, mess] = fread_catch(fid,[n(1),n(2)],'*char'); if ~all(ok); return; end;

      this->m_fileStream.read(&buf[0],buf.size());

      //data.ulabel=cellstr(ulabel)';
      std::string name;
      char symb;
      name.resize(nCols);
      for(unsigned int i=0;i<nRows;i++)
      {
        for(unsigned int j=0;j<nCols;j++)
        {
          symb   =buf[i+j*nRows]; 
          name[j] =symb;  
        }
        //Trim string.
        std::string sName(name);
        boost::erase_all(sName, " ");

        dimensionVec[i].setName(sName);
      }


      // resize for iax and npax;
      buf.resize(4*4*3);
      this->m_fileStream.read(&buf[0],4);

      unsigned int npax =  *((uint32_t*)(&buf[0]));
      unsigned int niax = 4-npax;

      if(niax>0){

        this->m_fileStream.read(&buf[0],buf.size());
        int i_axis_index;
        for(unsigned int i=0;i<niax;i++){
          i_axis_index = *((uint32_t*)(&buf[i*4]));

          dimensionVec[i_axis_index].setNumBins(1);
          dimensionVec[i_axis_index].setMax(*((float*)(&buf[4*(niax+i*2+1)])));
          dimensionVec[i_axis_index].setMin(*((float*)(&buf[4*(niax+i*2)])));
        }

      }
      // processing projection axis;
      if(npax>0){
        this->m_fileStream.read(&buf[0],4*npax);

        for(unsigned int i=0;i<npax;i++){

          std::vector<char> axis_buffer(51*4);
          this->m_fileStream.read(&axis_buffer[0],4);
          unsigned int  nAxisPoints = *((uint32_t*)(&axis_buffer[0]));
          if(axis_buffer.size()<nAxisPoints*4)axis_buffer.resize(nAxisPoints*4);
          this->m_fileStream.read(&axis_buffer[0],4*nAxisPoints);

          dimensionVec[i].setNumBins(nAxisPoints-1);
          dimensionVec[i].setMax(*((float*)(&axis_buffer[4*(nAxisPoints-1)])));
          dimensionVec[i].setMin(*((float*)(&axis_buffer[4*(0)])));

        }
      }

      //Add dimensions to the workspace by invoking the dimension builders.
      ws->addDimension(qx.create());
      ws->addDimension(qy.create());
      ws->addDimension(qz.create());
      ws->addDimension(en.create());

//      std::cout << qx.create()->getNBins() << " bins in x\n";
//      std::cout << qy.create()->getNBins() << " bins in y\n";
//      std::cout << qz.create()->getNBins() << " bins in z\n";
//      std::cout << en.create()->getNBins() << " bins in en\n";

    }


    /*==================================================================================
    Region: Functions in the following region are candidates for refactoring. Copied from MD_FileHoraceReader
    ==================================================================================*/

    /// Function provides seam on to access auxillary functions ported from MD_FileHoraceReader.
    void LoadSQW::parseMetadata()
    {
      std::vector<char> data_buffer;
      m_fileStream.seekg(m_dataPositions.if_sqw_start);
      data_buffer.resize(3*4);

      m_fileStream.read(&data_buffer[0],2*4);
      this->m_nDims = *((uint32_t*)(&data_buffer[4]));

      parse_sqw_main_header();

      // go through all component headers and read them (or calculate their length)
      std::streamoff next_position = m_dataPositions.component_headers_starts[0];
      size_t nFiles        = m_dataPositions.component_headers_starts.size();
      for(size_t i=0;i<nFiles;i++){
        m_dataPositions.component_headers_starts[i] = next_position;
        next_position = parse_component_header(next_position);
      }
      m_dataPositions.detectors_start = next_position;
      // get detectors
      m_dataPositions.data_start      = parse_sqw_detpar(m_dataPositions.detectors_start);
      // calculate all other data fields locations;
      parse_data_locations(m_dataPositions.data_start);
    }

    // auxiliary functions
    void LoadSQW::parse_sqw_main_header()
    { // we do not need this header  at the moment -> just need to calculated its length;

      std::vector<char> data_buffer(4 * 3);
      this->m_fileStream.read(&data_buffer[0], 4);

      unsigned int file_name_length = *((uint32_t*) (&data_buffer[0]));
      //skip main header file name
      m_fileStream.seekg(file_name_length, std::ios_base::cur);

      this->m_fileStream.read(&data_buffer[0], 4);             
      unsigned int file_path_length = *((uint32_t*) (&data_buffer[0]));


      //skip main header file path
      m_fileStream.seekg(file_path_length, std::ios_base::cur);    

      this->m_fileStream.read(&data_buffer[0], 4);
      unsigned int file_title = *((uint32_t*) (&data_buffer[0]));     

      //skip main header file path
      m_fileStream.seekg(file_title, std::ios_base::cur);          

      // indentify number of file headers, contributed into the dataset
      this->m_fileStream.read(&data_buffer[0], 4);               
      unsigned int nFiles = *((uint32_t*) (&data_buffer[0]));


      /// allocate space for the component headers positions;
      m_dataPositions.component_headers_starts.assign(nFiles, 0);

      std::streamoff last_location = m_fileStream.tellg();
      if (nFiles > 0)
      {
        m_dataPositions.component_headers_starts[0] = last_location;
      }
    }


    std::streamoff 
      LoadSQW::parse_component_header(std::streamoff start_location)
    { // we do not need this header  at the moment -> just calculating its length; or may be we do soon?
      std::vector<char> data_buffer(8);


      std::streamoff end_location = start_location;
      std::streamoff shift = start_location-this->m_fileStream.tellg();
      // move to spefied location, which should be usually 0;
      m_fileStream.seekg(shift,std::ios_base::cur);



      this->m_fileStream.read(&data_buffer[0],4);


      unsigned int file_name_length= *((uint32_t*)(&data_buffer[0]));
      //skip component header file name
      m_fileStream.seekg(file_name_length,std::ios_base::cur);


      this->m_fileStream.read(&data_buffer[0],4);
      unsigned int file_path_length= *((uint32_t*)(&data_buffer[0]));


      //skip component header file path
      m_fileStream.seekg(file_path_length,std::ios_base::cur);


      // move to by specifified nuber of bytes, see Matlab header above;
      m_fileStream.seekg(4*(7+3*4),std::ios_base::cur);


      // read number of energy bins;
      this->m_fileStream.read(&data_buffer[0],4);
      unsigned int nEn_bins = *((uint32_t*)(&data_buffer[0]));
      // skip energy values;
      m_fileStream.seekg(4*(nEn_bins),std::ios_base::cur);

      // skip offsets and conversions;
      m_fileStream.seekg(4*(4+4*4+4),std::ios_base::cur);


      // get labels matix size;
      this->m_fileStream.read(&data_buffer[0],8);

      unsigned int nRows = *((uint32_t*)(&data_buffer[0]));
      unsigned int nCols = *((uint32_t*)(&data_buffer[4]));

      // skip labels
      m_fileStream.seekg(nRows*nCols,std::ios_base::cur);


      end_location = (unsigned int)m_fileStream.tellg();
      return end_location;


    }
    
    std::streamoff
      LoadSQW::parse_sqw_detpar(std::streamoff start_location)
    { //
      std::vector<char> data_buffer(8);

      std::streamoff end_location = start_location;
      std::streamoff shift = start_location-this->m_fileStream.tellg();
      // move to specified location, which should be usually 0;
      m_fileStream.seekg(shift,std::ios_base::cur);              


      this->m_fileStream.read(&data_buffer[0],4);                 

      unsigned int file_name_length= *((uint32_t*)(&data_buffer[0]));
      //skip component header file name
      m_fileStream.seekg(file_name_length,std::ios_base::cur);     

      this->m_fileStream.read(&data_buffer[0],4);
      unsigned int file_path_length= *((uint32_t*)(&data_buffer[0]));  
      //skip component header file path
      m_fileStream.seekg(file_path_length,std::ios_base::cur);     

      this->m_fileStream.read(&data_buffer[0],4);
      unsigned int num_detectors = *((uint32_t*)(&data_buffer[0]));
      //skip detector information
      m_fileStream.seekg(num_detectors*6*4,std::ios_base::cur);  

      end_location = m_fileStream.tellg();
      return end_location;


    }
    void
      LoadSQW::parse_data_locations(std::streamoff data_start)
    {
      std::vector<char> data_buffer(12);

      //std::streamoff end_location = data_start;
      std::streamoff shift = data_start-this->m_fileStream.tellg();
      // move to specified location, which should be usually 0;
      m_fileStream.seekg(shift,std::ios_base::cur);

      this->m_fileStream.read(&data_buffer[0],4);


      unsigned int file_name_length= *((uint32_t*)(&data_buffer[0]));
      //skip dummy file name
      m_fileStream.seekg(file_name_length,std::ios_base::cur);


      this->m_fileStream.read(&data_buffer[0],4);
      unsigned int file_path_length= *((uint32_t*)(&data_buffer[0]));

      //skip dummy file path
      m_fileStream.seekg(file_path_length,std::ios_base::cur);


      this->m_fileStream.read(&data_buffer[0],4);
      unsigned int data_title_length = *((uint32_t*)(&data_buffer[0]));

      //skip data title
      m_fileStream.seekg(data_title_length,std::ios_base::cur);


      this->m_dataPositions.geom_start = m_fileStream.tellg();

      m_fileStream.seekg(4*(3+3+4+16+4),std::ios_base::cur);

      // get label information and skip labels;
      this->m_fileStream.read(&data_buffer[0],8);

      unsigned int n_labels      = *((uint32_t*)(&data_buffer[0])); 
      unsigned int labels_length = *((uint32_t*)(&data_buffer[4])); 
      m_fileStream.seekg(n_labels*labels_length,std::ios_base::cur);

      this->m_dataPositions.npax_start = m_fileStream.tellg();

      this->m_fileStream.read(&data_buffer[0],4);

      unsigned int npax = *((uint32_t*)(&data_buffer[0])); 
      unsigned int niax = npax-4;
      if(niax!=0){
        m_fileStream.seekg(3*niax*4,std::ios_base::cur);

      }
      if(npax!=0){
        this->m_nBins.resize(npax);

        // skip projection axis
        m_fileStream.seekg(npax*4,std::ios_base::cur);

        this->m_mdImageSize = 1;
        unsigned int nAxisPoints;
        for(unsigned int i=0;i<npax;i++){
          this->m_fileStream.read(&data_buffer[0],4);

          nAxisPoints = *((uint32_t*)(&data_buffer[0])); 
          m_nBins[i] = nAxisPoints-1;
          this->m_mdImageSize *= m_nBins[i] ;
          m_fileStream.seekg(nAxisPoints*4,std::ios_base::cur);

        }
        // skip display indexes;
        m_fileStream.seekg(npax*4,std::ios_base::cur);

      }
      // signal start:
      this->m_dataPositions.s_start = m_fileStream.tellg();
      // and skip to errors
      m_fileStream.seekg(this->m_mdImageSize*4,std::ios_base::cur);

      // error start:
      this->m_dataPositions.err_start= m_fileStream.tellg();
      m_fileStream.seekg(this->m_mdImageSize*4,std::ios_base::cur);

      // dnd data file.  we do not suppor this?
      if(m_fileStream.eof()){
        g_log.error()<<" DND horace data file supplied. This file reader needs SQW Horace type data file\n";
        throw(std::invalid_argument("DND Horace datasets are not supported by Mantid"));
      }

      this->m_dataPositions.n_cell_pix_start=m_fileStream.tellg();
      // skip to the end of pixels;
      m_fileStream.seekg(this->m_mdImageSize*8,std::ios_base::cur);

      if(m_fileStream.eof()){
        g_log.error()<<" DND b+ horace data file supplied. This file reader needs full SQW Horace type data file\n";
        throw(std::invalid_argument("DND b+ Horace datasets are not supported by Mantid"));
      }
      this->m_dataPositions.min_max_start = m_fileStream.tellg();
      // skip min-max start
      //[data.urange,count,ok,mess] = fread_catch(fid,[2,4],'float32'); if ~all(ok); return; end;
      m_fileStream.seekg(8*4,std::ios_base::cur);

      if(m_fileStream.eof()){
        g_log.error()<<" SQW a- horace data file supplied. This file reader needs full SQW Horace type data file\n";
        throw(std::invalid_argument("SQW a- Horace datasets are not supported by Mantid"));
      }
      // skip redundant field and read nPix (number of data points)
      this->m_fileStream.read(&data_buffer[0],12);

      this->m_nDataPoints =(size_t)( *((uint64_t*)(&data_buffer[4])));
      this->m_dataPositions.pix_start = this->m_fileStream.tellg();
    }


    /*==================================================================================
    EndRegion:
    ==================================================================================*/
}
}
