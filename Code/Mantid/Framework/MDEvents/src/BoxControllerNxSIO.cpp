#include "MantidMDEvents/BoxControllerNxSIO.h"
#include "MantidKernel/Exception.h"
#include "MantidAPI/FileFinder.h"

#include <string>

namespace Mantid
{
namespace MDEvents
{
    // these values have to coinside with the values defimed in MDLeanEvent and MDEvent correspondingly
    const char *EventTypes[] = {"MDLeanEvent","MDEvent"};
    // Default headers(attributes) describing the contents of the data, written by this class
    const char *EventHeaders[] ={"signal, errorSquared, center (each dim.)","signal, errorSquared, runIndex, detectorId, center (each dim.)"};

    std::string BoxControllerNxSIO::g_EventWSGroupName("MDEventWorkspace");
    std::string BoxControllerNxSIO::g_EventGroupName("event_data");
    std::string BoxControllerNxSIO::g_DBDataName("free_space_blocks");

   /**Constructor 
    @param nDim -- number of dimensions within the data to write
   */ 
   BoxControllerNxSIO::BoxControllerNxSIO(API::BoxController_sptr bc) :
       m_File(NULL),
       m_dataChunk(DATA_CHUNK),
       m_bc(bc),
       m_CoordSize(sizeof(coord_t)),
       m_EventType(FatEvent),
       m_ReadOnly(true),
       m_EventsVersion("1.0")
   {
       m_dataColumnSize=4+m_bc->getNDims();

       m_EventsTypesSupported.assign(EventTypes,std::end(EventTypes));
       m_EventsTypeHeaders.assign(EventHeaders,std::end(EventHeaders));
   }
   /**get event type form its string representation*/ 
   BoxControllerNxSIO::EventType BoxControllerNxSIO::TypeFromString(const std::vector<std::string> &typesSupported,const std::string typeName)
   {
          auto it = std::find(typesSupported.begin(),typesSupported.end(),typeName);
          if(it==typesSupported.end())
              throw std::invalid_argument("Unsupported event type: "+typeName+" provided ");

          return static_cast<EventType>(std::distance(typesSupported.begin(), it ));
   }
   /**The optional method to set up the event type and the size of the event coordinate
  /** As save/load operations use void data type, these function allow set up/get  the type name provided for the IO operations
   *  and the size of the data type in bytes (e.g. the  class dependant physical  meaning of the blockSize and blockPosition used 
   *  by save/load operations     
   *@param CoordSize -- size (in bytes) of the blockPosition and blockSize used in save/load operations. 4 and 8 are supported only
   *@paramtypeName  -- the name of the event used in the operations. The name itself defines the size and the format of the event
                       The events described in the class header are supported only
  */  
  void BoxControllerNxSIO::setDataType(const size_t blockSize, const std::string &typeName)
  {
      if(blockSize==4 || blockSize==8)
      {

          m_CoordSize = static_cast<unsigned int>(blockSize);
          m_EventType = TypeFromString(m_EventsTypesSupported,typeName);

          switch(m_EventType)
          {
          case (LeanEvent):
              m_dataColumnSize = 2+m_bc->getNDims();
              break;
          case (FatEvent):
              m_dataColumnSize = 4+m_bc->getNDims();
              break;
          default:
              throw std::invalid_argument(" Unsupported event kind Identified  ");
          }
      }
      else
          throw std::invalid_argument("The class currently supports 4(float) and 8(double) event coordinates only");
  }

  /** As save/load operations use void data type, these function allow set up/get  the type name provided for the IO operations
   *  and the size of the data type in bytes (e.g. the  class dependant physical  meaning of the blockSize and blockPosition used 
   *  by save/load operations     
   *@return CoordSize -- size (in bytes) of the blockPosition and blockSize used in save/load operations
   *@return typeName  -- the name of the event used in the operations. The name itself defines the size and the format of the event
  */

  void BoxControllerNxSIO::getDataType(size_t &CoordSize, std::string &typeName)
  {
      CoordSize= m_CoordSize;
      typeName = m_EventsTypesSupported[m_EventType];
  }

