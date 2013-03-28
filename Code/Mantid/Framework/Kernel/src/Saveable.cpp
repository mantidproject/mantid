#include "MantidKernel/Saveable.h"
#include "MantidKernel/System.h"
#include <limits>

namespace Mantid
{
    namespace Kernel
    {


        //----------------------------------------------------------------------------------------------
        /** Constructor
        */
        Saveable::Saveable()
            :ISaveable(),
            m_Busy(false),m_dataChanged(false),m_wasSaved(false),m_isLoaded(false)
        {
        }

        //----------------------------------------------------------------------------------------------
        /** Copy constructor --> big qusetions about the validity of such implementation
        */
        Saveable::Saveable(const Saveable & other)
            :ISaveable(other),
            m_Busy(other.m_Busy),m_dataChanged(other.m_dataChanged),m_wasSaved(other.m_wasSaved),m_isLoaded(false)
        {
        }

        /// @ set the data busy to prevent from removing them from memory. The process which does that should clean the data when finished with them
        void Saveable::setBusy(bool On)
        {
            m_Busy=On;
        }
        /** Call this method from the method which changes the object but keeps the object size the same to tell DiskBuffer to write it back
        the dataChanged ID is reset after save from the DataBuffer is emptied   */
        void Saveable::setDataChanged()
        { 
            if(this->wasSaved())m_dataChanged=true;
        }
        /** this method has to be called if the object has been discarded from memory and is not changed any more. 
        It expected to be called from clearDataFromMemory. */
        void Saveable::clearDataChanged()
        {
            m_dataChanged=false;
        }


        void Saveable::setLoaded(bool Yes)
        { m_isLoaded = Yes;}


        //----------------------------------------------------------------------------------------------
        /** Destructor
        */
        Saveable::~Saveable()
        {
        }



        /** Set the start/end point in the file where the events are located
        * @param newPos :: start point,
        * @param newSize :: number of events in the file   
        * @param wasSaved :: flag to mark if the info was saved, by default it does
        */
        void Saveable::setFilePosition(uint64_t newPos, size_t newSize, bool wasSaved)
        {  
            this->m_setter.lock();
            this->m_fileIndexStart=newPos;  
            this->m_fileNumEvents =static_cast<uint64_t>(newSize);
            m_wasSaved = wasSaved;
            this->m_setter.unlock();
        }




    } // namespace Mantid
} // namespace Kernel

