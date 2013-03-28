#include "MantidMDEvents/MDBoxSaveable.h"
#include "MantidMDEvents/MDBox.h"

namespace Mantid
{
namespace MDEvents
{
   //using Mantid::Kernel::DiskBuffer;

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
 /** Physical save the whole data. Tries to load any previous data from HDD 
  *  Called from the DiskBuffer.
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

  }
 
/** Loads the data from HDD if these data were not loaded before. */
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
//  //      std::cout << "MDBox ID " << this->getId() << " being saved." << std::endl;
//
//
//   // this aslo indirectly checks if the object knows its place (may be wrong place but no checks for that here)
//   if (this->wasSaved())
//   {
//     //TODO: redesighn const_cast
//    
//      MDBox<MDE,nd> *loader = const_cast<MDBox<MDE,nd> *>(this);
//      loader->load();  
//   
//
//      // This is the new size of the event list, possibly appended (if used AddEvent) or changed otherwise (non-const access)
//      if (data.size() > 0)
//      {
//
//         // Save at the ISaveable specified place
//          this->saveNexus(this->m_BoxController->getFile());
//      }
//   } 
//   else
//     if(data.size()>0)  throw std::runtime_error(" Attempt to save undefined event");
//   
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
//    /** Static method to save a vector of MDEvents of this type to a nexus file
//     * open to the right group.
//     * This method plops the events as a slab at a particular point in an already created array.
//     * The data block MUST be already open.
//     *
//     * This will be re-implemented by any other MDLeanEvent-like type.
//     *
//     * @param events :: reference to the vector of events to save.
//     * @param file :: open NXS file.
//     * @param startIndex :: index in the array to start saving to
//     * @param[out] totalSignal :: returns the integrated signal of all events
//     * @param[out] totalErrorSquared :: returns the integrated squared error of all events
//     * */
//    static void saveVectorToNexusSlab(const std::vector<MDLeanEvent<nd> > & events, ::NeXus::File * file, const uint64_t startIndex,
//        signal_t & totalSignal, signal_t & totalErrorSquared)
//    {
//      size_t numEvents = events.size();
//      size_t numColumns = nd+2;
//      coord_t * data = new coord_t[numEvents*numColumns];
//
//      totalSignal = 0;
//      totalErrorSquared = 0;
//
//      size_t index = 0;
//      typename std::vector<MDLeanEvent<nd> >::const_iterator it = events.begin();
//      typename std::vector<MDLeanEvent<nd> >::const_iterator it_end = events.end();
//      for (; it != it_end; ++it)
//      {
//        const MDLeanEvent<nd> & event = *it;
//        float signal = event.signal;
//        float errorSquared = event.errorSquared;
//        data[index++] = static_cast<coord_t>(signal);
//        data[index++] = static_cast<coord_t>(errorSquared);
//        for(size_t d=0; d<nd; d++)
//          data[index++] = event.center[d];
//        // Track the total signal
//        totalSignal += signal_t(signal);
//        totalErrorSquared += signal_t(errorSquared);
//      }
//
//      putDataInNexus(file, data, startIndex, numEvents, numColumns);
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
//    //---------------------------------------------------------------------------------------------
//    /** Static method to load part of a HDF block into a vector of MDEvents.
//     * The data block MUST be already open, using e.g. openNexusData()
//     *
//     * This will be re-implemented by any other MDLeanEvent-like type.
//     *
//     * @param events :: reference to the vector of events to load. This is NOT cleared by the method before loading.
//     * @param file :: open NXS file.
//     * @param indexStart :: index (in events) in the data field to start at
//     * @param numEvents :: number of events to load.
//     * */
//    static void loadVectorFromNexusSlab(std::vector<MDLeanEvent<nd> > & events, ::NeXus::File * file,
//        uint64_t indexStart, uint64_t numEvents)
//    {
//      if (numEvents == 0)
//        return;
//
//      // Number of columns = number of dimensions + 2 (signal/error)
//      size_t numColumns = nd+2;
//      // Load the data
//      coord_t * data = getDataFromNexus(file, indexStart, numEvents, numColumns);
//
//      // Reserve the amount of space needed. Significant speed up (~30% thanks to this)
//      events.reserve( events.size() + numEvents);
//      for (size_t i=0; i<numEvents; i++)
//      {
//        // Index into the data array
//        size_t ii = i*numColumns;
//
//        // Point directly into the data block for the centers.
//        coord_t * centers = data + ii+2;
//
//        // Create the event with signal, error squared, and the centers
//        events.push_back( MDLeanEvent<nd>(float(data[ii]), float(data[ii + 1]), centers) );
//      }
//
//      // Release the memory (all has been COPIED into MDLeanEvent's)
//      delete [] data;
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
//  //---------------------------------------------------------------------------------------------
//    /** Open the NXS data blocks for loading.
//     * The data should have been created before.
//     *
//     * @param file :: open NXS file.
//     * @return the number of events currently in the data field.
//     */
//    uint64_t BoxController::openEventNexusData(::NeXus::File * file)
//    {
//      // Open the data
//      file->openData("event_data");
//      // Return the size of dimension 0 = the number of events in the field
//      return uint64_t(file->getInfo().dims[0]);
//    }
//    void BoxController::closeNexusData(::NeXus::File * file)
//    {
//      file->closeData();
//    }


///**TODO: this should not be here, refactor out*/ 
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