  /**Open the file to use in IO operations with events 
   *
   *@param fileName the name of the file to open. Search for file perfomed within the Mantid search path. 
   *@more  opening mode (read or read/write)
   *
   *
  */ 
  bool BoxControllerNxSIO::openFile(const std::string &fileName,const std::string &mode)
  {
      // file already opened 
      if(m_File)return false;

      m_ReadOnly = true;;
      NXaccess access(NXACC_READ);
      if(mode.find("w")!=std::string::npos ||mode.find("W")!=std::string::npos)
      {
          m_ReadOnly=false; 
          access     =NXACC_RDWR;
      }

      // open file if it exists or crate it if not in the mode requested
      bool fileExists(true);
      m_fileName = API::FileFinder::Instance().getFullPath(fileName);
      if(m_fileName.empty())
      {
          fileExists = false;
          if(!m_ReadOnly)
          {
              std::string filePath=Kernel::ConfigService::Instance().getString("defaultsave.directory");
              if(filePath.empty())
                m_fileName = fileName;
              else
                m_fileName = filePath+"/"+fileName;
          }
          else
              throw Kernel::Exception::FileError("Can not open file to read ",m_fileName);
      }
      try
      {
          if(fileExists)
              m_File = new ::NeXus::File(m_fileName, access);
          else
              m_File = new ::NeXus::File(m_fileName, NXACC_CREATE5);
      }
      catch(...)
      {
          throw Kernel::Exception::FileError("Can not open NeXus file",m_fileName);
      }

      // idenity if neessary group is already in the file

      std::map<std::string, std::string> groupEntries;

      m_File->getEntries(groupEntries);
      if(groupEntries.find(g_EventWSGroupName)!=groupEntries.end()) // WS group exist
      {
          OpenAndCheckWSGroup();
          // get WS group entries
          m_File->getEntries(groupEntries);

         if(groupEntries.find(g_EventGroupName)!=groupEntries.end()) // Event gropup exist
              OpenAndCheckEventGroup();
         else
              CreateEventGroup();
      }
      else // create ws group and Event Group place Event attribute to it. 
      {
          CreateWSGroup();
          CreateEventGroup();
      }

      // we are in MDEvnt data group now

      // read if exist and create if not the group, which is responsible for saving DiskBuffer infornation;
      getDiskBufferFileData();

      if(m_ReadOnly)
          prepareNxSToRead_CurVersion();
      else
          prepareNxSToWrite_CurVersion();


      return true;
  }
  /**Create group responsible for keeping events and add necessary attributes to it*/ 
  void BoxControllerNxSIO::CreateEventGroup()
  {
       if(m_ReadOnly) 
           throw Kernel::Exception::FileError("The NXdata group: "+g_EventGroupName+" does not exist in the file opened for read",m_fileName);

       try
       {
            m_File->makeGroup(g_EventGroupName, "NXdata",true);
            m_File->putAttr("version", m_EventsVersion);
        }
        catch(...)
       {
            throw Kernel::Exception::FileError("Can not create new NXdata group: "+g_EventGroupName,m_fileName);
       }
  }
  /**Create group responsible for keeping MD event workspace and add necessary  (some) attributes to it*/ 
  void BoxControllerNxSIO::CreateWSGroup()
  {
     if(m_ReadOnly) 
        throw Kernel::Exception::FileError("The NXdata group: "+g_EventWSGroupName+" does not exist in the file opened for read",m_fileName);

      try
      {
              m_File->makeGroup(g_EventWSGroupName, "NXentry", true);
              m_File->putAttr("event_type", m_EventsTypesSupported[m_EventType]);
      }catch(...)
      {
               throw Kernel::Exception::FileError("Can not create new NXdata group: "+g_EventWSGroupName,m_fileName);
      }

  }
  /** Open existing Workspace group and check the attributes necessary for this algorithm to work*/ 
  void BoxControllerNxSIO::OpenAndCheckWSGroup()
  {
      m_File->openGroup(g_EventWSGroupName, "NXentry");

      std::string eventType;
      m_File->getAttr("event_type",eventType);
      if(eventType!=m_EventsTypesSupported[m_EventType])
               throw Kernel::Exception::FileError("trying to write-access a workspace with the type of events different from the one intended to write ",m_fileName);

  }
  /** Open existing Event group and check the attributes necessary for this algorithm to work */ 
  void BoxControllerNxSIO::OpenAndCheckEventGroup()
  {

      m_File->openGroup(g_EventGroupName, "NXdata");
      std::string fileGroupVersion;
      m_File->getAttr("version",fileGroupVersion);

      if(fileGroupVersion!=m_EventsVersion)
             throw Kernel::Exception::FileError("Trying to open existing data grop to write new event data but the group with differetn version: "+
                                                 fileGroupVersion+" already exists ",m_fileName);

  }
  /** Helper function which prepares NeXus event structure to accept events 
   *@param groupExist -- if true, indicate that group for events already exist and new data have to be created within existing group
   */ 
  void BoxControllerNxSIO::prepareNxSToWrite_CurVersion()
  {

      // Are data already there?
      std::string EventData("event_data");
      std::map<std::string, std::string> groupEntries;
      m_File->getEntries(groupEntries);
      if(groupEntries.find(EventData)!=groupEntries.end()) // yes, open it
      {
             m_File->openData(EventData);
          //HACK -- there is no difference between empty event dataset and the dataset with 1 event. It is unclear how to deal with this stuff
             int64_t nFilePoints = m_File->getInfo().dims[0];
             m_bc->getDiskBuffer().setFileLength(nFilePoints);
      }
      else  // no, create it
      {
         // Prepare the event data array for writing operations:
         std::vector<int> dims(2,0);
         dims[0] = NX_UNLIMITED;
         // One point per dimension, plus signal, plus error, plus runIndex, plus detectorID = nd+4
         dims[1] = int(m_dataColumnSize);

        // Now the chunk size.
        std::vector<int> chunk(dims);
        chunk[0] = int(m_dataChunk);

        // Make and open the data
        if(m_CoordSize==4)
            m_File->makeCompData("event_data", ::NeXus::FLOAT32, dims, ::NeXus::NONE, chunk, true);
        else
            m_File->makeCompData("event_data", ::NeXus::FLOAT64, dims, ::NeXus::NONE, chunk, true);

        // A little bit of description for humans to read later
        m_File->putAttr("description", m_EventsTypeHeaders[m_EventType]);
        // disk buffer knows that the file has no events
        m_bc->getDiskBuffer().setFileLength(0);

      }
     

  }

