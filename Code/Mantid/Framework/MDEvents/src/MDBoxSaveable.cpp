#include "MantidMDEvents/MDBoxSaveable.h"
#include "MantidMDEvents/MDBox.h"

namespace Mantid
{
namespace MDEvents
{


  //-----------------------------------------------------------------------------------------------
  /** Call to save the data (if needed) and release the memory used.
   *  Called from the DiskBuffer.
   *  If called directly presumes to know its file location and [TODO: refactor this] needs the file to be open correctly on correct group 
   */
  TMDE(
  void MDBox)::save()const
  {
  //      std::cout << "MDBox ID " << this->getId() << " being saved." << std::endl;


   // this aslo indirectly checks if the object knows its place (may be wrong place but no checks for that here)
   if (this->wasSaved())
   {
     //TODO: redesighn const_cast
    // This will load and append events ONLY if needed.
      MDBox<MDE,nd> *loader = const_cast<MDBox<MDE,nd> *>(this);
      loader->load();  // this will set isLoaded to true if not already loaded;
   

      // This is the new size of the event list, possibly appended (if used AddEvent) or changed otherwise (non-const access)
      if (data.size() > 0)
      {

         // Save at the ISaveable specified place
          this->saveNexus(this->m_BoxController->getFile());
      }
   } 
   else
     if(data.size()>0)  throw std::runtime_error(" Attempt to save undefined event");
   
   
  }


  //-----------------------------------------------------------------------------------------------
  /** Load the box's Event data from an open nexus file.
   * The FileIndex start and numEvents must be set correctly already.
   * Clear existing data from memory!
   *
   * @param file :: Nexus File object, must already by opened with MDE::openNexusData()
   * @param setIsLoaded :: flag if box is loaded from file
   */
  TMDE(
  inline void MDBox)::loadNexus(::NeXus::File * file, bool setIsLoaded)
  {
    this->data.clear();
    uint64_t fileIndexStart = this->getFilePosition();
    uint64_t fileNumEvents  = this->getFileSize();
    if(fileIndexStart == std::numeric_limits<uint64_t>::max())
      throw(std::runtime_error("MDBox::loadNexus -- attempt to load box from undefined location"));
    MDE::loadVectorFromNexusSlab(this->data, file, fileIndexStart, fileNumEvents);

   
    this->m_isLoaded=setIsLoaded;
  
  }



 TMDE(
 inline void MDBox)::load()
 {
    // Is the data in memory right now (cached copy)?
    if (!m_isLoaded)
    {
      // Perform the data loading
      ::NeXus::File * file = this->m_BoxController->getFile();
      if (file)
      {
        // Mutex for disk access (prevent read/write at the same time)
        Kernel::RecursiveMutex & mutex = this->m_BoxController->getDiskBuffer().getFileMutex();
        mutex.lock();
        // Note that this APPENDS any events to the existing event list
        //  (in the event that addEvent() was called for a box that was on disk)
        try
        {
          uint64_t fileIndexStart = this->getFilePosition();
          uint64_t fileNumEvents  = this->getFileSize();
          MDE::loadVectorFromNexusSlab(data, file,fileIndexStart, fileNumEvents);
          m_isLoaded = true;
          mutex.unlock();
        }
        catch (std::exception &e)
        {
          mutex.unlock();
          throw e;
        }
      }
    }
  }

 //-----------------------------------------------------------------------------------------------
  /** Save the box's Event data to an open nexus file.
   *
   * @param file :: Nexus File object, must already by opened with MDE::prepareNexusData()
   */
  TMDE(
  inline void MDBox)::saveNexus(::NeXus::File * file) const
  {
    //std::cout << "Box " << this->getId() << " saving to " << m_fileIndexStart << std::endl;
    MDE::saveVectorToNexusSlab(this->data, file, this->getFilePosition(),
                               this->m_signal, this->m_errorSquared);

  }

}
}