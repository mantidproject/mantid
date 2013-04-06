#include "MantidMDEvents/BoxControllerNxSIO.h"
#include "MantidMDEvents/MDBoxFlatTree.h"
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

    std::string BoxControllerNxSIO::g_EventGroupName("event_data");
    std::string BoxControllerNxSIO::g_DBDataName("free_space_blocks");

   /**Constructor 
    @param bc shared pointer to the box controller which uses this IO operations
   */ 
   BoxControllerNxSIO::BoxControllerNxSIO(API::BoxController *const bc) :
       m_File(NULL),
       m_ReadOnly(true),
       m_dataChunk(DATA_CHUNK),  
       m_bc(bc),
       m_BlockStart(2,0),
       m_BlockSize(2,0),
       m_CoordSize(sizeof(coord_t)),
       m_EventType(FatEvent),
       m_EventsVersion("1.0")
   {
       m_BlockSize[1] = 4+m_bc->getNDims();

       for(size_t  i=0;i<2;i++)
       {
           m_EventsTypesSupported.push_back(EventTypes[i]);
           m_EventsTypeHeaders.push_back(EventHeaders[i]);
       }

       //m_EventsTypesSupported.assign(EventTypes,std::end(EventTypes));
       //m_EventsTypeHeaders.assign(EventHeaders,std::end(EventHeaders));
   }
   /**get event type form its string representation*/ 
   BoxControllerNxSIO::EventType BoxControllerNxSIO::TypeFromString(const std::vector<std::string> &typesSupported,const std::string typeName)
   {
          auto it = std::find(typesSupported.begin(),typesSupported.end(),typeName);
          if(it==typesSupported.end())
              throw std::invalid_argument("Unsupported event type: "+typeName+" provided ");

          return static_cast<EventType>(std::distance(typesSupported.begin(), it ));
   }
 /** The optional method to set up the event type and the size of the event coordinate
   *  As save/load operations use void data type, these function allow set up/get  the type name provided for the IO operations
   *  and the size of the data type in bytes (e.g. the  class dependant physical  meaning of the blockSize and blockPosition used 
   *  by save/load operations     
   * @param CoordSize -- size (in bytes) of the blockPosition and blockSize used in save/load operations. 4 and 8 are supported only
   * @paramtypeName  -- the name of the event used in the operations. The name itself defines the size and the format of the event
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
              m_BlockSize[1] = 2+m_bc->getNDims();
              break;
          case (FatEvent):
              m_BlockSize[1] = 4+m_bc->getNDims();
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

  void BoxControllerNxSIO::getDataType(size_t &CoordSize, std::string &typeName)const
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

      m_fileMutex.lock();
      m_ReadOnly = true;
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
      m_File = MDBoxFlatTree::createOrOpenMDWSgroup(m_fileName,this->m_bc->getNDims(), m_EventsTypesSupported[m_EventType],m_ReadOnly);
      // we are in MD workspace Class  group now
      std::map<std::string, std::string> groupEntries;
      m_File->getEntries(groupEntries);
      if(groupEntries.find(g_EventGroupName)!=groupEntries.end()) // yes, open it
          OpenAndCheckEventGroup();
      else // create and open it
          CreateEventGroup();
      // we are in MDEvent group now (either created or opened)


      // read if exist and create if not the group, which is responsible for saving DiskBuffer infornation;
      getDiskBufferFileData();

      if(m_ReadOnly)
          prepareNxSdata_CurVersion();
      else
          prepareNxSToWrite_CurVersion();
      m_fileMutex.unlock();

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
  /** Helper function which prepares NeXus event structure to accept events   */ 
  void BoxControllerNxSIO::prepareNxSToWrite_CurVersion()
  {

      // Are data already there?
      std::string EventData("event_data");
      std::map<std::string, std::string> groupEntries;
      m_File->getEntries(groupEntries);
      if(groupEntries.find(EventData)!=groupEntries.end()) // yes, open it
      {
          prepareNxSdata_CurVersion();
      }
      else  // no, create it
      {
         // Prepare the event data array for writing operations:
         m_BlockSize[0] = NX_UNLIMITED;

        // Now the chunk size.
        std::vector<int64_t> chunk(m_BlockSize);
        chunk[0] = static_cast<int64_t>(m_dataChunk);

        // Make and open the data
        if(m_CoordSize==4)
            m_File->makeCompData("event_data", ::NeXus::FLOAT32, m_BlockSize, ::NeXus::NONE, chunk, true);
        else
            m_File->makeCompData("event_data", ::NeXus::FLOAT64, m_BlockSize, ::NeXus::NONE, chunk, true);

        // A little bit of description for humans to read later
        m_File->putAttr("description", m_EventsTypeHeaders[m_EventType]);
        // disk buffer knows that the file has no events
        this->setFileLength(0);


      }
     

  }
  /** Open the NXS data blocks for loading/saving.
    * The data should have been created before.     */
  void BoxControllerNxSIO::prepareNxSdata_CurVersion()
  {
      // Open the data
      m_File->openData("event_data");
// There are rummors that this is faster. Not sure if it is important
//      int type = ::NeXus::FLOAT32; 
//      int rank = 0;
//      NXgetinfo(file->getHandle(), &rank, dims, &type);
       NeXus::Info info = m_File->getInfo();
       if(info.type == ::NeXus::FLOAT64)
       {
           if(m_CoordSize == 4)
               throw Kernel::Exception::NotImplementedError("converting from old style 64-bit event file data is not yet implemented ");
       }

      // check if the number of dimensions in the file corresponds to the number of dimesnions to read.
      size_t nFileDim;
      size_t ndim2    = static_cast<size_t>(info.dims[1]);
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

      //HACK -- there is no difference between empty event dataset and the dataset with 1 event. 
      // It is unclear how to deal with this stuff but the situations, where the dataset was created and closed without writing there anything 
      // and then opened again to write data into it are probably rare. 
      uint64_t nFilePoints = info.dims[0];
      this->setFileLength(nFilePoints);
    }
  /** Load free space blocks from the data file or create the NeXus place to read/write them*/
  void BoxControllerNxSIO::getDiskBufferFileData()
  {
     std::vector<uint64_t> freeSpaceBlocks;
     this->getFreeSpaceVector(freeSpaceBlocks);
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

 /** Save data block on specific position within properly opened NeXus data array
   *@param DataBlock     -- the vector with data to write
   *@param blockPosition -- The starting place to save data to   */ 
 void BoxControllerNxSIO::saveBlock(const std::vector<float> & DataBlock, const uint64_t blockPosition)const 
 {
      std::vector<int64_t> start(2,0);
      start[0] = int64_t(blockPosition);

      // Specify the dimensions
      std::vector<int64_t> dims(m_BlockSize);
      dims[0] =  int64_t(DataBlock.size()/this->getNDataColums());


     // ugly cast but why would putSlab change the data?. This is NeXus bug which makes putSlab method non-constant
     std::vector<float> &mData = const_cast<std::vector<float>& >(DataBlock);

     m_fileMutex.lock();
     {
        m_File->putSlab<float>(mData,start,dims);

        if(blockPosition+dims[0]>this->getFileLength())
             this->setFileLength(blockPosition+dims[0]);
     }
     m_fileMutex.unlock();

 }

 /** Load particular data block from the opened NeXus file. 
   *@param Block         -- the storage vector to place data into
   *@param blockPosition -- The starting place to read data from
   *@param nPoints       -- number of data points (events) to read 

   *@returns Block -- resized block of data containing serialized events representation. 
 */
 void BoxControllerNxSIO::loadBlock(std::vector<float> & Block, const uint64_t blockPosition,const size_t nPoints)const
 {
     if(blockPosition+nPoints>this->getFileLength())
         throw Kernel::Exception::FileError("Attemtp to read behind the file end",m_fileName);


     std::vector<int64_t> start(2,0);
     start[0] = static_cast<int64_t>(blockPosition);
     std::vector<int64_t> size(m_BlockSize);
     size[0]=static_cast<int64_t>(nPoints);
     Block.resize(size[0]*size[1]);

     m_fileMutex.lock();
       m_File->getSlab(&Block[0],start,size);
     m_fileMutex.unlock();


 }
 /// Clear NeXus internal cache
 void BoxControllerNxSIO::flushData()const
 {
   m_fileMutex.lock();
     m_File->flush();
   m_fileMutex.unlock();
 }
 /** flash disk buffer data from memory and close underlying NeXus file*/ 
 void BoxControllerNxSIO::closeFile()
 {
     if(m_File)
     {
         // write all file-backed data still stack in the data buffer into the file.
         this->flushCache();
         m_fileMutex.lock();
         {
             m_File->closeData(); // close events data

             if(!m_ReadOnly)    // write free space groups from the disk buffer
             {
                 std::vector<uint64_t> freeSpaceBlocks;
                 this->getFreeSpaceVector(freeSpaceBlocks);
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
             m_File->closeGroup(); // close workspace group
             m_File->close();      // close NeXus file

             delete m_File;
             m_File=NULL;
         }
         m_fileMutex.unlock();
     }
 }

 BoxControllerNxSIO::~BoxControllerNxSIO()
 {
     this->closeFile();
 }

///**TODO: this should not be here, refactor out*/ 
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

//  void MDBoxFlatTree::initEventFileStorage(::NeXus::File *hFile,API::BoxController_sptr bc,bool setFileBacked,const std::string &EventType)
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
//    if (setFileBacked)         // Set it back to the new file handle
//       bc->setFile(hFile, m_FileName, NumOldEvents);
//
//
//  }
//
}
}