    /** Open the NXS data blocks for loading.
     * The data should have been created before.
     *
     * @param file :: open NXS file.
     * @return the number of events currently in the data field.
     */
  void BoxControllerNxSIO::prepareNxSToRead_CurVersion()
  {
      // Open the data
      m_File->openData("event_data");

      // check if the number of dimensions in the file corresponds to the number of dimesnions to read.
      size_t nFileDim;
      size_t ndim2    = static_cast<size_t>(m_File->getInfo().dims[1]);
      switch(m_EventType)
      {
      case(LeanEvent):
          nFileDim = ndim2-2;
          break;
      case(FatEvent):
          nFileDim = ndim2-4;
          break;
      default:
          throw Kernel::Exception::FileError("Unexpected type of events in the data file",m_fileName);
      }

      if(nFileDim != m_bc->getNDims())
          throw Kernel::Exception::FileError("Trying to open event data with different number of dimensions ",m_fileName);

      //HACK -- there is no difference between empty event dataset and the dataset with 1 event. It is unclear how to deal with this stuff
      int64_t nFilePoints = m_File->getInfo().dims[0];
      m_bc->getDiskBuffer().setFileLength(nFilePoints);

    }

  void BoxControllerNxSIO::getDiskBufferFileData()
  {
      std::vector<uint64_t> freeSpaceBlocks;
      m_bc->getDiskBuffer().getFreeSpaceVector(freeSpaceBlocks);
     if (freeSpaceBlocks.empty())
         freeSpaceBlocks.resize(2, 0); // Needs a minimum size

  //    // Get a vector of the free space blocks to save to the file
      std::vector<int64_t> free_dims(2,2);
      free_dims[0] = int64_t(freeSpaceBlocks.size()/2);
      std::vector<int64_t> free_chunk(2,2);
      free_chunk[0] =int64_t(m_dataChunk);

      std::map<std::string, std::string> groupEntries;
      m_File->getEntries(groupEntries);
      if(groupEntries.find(g_DBDataName)!=groupEntries.end()) // data exist, open it
      {
          if(!m_ReadOnly)
              m_File->writeUpdatedData(g_DBDataName, freeSpaceBlocks, free_dims);
          //else  TODO:  we are not currently able to read and set free space blocks into memory !!!
              // m_File->readData("free_space_blocks",freeSpaceBlocks);
      }
      else  // create and open the group
      {
          if(m_ReadOnly)
              throw Kernel::Exception::FileError("Attempt to create new DB group in the read-only file",m_fileName);
           m_File->writeExtendibleData(g_DBDataName, freeSpaceBlocks, free_dims, free_chunk);
      }


  }
 void BoxControllerNxSIO::saveBlock(void const * const /* Block */, const uint64_t /*blockPosition*/,const size_t /*blockSize*/)
 {
 }
 void BoxControllerNxSIO::loadBlock(void  * const  /* Block */, const uint64_t /*blockPosition*/,const size_t /*blockSize*/)
 {
 }
 void BoxControllerNxSIO::flushData()
 {
 }
 void BoxControllerNxSIO::closeFile()
 {
    if(m_File)
    {
         m_File->closeData(); // close events data

         if(!m_ReadOnly)    // write free space groups from the disk buffer
         {
            std::vector<uint64_t> freeSpaceBlocks;
            m_bc->getDiskBuffer().getFreeSpaceVector(freeSpaceBlocks);
            if (!freeSpaceBlocks.empty())
            {
                std::vector<int64_t> free_dims(2,2);
                free_dims[0] = int64_t(freeSpaceBlocks.size()/2);
                std::vector<int64_t> free_chunk(2,2);
                free_chunk[0] =int64_t(m_dataChunk);
                m_File->writeUpdatedData("free_space_blocks", freeSpaceBlocks, free_dims);
            }
         }
         
         m_File->closeGroup(); // close events group
         m_File->closeGroup();
         m_File->close();
         delete m_File;
         m_File=NULL;
    }
 }

