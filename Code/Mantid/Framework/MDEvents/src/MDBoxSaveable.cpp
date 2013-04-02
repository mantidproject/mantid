#include "MantidMDEvents/MDBoxSaveable.h"
#include "MantidMDEvents/MDBox.h"

namespace Mantid
{
namespace MDEvents
{

   MDBoxSaveable::MDBoxSaveable(API::IMDNode *const Host):
       m_MDNode(Host)
   {
   }

   /** flush data out of the write buffer */
   void  MDBoxSaveable::flushData()const
   {
      m_MDNode->getBoxController()->getFileIO()->flushData();
   }

 //-----------------------------------------------------------------------------------------------
 /** Physically save the box data. Tries to load any previous data from HDD 
  *  Private function called from the DiskBuffer.
  */
  void MDBoxSaveable::save()const 
  {
    /**Save the box at the disk position defined by this class. The IMDNode has to be file backed for this method to work */
      API::IBoxControllerIO *fileIO = m_MDNode->getBoxController()->getFileIO();
      if(this->wasSaved())
      {
        auto loader = const_cast<MDBoxSaveable *>(this);
        loader->load();  
      }

      m_MDNode->saveAt(fileIO,this->getFilePosition());
      this->m_wasSaved=true;

  }
 
/** Loads the data from HDD if these data has not been loaded before. 
  * private function called from the DiskBuffer
 */
 void MDBoxSaveable::load()
 {
      API::IBoxControllerIO *fileIO = m_MDNode->getBoxController()->getFileIO(); 

    // Is the data in memory right now (cached copy)?
    if (!m_isLoaded)
    {
        m_MDNode->loadAndAddFrom(fileIO,this->m_fileIndexStart,this->m_fileNumEvents);
        this->m_isLoaded = true;
    }

  }

// old save parts

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

    ////-----------------------------------------------------------------------------------
    /// Filename of the file backend
   // std::string m_filename;
    /// The size of the events block which can be written in the neXus array at once (continious part of the data block)
    //size_t m_DataChunk;

    /// Open file handle to the file back-end
   //    ::NeXus::File * m_file;
    ///**The method returns the data chunk (continious part of the NeXus array) used to write data on HDD */ 
    //size_t getDataChunk()const
    //{
    //  return m_DataChunk;
    //}
    ///** The method used to load nexus data chunk size to the box controller. Used when loading MDEvent nexus file 
    //    Disabled at the moment as it is unclear how to get acsess to physical size of NexUs data set and optimal chunk size should be physical Nexus chunk size
    //*/ 
    ////void setChunkSize(size_t chunkSize)
    ////{
    ////  m_DataChunk = chunkSize;
    //// }
      //m_DataChunk = 10000;


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



//
//  void BoxController::prepareEventNexusData(::NeXus::File * file, const size_t chunkSize,const size_t nColumns,const std::string &descr)
//  {
//      std::vector<int> dims(2,0);
//      dims[0] = NX_UNLIMITED;
//      // One point per dimension, plus signal, plus error, plus runIndex, plus detectorID = nd+4
//      dims[1] = int(nColumns);
//
//      // Now the chunk size.
//      std::vector<int> chunk(dims);
//      chunk[0] = int(chunkSize);
//
//      // Make and open the data
//#ifdef COORDT_IS_FLOAT
//      file->makeCompData("event_data", ::NeXus::FLOAT32, dims, ::NeXus::NONE, chunk, true);
//#else
//      file->makeCompData("event_data", ::NeXus::FLOAT64, dims, ::NeXus::NONE, chunk, true);
//#endif
//
//      // A little bit of description for humans to read later
//      file->putAttr("description", descr);
//
//  }
//

}
}