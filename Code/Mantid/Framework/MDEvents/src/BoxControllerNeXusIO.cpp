#include "MantidMDEvents/BoxControllerNeXusIO.h"
#include "MantidMDEvents/MDBoxFlatTree.h"
#include "MantidKernel/Exception.h"
#include "MantidAPI/FileFinder.h"
#include "MantidMDEvents/MDEvent.h"

#include <string>

namespace Mantid
{
namespace MDEvents
{
    // Default headers(attributes) describing the contents of the data, written by this class
    const char *EventHeaders[] ={"signal, errorSquared, center (each dim.)",
                                 "signal, errorSquared, runIndex, detectorId, center (each dim.)"};

    std::string BoxControllerNeXusIO::g_EventGroupName("event_data");
    std::string BoxControllerNeXusIO::g_DBDataName("free_space_blocks");

   /**Constructor 
    @param bc shared pointer to the box controller which uses this IO operations
   */ 
   BoxControllerNeXusIO::BoxControllerNeXusIO(API::BoxController *const bc) :
       m_File(NULL),
       m_ReadOnly(true),
       m_dataChunk(DATA_CHUNK),  
       m_bc(bc),
       m_BlockStart(2,0),
       m_BlockSize(2,0),
       m_CoordSize(sizeof(coord_t)),
       m_EventType(FatEvent),
       m_EventsVersion("1.0"),
       m_ReadConversion(noConversion)
   {
       m_BlockSize[1] = 4+m_bc->getNDims();

       for(size_t  i=0;i<2;i++)
       {
           m_EventsTypeHeaders.push_back(EventHeaders[i]);
       }

       m_EventsTypesSupported.resize(2);
       m_EventsTypesSupported[LeanEvent] = MDLeanEvent<1>::getTypeName();
       m_EventsTypesSupported[FatEvent] = MDEvent<1>::getTypeName();

   }
   /**get event type form its string representation*/ 
   BoxControllerNeXusIO::EventType BoxControllerNeXusIO::TypeFromString(const std::vector<std::string> &typesSupported,const std::string typeName)
   {
          auto it = std::find(typesSupported.begin(),typesSupported.end(),typeName);
          if(it==typesSupported.end())
              throw std::invalid_argument("Unsupported event type: "+typeName+" provided ");

          return static_cast<EventType>(std::distance(typesSupported.begin(), it ));
   }
 /** The optional method to set up the event type and the size of the event coordinate
   *  As save/load operations use void data type, these function allow set up/get  the type name provided for the IO operations
   *  and the size of the data type in bytes (e.g. the  class dependent physical  meaning of the blockSize and blockPosition used 
   *  by save/load operations     
   * @param blockSize -- size (in bytes) of the blockPosition and blockSize used in save/load operations. 4 and 8 are supported only
                         e.g. float and double
   * @param typeName  -- the name of the event used in the operations. The name itself defines the size and the format of the event
                         The events described in the class header are supported only  */
  void BoxControllerNeXusIO::setDataType(const size_t blockSize, const std::string &typeName)
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
   *  and the size of the data type in bytes (e.g. the  class dependent physical  meaning of the blockSize and blockPosition used 
   *  by save/load operations     
   *@return CoordSize -- size (in bytes) of the blockPosition and blockSize used in save/load operations
   *@return typeName  -- the name of the event used in the operations. The name itself defines the size and the format of the event
  */

  void BoxControllerNeXusIO::getDataType(size_t &CoordSize, std::string &typeName)const
  {
      CoordSize= m_CoordSize;
      typeName = m_EventsTypesSupported[m_EventType];
  }

  /**Open the file to use in IO operations with events 
   *
   *@param fileName -- the name of the file to open. Search for file performed within the Mantid search path. 
   *@param mode  -- opening mode (read or read/write)
   *
   *
  */ 
  bool BoxControllerNeXusIO::openFile(const std::string &fileName,const std::string &mode)
  {
      // file already opened 
      if(m_File)return false;

      Poco::ScopedLock<Poco::FastMutex> _lock(m_fileMutex);
      m_ReadOnly = true;
      if(mode.find("w")!=std::string::npos ||mode.find("W")!=std::string::npos)
      {
          m_ReadOnly=false; 
      }

      // open file if it exists or crate it if not in the mode requested
      m_fileName = API::FileFinder::Instance().getFullPath(fileName);
      if(m_fileName.empty())
      {
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
      int nDims = static_cast<int>(this->m_bc->getNDims());

      bool group_exists;
      m_File = MDBoxFlatTree::createOrOpenMDWSgroup(m_fileName,nDims, m_EventsTypesSupported[m_EventType],m_ReadOnly,group_exists);

      // we are in MD workspace Class  group now
      std::map<std::string, std::string> groupEntries;
      m_File->getEntries(groupEntries);
      if(groupEntries.find(g_EventGroupName)!=groupEntries.end()) // yes, open it
          OpenAndCheckEventGroup();
      else // create and open it
          CreateEventGroup();
      // we are in MDEvent group now (either created or opened)


      // read if exist and create if not the group, which is responsible for saving DiskBuffer information;
      getDiskBufferFileData();

      if(m_ReadOnly)
          prepareNxSdata_CurVersion();
      else
          prepareNxSToWrite_CurVersion();

      return true;
  }
  /**Create group responsible for keeping events and add necessary attributes to it*/ 
  void BoxControllerNeXusIO::CreateEventGroup()
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
  void BoxControllerNeXusIO::OpenAndCheckEventGroup()
  {

      m_File->openGroup(g_EventGroupName, "NXdata");
      std::string fileGroupVersion;
      m_File->getAttr("version",fileGroupVersion);

      if(fileGroupVersion!=m_EventsVersion)
             throw Kernel::Exception::FileError("Trying to open existing data grop to write new event data but the group with differetn version: "+
                                                 fileGroupVersion+" already exists ",m_fileName);

  }
  /** Helper function which prepares NeXus event structure to accept events   */ 
  void BoxControllerNeXusIO::prepareNxSToWrite_CurVersion()
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
  void BoxControllerNeXusIO::prepareNxSdata_CurVersion()
  {
      // Open the data
      m_File->openData("event_data");
// There are rummors that this is faster. Not sure if it is important
//      int type = ::NeXus::FLOAT32; 
//      int rank = 0;
//      NXgetinfo(file->getHandle(), &rank, dims, &type);

       NeXus::Info info = m_File->getInfo();
       int Type = info.type;

       m_ReadConversion = noConversion;
       switch(Type)
       {
       case(::NeXus::FLOAT64):
               if(m_CoordSize == 4)
                   m_ReadConversion = doubleToFolat;
                break;
       case(::NeXus::FLOAT32):
               if(m_CoordSize == 8)
                   m_ReadConversion = floatToDouble;
               break;

       default:
           throw Kernel::Exception::FileError("Unknown events data format ",m_fileName);
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
  void BoxControllerNeXusIO::getDiskBufferFileData()
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
          // Read the free space blocks in from the existing file
          m_File->readData(g_DBDataName, freeSpaceBlocks);
          this->setFreeSpaceVector(freeSpaceBlocks);
      }
      else  // create and open the group
      {
          if(m_ReadOnly)
              throw Kernel::Exception::FileError("Attempt to create new DB group in the read-only file",m_fileName);
           m_File->writeExtendibleData(g_DBDataName, freeSpaceBlocks, free_dims, free_chunk);
      }
 }


//-------------------------------------------------------------------------------------------------------------------------------------
 /** Save generc data block on specific position within properly opened NeXus data array
   *@param DataBlock     -- the vector with data to write
   *@param blockPosition -- The starting place to save data to   */ 
 template<typename Type>
 void BoxControllerNeXusIO::saveGenericBlock(const std::vector<Type> & DataBlock, const uint64_t blockPosition)const 
 {
      std::vector<int64_t> start(2,0);
      // Specify the dimensions
      std::vector<int64_t> dims(m_BlockSize);

     Poco::ScopedLock<Poco::FastMutex> _lock(m_fileMutex);
     start[0] = int64_t(blockPosition);
     dims[0] =  int64_t(DataBlock.size()/this->getNDataColums());
     

     // ugly cast but why would putSlab change the data?. This is NeXus bug which makes putSlab method non-constant
     std::vector<Type> &mData = const_cast<std::vector<Type>& >(DataBlock);

     {
        m_File->putSlab<Type>(mData,start,dims);

        if(blockPosition+dims[0]>this->getFileLength())
             this->setFileLength(blockPosition+dims[0]);
     }


 }
  
/** Save float data block on specific position within properly opened NeXus data array
   *@param DataBlock     -- the vector with data to write
   *@param blockPosition -- The starting place to save data to   */ 
  void BoxControllerNeXusIO::saveBlock(const std::vector<float> & DataBlock, const uint64_t blockPosition)const
 {
     this->saveGenericBlock(DataBlock,blockPosition);
 }
/** Save double precision data block on specific position within properly opened NeXus data array
   *@param DataBlock     -- the vector with data to write
   *@param blockPosition -- The starting place to save data to   */ 
 void BoxControllerNeXusIO::saveBlock(const std::vector<double> & DataBlock, const uint64_t blockPosition)const
 {
     this->saveGenericBlock(DataBlock,blockPosition);
 }

 /** Load generic  data block from the opened NeXus file. 
   *@param Block         -- the storage vector to place data into
   *@param blockPosition -- The starting place to read data from
   *@param nPoints       -- number of data points (events) to read 

   *@returns Block -- resized block of data containing serialized events representation. 
 */
 template<typename Type> 
 void BoxControllerNeXusIO::loadGenericBlock(std::vector<Type> & Block, const uint64_t blockPosition,const size_t nPoints)const
 {
     if(blockPosition+nPoints>this->getFileLength())
         throw Kernel::Exception::FileError("Attemtp to read behind the file end",m_fileName);


     std::vector<int64_t> start(2,0);
     std::vector<int64_t> size(m_BlockSize);

     Poco::ScopedLock<Poco::FastMutex> _lock(m_fileMutex);

     start[0] = static_cast<int64_t>(blockPosition);
     size[0]=static_cast<int64_t>(nPoints);
     Block.resize(size[0]*size[1]);


     m_File->getSlab(&Block[0],start,size);


 }

 /** Helper funcion which allows to convert one data fomat into another */
 template<typename FROM,typename TO>
 void convertFormats(const std::vector<FROM> &inData,std::vector<TO> &outData)
 {
     outData.reserve(inData.size());
     for(size_t i=0;i<inData.size();i++)
     {
         outData.push_back(static_cast<TO>(inData[i]));
     }
 }
 /** Load float  data block from the opened NeXus file. 
   *@param Block         -- the storage vector to place data into
   *@param blockPosition -- The starting place to read data from
   *@param nPoints       -- number of data points (events) to read 

   *@returns Block -- resized block of data containing serialized events representation. 
 */
 void BoxControllerNeXusIO::loadBlock(std::vector<float> & Block, const uint64_t blockPosition,const size_t nPoints)const
 {
     std::vector<double> tmp;
     switch(m_ReadConversion)
     {
     case(noConversion):
         loadGenericBlock(Block,blockPosition,nPoints);
         break;
     case(doubleToFolat):
         loadGenericBlock(tmp,blockPosition,nPoints);
         convertFormats(tmp,Block);
         break;
     default:
         throw Kernel::Exception::FileError(" Attempt to read float data from unsupported file format",m_fileName);
     }       
 }
 /** Load double  data block from the opened NeXus file. 
   *@param Block         -- the storage vector to place data into
   *@param blockPosition -- The starting place to read data from
   *@param nPoints       -- number of data points (events) to read 

   *@returns Block -- resized block of data containing serialized events representation. 
 */
void BoxControllerNeXusIO::loadBlock(std::vector<double> & Block, const uint64_t blockPosition,const size_t nPoints)const
 {
     std::vector<float> tmp;
     switch(m_ReadConversion)
     {
     case(noConversion):
         loadGenericBlock(Block,blockPosition,nPoints);
         break;
     case(floatToDouble):
         loadGenericBlock(tmp,blockPosition,nPoints);
         convertFormats(tmp,Block);
         break;
     default:
         throw Kernel::Exception::FileError(" Attempt to read double data from unsupported file format",m_fileName);
     }       
 }

//-------------------------------------------------------------------------------------------------------------------------------------

 /// Clear NeXus internal cache
 void BoxControllerNeXusIO::flushData()const
 {
    Poco::ScopedLock<Poco::FastMutex> _lock(m_fileMutex);
    m_File->flush();

 }
 /** flush disk buffer data from memory and close underlying NeXus file*/ 
 void BoxControllerNeXusIO::closeFile()
 {
     if(m_File)
     {
         // write all file-backed data still stack in the data buffer into the file.
         this->flushCache();
         // lock file
         Poco::ScopedLock<Poco::FastMutex> _lock(m_fileMutex);
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

                     m_File->writeUpdatedData(g_DBDataName, freeSpaceBlocks, free_dims);
                 }
             }

             m_File->closeGroup(); // close events group
             m_File->closeGroup(); // close workspace group
             m_File->close();      // close NeXus file

             delete m_File;
             m_File=NULL;
         }
     }
 }

 BoxControllerNeXusIO::~BoxControllerNeXusIO()
 {
     this->closeFile();
 }


}
}