  BoxControllerNxSIO::~BoxControllerNxSIO()
  {
      this->closeFile();
  }


// //-----------------------------------------------------------------------------------------------
// /** Call to save the data (if needed) and release the memory used.
//  *  Called from the DiskBuffer.
//  *  If called directly presumes to know its file location and [TODO: refactor this] needs the file to be open correctly on correct group 
//  */
//  void MDBoxNXSaveable::save()
//  {
////  //      std::cout << "MDBox ID " << this->getId() << " being saved." << std::endl;
////
////
////   // this aslo indirectly checks if the object knows its place (may be wrong place but no checks for that here)
////   if (this->wasSaved())
////   {
////     //TODO: redesighn const_cast
////    // This will load and append events ONLY if needed.
////      MDBox<MDE,nd> *loader = const_cast<MDBox<MDE,nd> *>(this);
////      loader->load();  // this will set isLoaded to true if not already loaded;
////   
////
////      // This is the new size of the event list, possibly appended (if used AddEvent) or changed otherwise (non-const access)
////      if (data.size() > 0)
////      {
////
////         // Save at the ISaveable specified place
////          this->saveNexus(this->m_BoxController->getFile());
////      }
////   } 
////   else
////     if(data.size()>0)  throw std::runtime_error(" Attempt to save undefined event");
////   
////   
//  }
////
// void MDBoxNXSaveable::load()
// {
////    // Is the data in memory right now (cached copy)?
////    if (!m_isLoaded)
////    {
////      // Perform the data loading
////      ::NeXus::File * file = this->m_BoxController->getFile();
////      if (file)
////      {
////        // Mutex for disk access (prevent read/write at the same time)
////        Kernel::RecursiveMutex & mutex = this->m_BoxController->getDiskBuffer().getFileMutex();
////        mutex.lock();
////        // Note that this APPENDS any events to the existing event list
////        //  (in the event that addEvent() was called for a box that was on disk)
////        try
////        {
////          uint64_t fileIndexStart = this->getFilePosition();
////          uint64_t fileNumEvents  = this->getFileSize();
////          MDE::loadVectorFromNexusSlab(data, file,fileIndexStart, fileNumEvents);
////          m_isLoaded = true;
////          mutex.unlock();
////        }
////        catch (std::exception &e)
////        {
////          mutex.unlock();
////          throw e;
////        }
////      }
////    }
//  }
//


//
//  //-----------------------------------------------------------------------------------------------
//  /** Load the box's Event data from an open nexus file.
//   * The FileIndex start and numEvents must be set correctly already.
//   * Clear existing data from memory!
//   *
//   * @param file :: Nexus File object, must already by opened with MDE::openNexusData()
//   * @param setIsLoaded :: flag if box is loaded from file
//   */
//  TMDE(
//  inline void MDBox)::loadNexus(::NeXus::File * file, bool setIsLoaded)
//  {
//    this->data.clear();
//    uint64_t fileIndexStart = this->getFilePosition();
//    uint64_t fileNumEvents  = this->getFileSize();
//    if(fileIndexStart == std::numeric_limits<uint64_t>::max())
//      throw(std::runtime_error("MDBox::loadNexus -- attempt to load box from undefined location"));
//    MDE::loadVectorFromNexusSlab(this->data, file, fileIndexStart, fileNumEvents);
//
//   
//    this->m_isLoaded=setIsLoaded;
//  
//  }
//
//
// //-----------------------------------------------------------------------------------------------
//  /** Save the box's Event data to an open nexus file.
//   *
//   * @param file :: Nexus File object, must already by opened with MDE::prepareNexusData()
//   */
//  TMDE(
//  inline void MDBox)::saveNexus(::NeXus::File * file) const
//  {
//    //std::cout << "Box " << this->getId() << " saving to " << m_fileIndexStart << std::endl;
//    MDE::saveVectorToNexusSlab(this->data, file, this->getFilePosition(),
//                               this->m_signal, this->m_errorSquared);
//
//  }
//
//    //---------------------------------------------------------------------------------------------
//    /** When first creating a NXS file containing the data, the proper
//     * data block(s) need to be created.
//     *
//     * @param file :: open NXS file.
//     * @param chunkSize :: chunk size to use when creating the data set (in number of events).
//     */
//    static void prepareNexusData(::NeXus::File * file, const uint64_t chunkSize)
//    {
//       API::BoxController::prepareEventNexusData(file,chunkSize,nd+4,"signal, errorSquared, runIndex, detectorId, center (each dim.)");
//    }
//
//

//
//    //---------------------------------------------------------------------------------------------
//    /** When first creating a NXS file containing the data, the proper
//     * data block(s) need to be created.
//     *
//     * @param file :: open NXS file.
//     * @param chunkSize :: chunk size to use when creating the data set (in number of events).
//     */
//    static void prepareNexusData(::NeXus::File * file, const uint64_t chunkSize)
//    {
//      API::BoxController::prepareEventNexusData(file,chunkSize,nd+2,"signal, errorsquared, center (each dim.)");
//    }
//
//  
//    //---------------------------------------------------------------------------------------------
//    /** Do any final clean up of NXS event data blocks
//     *
//     * @param file :: open NXS file.
//     */  
//
//
//    //---------------------------------------------------------------------------------------------
//    /** Put a slab of MDEvent data into the nexus file.
//     * This is reused by both MDEvent and MDLeanEvent
//     *
//     * If needed, coerce it to the format of the output file (for old
//     * .nxs files in doubles)
//     *
//     * @param file :: open NXS file.
//     * @param data :: pointer to the numEvents*numColumn-sized array of data
//     *        This gets deleted by this method!
//     * @param startIndex :: index in the array to start saving to
//     * @param numEvents :: number of events to save.
//     * @param numColumns :: how many columns in the data set (depends on the data type)
//     */
//    static inline void putDataInNexus(::NeXus::File * file,
//        coord_t * data, const uint64_t startIndex,
//        const uint64_t numEvents, const size_t numColumns)
//    {
//      //TODO: WARNING NEXUS NEEDS TO BE UPDATED TO USE 64-bit ints on Windows.
//      std::vector<int64_t> start(2,0);
//      start[0] = int64_t(startIndex);
//
//      // Specify the dimensions
//      std::vector<int64_t> dims;
//      dims.push_back(int64_t(numEvents));
//      dims.push_back(int64_t(numColumns));
//
//      // C-style call is much faster than the C++ call.
////      int dims_ignored[NX_MAXRANK];
////      int type = ::NeXus::FLOAT32;
////      int rank = 0;
////      NXgetinfo(file->getHandle(), &rank, dims_ignored, &type);
//      NeXus::Info info = file->getInfo();
//
//      if (info.type == ::NeXus::FLOAT64)
//      {
//        // Handle file-backed OLD files that are in doubles.
//
//        // Convert the floats to doubles
//        size_t dataSize = (numEvents*numColumns);
//        double * dblData = new double[dataSize];
//        for (size_t i=0; i<dataSize;i++)
//          dblData[i] = static_cast<double>(data[i]);
//        delete [] data;
//
//        // And save as doubles
//        try
//        {
//          file->putSlab(dblData, start, dims);
//        }
//        catch (std::exception &)
//        {
//          delete [] dblData;
//          throw;
//        }
//
//        delete [] dblData;
//      }
//      else
//      {
//        /* ------------- Normal files, saved in floats -------- */
//        try
//        {
//          file->putSlab(data, start, dims);
//        }
//        catch (std::exception &)
//        {
//          delete [] data;
//          throw;
//        }
//        delete [] data;
//      }
//
//    }
//

//    //---------------------------------------------------------------------------------------------
//    /** Get a slab of MDEvent data out of the nexus file.
//     * This is reused by both MDEvent and MDLeanEvent
//     *
//     * If needed, coerce it to the desired output data type (coord_t)
//     *
//     * @param file :: open NXS file.
//     * @param indexStart :: index (in events) in the data field to start at
//     * @param numEvents :: number of events to load.
//     * @param numColumns :: how many columns in the data set (depends on the data type)
//     * @return a pointer to the allocated data array. Must be deleted by caller.
//     */
//    static inline coord_t * getDataFromNexus(::NeXus::File * file,
//        uint64_t indexStart, uint64_t numEvents,
//        size_t numColumns)
//    {
//
//      // Start/size descriptors
//      std::vector<int> start(2,0);
//      start[0] = int(indexStart); //TODO: What if # events > size of int32???
//
//      std::vector<int> size(2,0);
//      size[0] = int(numEvents);
//      size[1] = int(numColumns);
//
//      // Allocate the data
//      size_t dataSize = numEvents*(numColumns);
//      coord_t * data = new coord_t[dataSize];
//
//      // C-style call is much faster than the C++ call.
////      int dims[NX_MAXRANK];
////      int type = ::NeXus::FLOAT32;
////      int rank = 0;
////      NXgetinfo(file->getHandle(), &rank, dims, &type);
//      NeXus::Info info = file->getInfo();
//
//#ifdef COORDT_IS_FLOAT
//      /* coord_t is a single-precision float */
//      if (info.type == ::NeXus::FLOAT64)
//      {
//        // Handle old files that are recorded in DOUBLEs to load as FLOATS
//        double * dblData = new double[dataSize];
//        file->getSlab(dblData, start, size);
//        for (size_t i=0; i<dataSize;i++)
//          data[i] = static_cast<coord_t>(dblData[i]);
//        delete [] dblData;
//      }
//      else
//      {
//        // Get the slab into the allocated data
//        file->getSlab(data, start, size);
//      }
//#else
//      /* coord_t is double */
//      if (type == ::NeXus::FLOAT32)
//        throw std::runtime_error("The .nxs file's data is set as FLOATs but Mantid was compiled to work with data (coord_t) as doubles. Cannot load this file");
//
//      // Get the slab into the allocated data
//      file->getSlab(data, start, size);
//#endif
//      return data;
//    }
//

   

