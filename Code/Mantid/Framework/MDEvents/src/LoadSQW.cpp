#include "MantidMDEvents/LoadSQW.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidKernel/Matrix.h"
#include "MantidGeometry/MDGeometry/MDHistoDimensionBuilder.h"
#include <iostream>
#include "MantidKernel/ThreadScheduler.h"
#include "MantidKernel/ThreadPool.h"
#include "MantidAPI/Progress.h"
#include "MantidKernel/CPUTimer.h"

using Mantid::Kernel::ThreadSchedulerFIFO;
using Mantid::Kernel::ThreadPool;
using Mantid::API::Progress;
using Mantid::Kernel::CPUTimer;

namespace Mantid
{
  namespace MDEvents
  {

    DECLARE_ALGORITHM(LoadSQW)

    /// Constructor
    LoadSQW::LoadSQW()
    {
    }

    /// Initalize the algorithm
    void LoadSQW::init()
    {
      std::vector<std::string> fileExtensions(1);
      fileExtensions[0]=".sqw";
      declareProperty(new API::FileProperty("Filename","", API::FileProperty::Load,fileExtensions), "File of type SQW format");
      declareProperty(new API::WorkspaceProperty<API::IMDEventWorkspace>("OutputWorkspace","", Kernel::Direction::Output),
        "Output IMDEventWorkspace reflecting sqw data read-in");
    }

    /// Execute the algorithm
    void LoadSQW::exec()
    {
      std::string filename = getProperty("Filename");
      m_fileStream.open(filename.c_str(), std::ios::binary);

      // Parse Extract metadata. Including data locations.
      parseMetadata();

      // Create a new output workspace.
      MDEventWorkspace4* pWs = new MDEventWorkspace4;
      Mantid::API::IMDEventWorkspace_sptr ws(pWs);

      // Add dimensions onto workspace.
      addDimensions(pWs);
      
      // Set some reasonable values for the box controller
      pWs->getBoxController()->setSplitInto(3);
      pWs->getBoxController()->setSplitThreshold(400);

      // Initialize the workspace.
      pWs->initialize();
      // Start with a MDGridBox.
      pWs->splitBox();

      // Read events into the workspace.
      addEvents(pWs);

      //Persist the workspace.
      
      API::IMDEventWorkspace_sptr i_out = getProperty("OutputWorkspace");
      if(i_out)
      {
        throw std::runtime_error("Cannot currently handle overriting of an existing workspace.");
      }
      
      setProperty("OutputWorkspace", ws);
    }

    /// Provide wiki documentation.
    void LoadSQW::initDocs()
    {
      this->setWikiSummary("Create an IMDEventWorkspace with events in reciprocal space (Qx, Qy, Qz) from a SQW file.");
      this->setOptionalMessage("Create an IMDEventWorkspace with events in reciprocal space (Qx, Qy, Qz) from a SQW file.");
      this->setWikiDescription(""
        "The algorithm takes every pixel defined in the SQW horace file and converts it into an event. SQW DND/Image data is used to format dimension, with which to work."
        "After the algorithm completes a fully formed [[MDEventWorkspace]] is provided.\n"
        "If the OutputWorkspace does NOT already exist, a default one is created. This is not the only route to generating MDEventWorkspaces."
        ""
        );
    }

    /// Add a dimension after reading info from file.
    void LoadSQW::addDimensions(Mantid::MDEvents::MDEventWorkspace4* ws)
    {
      using Mantid::Geometry::MDHistoDimensionBuilder;
      Mantid::Geometry::Vec_MDHistoDimensionBuilder dimensionVec(4);
      MDHistoDimensionBuilder& qx = dimensionVec[0];
      MDHistoDimensionBuilder& qy = dimensionVec[1];
      MDHistoDimensionBuilder& qz = dimensionVec[2];
      MDHistoDimensionBuilder& en = dimensionVec[3];

      //Horace Tags.
      qx.setId("qx");
      qx.setUnits("Ang");
      qy.setId("qy");
      qy.setUnits("Ang");
      qz.setId("qz");
      qz.setUnits("Ang");
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
    }

    /// Add events after reading pixels/datapoints from file.
    void LoadSQW::addEvents(Mantid::MDEvents::MDEventWorkspace4* ws)
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
      const size_t column_size_4 = column_size * 4; //offset, gives s
      const size_t column_size_5 = column_size * 5; //offset, gives err
      const size_t pixel_width = ncolumns * column_size;
      const size_t data_buffer_size = pixel_width * m_nDataPoints;

      std::vector<char> vecBuffer = std::vector<char>(data_buffer_size);
      char* pData = &vecBuffer[0];

      Progress * prog = new Progress(this, 0.0, 1.0, 50);

      prog->report("Loading file");

      this->m_fileStream.seekg(this->m_dataPositions.pix_start, std::ios::beg);
      this->m_fileStream.read(pData,data_buffer_size);
      float error;

      std::cout << tim << " to load the data from file." << std::endl;

      prog->report("Loading pixels");

      // Perform the loading/splitting in chunks
      size_t startBlock = 0;
      size_t lastChunkSize = 10000;
      size_t endBlock = lastChunkSize*pixel_width;
      // Don't go past the end
      if (endBlock > data_buffer_size) endBlock = data_buffer_size;

      while (startBlock < data_buffer_size)
      {
        prog->report("Making Events");
        for(size_t current_pix = startBlock; current_pix < endBlock; current_pix+=pixel_width)
        {
          coord_t centers[4] =
          {
              *(reinterpret_cast<float*>(pData + current_pix)),
              *(reinterpret_cast<float*>(pData + current_pix + column_size)),
              *(reinterpret_cast<float*>(pData + current_pix + column_size_2)),
              *(reinterpret_cast<float*>(pData + current_pix + column_size_3))
          };
          error = *reinterpret_cast<float*>(pData + current_pix + column_size_5);
          ws->addEvent(MDEvent<4>(*reinterpret_cast<float*>(pData + current_pix + column_size_4), error*error , centers));
        }

        std::cout << tim << " to make and add the events." << std::endl;

        prog->report("Splitting boxes");

        // This splits up all the boxes according to split thresholds and sizes.
        // TODO: Would it be more efficient to do the splitting on regulare intervals? Not necessarily, should try it.
        Kernel::ThreadScheduler * ts = new ThreadSchedulerFIFO();
        ThreadPool tp(ts);
        ws->splitAllIfNeeded(ts);
        tp.joinAll();

        std::cout << tim << " to split the MDBoxes." << std::endl;

        // Get ready for the next chunk. Grow them by a factor of 2 each time.
        startBlock = endBlock;
        lastChunkSize *= 2;
        endBlock = startBlock + lastChunkSize*pixel_width;
        // Don't go past the end
        if (endBlock > data_buffer_size) endBlock = data_buffer_size;
      }

      prog->report("Refreshing cache");
      ws->refreshCache();

      delete prog;

    }

    /// Destructor
    LoadSQW::~LoadSQW()
    {
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
      unsigned int i;

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
        for(i=0;i<npax;i++){
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
