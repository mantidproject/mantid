#include "MantidMDEvents/MDBoxSaveable.h"
#include "MantidMDEvents/MDBox.h"

namespace Mantid
{
namespace MDEvents
{
   MDBoxSaveable::MDBoxSaveable(API::IMDNode *const, size_t ID)
   {
   }

 //-----------------------------------------------------------------------------------------------
 /** Call to save the data (if needed) and release the memory used.
  *  Called from the DiskBuffer.
  *  If called directly presumes to know its file location and [TODO: refactor this] needs the file to be open correctly on correct group 
  */
  void MDBoxSaveable::save()
  {
//  //      std::cout << "MDBox ID " << this->getId() << " being saved." << std::endl;
//
//
//   // this aslo indirectly checks if the object knows its place (may be wrong place but no checks for that here)
//   if (this->wasSaved())
//   {
//     //TODO: redesighn const_cast
//    // This will load and append events ONLY if needed.
//      MDBox<MDE,nd> *loader = const_cast<MDBox<MDE,nd> *>(this);
//      loader->load();  // this will set isLoaded to true if not already loaded;
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
  }
//
 void MDBoxSaveable::load()
 {
//    // Is the data in memory right now (cached copy)?
//    if (!m_isLoaded)
//    {
//      // Perform the data loading
//      ::NeXus::File * file = this->m_BoxController->getFile();
//      if (file)
//      {
//        // Mutex for disk access (prevent read/write at the same time)
//        Kernel::RecursiveMutex & mutex = this->m_BoxController->getDiskBuffer().getFileMutex();
//        mutex.lock();
//        // Note that this APPENDS any events to the existing event list
//        //  (in the event that addEvent() was called for a box that was on disk)
//        try
//        {
//          uint64_t fileIndexStart = this->getFilePosition();
//          uint64_t fileNumEvents  = this->getFileSize();
//          MDE::loadVectorFromNexusSlab(data, file,fileIndexStart, fileNumEvents);
//          m_isLoaded = true;
//          mutex.unlock();
//        }
//        catch (std::exception &e)
//        {
//          mutex.unlock();
//          throw e;
//        }
//      }
//    }
  }



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
//    static void saveVectorToNexusSlab(const std::vector<MDEvent<nd> > & events, ::NeXus::File * file, const uint64_t startIndex,
//        signal_t & totalSignal, signal_t & totalErrorSquared)
//    {
//      size_t numEvents = events.size();
//      if(numEvents==0)return;
//      size_t numColumns = nd+4;
//      coord_t * data = new coord_t[numEvents*numColumns];
//
//      double SignalSum(0);
//      double ErrorSqSum(0);
//
//      size_t index = 0;
//      typename std::vector<MDEvent<nd> >::const_iterator it = events.begin();
//      typename std::vector<MDEvent<nd> >::const_iterator it_end = events.end();
//      for (; it != it_end; ++it)
//      {
//        const MDEvent<nd> & event = *it;
//        float signal = event.signal;
//        float errorSquared = event.errorSquared;
//        data[index++] = static_cast<coord_t>(signal);
//        data[index++] = static_cast<coord_t>(errorSquared);
//        // Additional stuff for MDEvent
//        data[index++] = static_cast<coord_t>(event.runIndex);
//        data[index++] = static_cast<coord_t>(event.detectorId);
//        for(size_t d=0; d<nd; d++)
//          data[index++] = event.center[d];
//        // Track the total signal
//        SignalSum += double(signal);
//        ErrorSqSum += double(errorSquared);
//      }
//      totalSignal = signal_t(SignalSum);
//      totalErrorSquared = signal_t(ErrorSqSum);
//
//      MDLeanEvent<nd>::putDataInNexus(file, data, startIndex, numEvents, numColumns);
//     }
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
//    static void loadVectorFromNexusSlab(std::vector<MDEvent<nd> > & events, ::NeXus::File * file, uint64_t indexStart, uint64_t numEvents)
//    {
//      if (numEvents == 0)
//        return;
//
//      // Number of columns = number of dimensions + 4 (signal/error/detId/runIndex)
//      size_t numColumns = nd+4;
//      // Load the data
//      coord_t * data = MDLeanEvent<nd>::getDataFromNexus(file, indexStart, numEvents, numColumns);
//
//      // Reserve the amount of space needed. Significant speed up (~30% thanks to this)
//      events.reserve( events.size() + numEvents);
//      for (size_t i=0; i<numEvents; i++)
//      {
//        // Index into the data array
//        size_t ii = i*numColumns;
//
//        // Point directly into the data block for the centers.
//        coord_t * centers = data + ii+4;
//
//        // Create the event with signal, error squared,
//        // the runIndex, the detectorID, and the centers
//        events.push_back( MDEvent<nd>(float(data[ii]), float(data[ii + 1]),
//            uint16_t(data[ii + 2]), int32_t(data[ii+3]), centers) );
//      }
//
//      // Release the memory (all has been COPIED into MDLeanEvent's)
//      delete [] data;
//    }
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

//
//  };


}
}