    ///** @return the open NeXus file handle. NULL if not file-backed. */
    //::NeXus::File * getFile() const
    //{ return m_file; }

    ///** Sets the open Nexus file to use with file-based back-end
    // * @param file :: file handle
    // * @param filename :: full path to the file
    // * @param fileLength :: length of the file being open, in number of events in this case */
    //void setFile(::NeXus::File * file, const std::string & filename, const uint64_t fileLength)
    //{
    //  m_file = file;
    //  m_filename = filename;
    //  m_diskBuffer.setFileLength(fileLength);
    //}

    ///** @return true if the MDEventWorkspace is backed by a file */
    //bool isFileBacked() const
    //{ return m_file != NULL; }

    ///// @return the full path to the file open as the file-based back end.
    //const std::string & getFilename() const
    //{ return m_filename; }

    //void closeFile(bool deleteFile = false);



//
//  };


//  //------------------------------------------------------------------------------------------------------
//  /** Close the open file for the back-end, if any
//   * Note: this does not save any data that might be, e.g., in the MRU.
//   * @param deleteFile :: if true, will delete the file. Default false.
//   */
//  void BoxController::closeFile(bool deleteFile)
//  {
//    if (m_file)
//    {
//      m_file->close();
//      m_file = NULL;
//    }
//    if (deleteFile && !m_filename.empty())
//    {
//      Poco::File file(m_filename);
//      if (file.exists()) file.remove();
//      m_filename = "";
//    }
//  }
//
//
//
//  //---------------------------------------------------------------------------------------------
//    void BoxController::closeNexusData(::NeXus::File * file)
//    {
//      file->closeData();
//    }

//  void MDBoxFlatTree::initEventFileStorage(const std::string &fileName,API::BoxController_sptr bc,bool FileBacked,const std::string &EventType)
//  {
//    m_FileName = fileName;
//    ::NeXus::File * hFile;
//      // Erase the file if it exists
//    Poco::File oldFile(m_FileName);
//    if (oldFile.exists())
//    {
//      hFile = new ::NeXus::File(m_FileName, NXACC_RDWR);
//      hFile->openGroup("MDEventWorkspace", "NXentry");
//    }
//    else
//    {
//      // Create a new file in HDF5 mode.
//      hFile = new ::NeXus::File(m_FileName, NXACC_CREATE5);
//      hFile->makeGroup("MDEventWorkspace", "NXentry", true);
//
//      auto nDim = int32_t(bc->getNDims());
//   // Write out some general information like # of dimensions
//      hFile->writeData("dimensions", nDim);
//      hFile->putAttr("event_type", EventType);
//      //TODO: what about history here?
//    }  
//
//    initEventFileStorage(hFile,bc,FileBacked,EventType);
//    if(!FileBacked)
//    {
//      hFile->closeGroup();
//      hFile->close();
//      delete hFile;
//    }
//  }
///**TODO: this should not be here, refactor out*/ 
//  void MDBoxFlatTree::initEventFileStorage(::NeXus::File *hFile,API::BoxController_sptr bc,bool MakeFileBacked,const std::string &EventType)
//  {
//    bool update=true;
//// Start the event Data group, TODO: should be better way of checking existing group
//    try
//    {
//      hFile->openGroup("event_data", "NXdata");
//    }
//    catch(...)
//    {
//      update=false;
//      hFile->makeGroup("event_data", "NXdata",true);
//    }
//    hFile->putAttr("version", "1.0");
//
//
//    // Prepare the data chunk storage.
//    size_t chunkSize = bc->getDataChunk();
//    size_t nDim = bc->getNDims();
//    uint64_t NumOldEvents(0);
//    if (update)
//       NumOldEvents= API::BoxController::openEventNexusData(hFile);
//    else
//    {
//      std::string descr;
//      size_t nColumns;
//      if(EventType=="MDEvent")
//      {
//        nColumns = nDim+4;
//        descr="signal, errorSquared, runIndex, detectorId, center (each dim.)";
//      }
//      else if(EventType=="MDLeanEvent")
//      {
//        nColumns = nDim+2;
//        descr="signal, errorsquared, center (each dim.)";
//      }
//      else
//        throw std::runtime_error("unknown event type encontered");
//
//      API::BoxController::prepareEventNexusData(hFile, chunkSize,nColumns,descr);
//   }
//      // Initialize the file-backing
//    if (MakeFileBacked)         // Set it back to the new file handle
//       bc->setFile(hFile, m_FileName, NumOldEvents);
//
//
//  }
//
}
}