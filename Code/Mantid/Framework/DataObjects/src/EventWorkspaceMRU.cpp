#include "MantidDataObjects/EventWorkspaceMRU.h"
#include "MantidKernel/System.h"

namespace Mantid
{
namespace DataObjects
{


  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  EventWorkspaceMRU::EventWorkspaceMRU()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  EventWorkspaceMRU::~EventWorkspaceMRU()
  {
    //Make sure you free up the memory in the MRUs
    for (size_t i=0; i < m_bufferedDataY.size(); i++)
      if (m_bufferedDataY[i])
      {
        m_bufferedDataY[i]->clear();
        delete m_bufferedDataY[i];
      };
    m_bufferedDataY.clear();

    for (size_t i=0; i < m_bufferedDataE.size(); i++)
      if (m_bufferedDataE[i])
      {
        m_bufferedDataE[i]->clear();
        delete m_bufferedDataE[i];
      };
    m_bufferedDataE.clear();

  }
  

  //---------------------------------------------------------------------------
  /** This function makes sure that there are enough data
   * buffers (MRU's) for E for the number of threads requested.
   * @param thread_num :: thread number that wants a MRU buffer
   */
  void EventWorkspaceMRU::ensureEnoughBuffersE(size_t thread_num) const
  {
    PARALLEL_CRITICAL(EventWorkspace_MRUE_access)
    {
      if (m_bufferedDataE.size() <= thread_num)
      {
        m_bufferedDataE.resize(thread_num+1, NULL);
        for (size_t i=0; i < m_bufferedDataE.size(); i++)
        {
          if (!m_bufferedDataE[i])
            m_bufferedDataE[i] = new mru_list(50); //Create a MRU list with this many entries.

        }
      }
    }
  }
  //---------------------------------------------------------------------------
  /** This function makes sure that there are enough data
   * buffers (MRU's) for Y for the number of threads requested.
   * @param thread_num :: thread number that wants a MRU buffer
   */
  void EventWorkspaceMRU::ensureEnoughBuffersY(size_t thread_num) const
  {
    PARALLEL_CRITICAL(EventWorkspace_MRUY_access)
    {
      if (m_bufferedDataY.size() <= thread_num)
      {
        m_bufferedDataY.resize(thread_num+1, NULL);
        for (size_t i=0; i < m_bufferedDataY.size(); i++)
        {
          if (!m_bufferedDataY[i])
            m_bufferedDataY[i] = new mru_list(50); //Create a MRU list with this many entries.

        }
      }
    }
  }

  //---------------------------------------------------------------------------
  /// Clear all the data in the MRU buffers
  void EventWorkspaceMRU::clear()
  {
    // (Make this call thread safe)
    PARALLEL_CRITICAL(EventWorkspace_MRUY_access)
      {
      //Make sure you free up the memory in the MRUs
      for (size_t i=0; i < m_bufferedDataY.size(); i++)
        if (m_bufferedDataY[i])
        {
          m_bufferedDataY[i]->clear();
        };
      }

    PARALLEL_CRITICAL(EventWorkspace_MRUE_access)
    {
      for (size_t i=0; i < m_bufferedDataE.size(); i++)
        if (m_bufferedDataE[i])
        {
          m_bufferedDataE[i]->clear();
        };
    }
  }

  //---------------------------------------------------------------------------
  /** Find a Y histogram in the MRU
   *
   * @param thread_num :: number of the thread in which this is run
   * @param index :: index of the data to return
   * @return pointer to the MantidVecWithMarker that has the data; NULL if not found.
   */
  MantidVecWithMarker * EventWorkspaceMRU::findY(size_t thread_num, size_t index)
  {
    MantidVecWithMarker * out;
    PARALLEL_CRITICAL(EventWorkspace_MRUY_access)
    {
      out = m_bufferedDataY[thread_num]->find(index);
    }
    return out;
  }

  /** Find a Y histogram in the MRU
   *
   * @param thread_num :: number of the thread in which this is run
   * @param index :: index of the data to return
   * @return pointer to the MantidVecWithMarker that has the data; NULL if not found.
   */
  MantidVecWithMarker * EventWorkspaceMRU::findE(size_t thread_num, size_t index)
  {
    MantidVecWithMarker * out;
    PARALLEL_CRITICAL(EventWorkspace_MRUY_access)
    {
      out = m_bufferedDataE[thread_num]->find(index);
    }
    return out;
  }

  /** Insert a new histogram into the MRU
   *
   * @param thread_num :: thread being accessed
   * @param data :: the new data
   * @return a MantidVecWithMarker * that needs to be deleted, or NULL if nothing needs to be deleted.
   */
  MantidVecWithMarker * EventWorkspaceMRU::insertY(size_t thread_num, MantidVecWithMarker * data)
  {
    MantidVecWithMarker * out;
    PARALLEL_CRITICAL(EventWorkspace_MRUY_access)
    {
      out = m_bufferedDataY[thread_num]->insert(data);
    }
    return out;
  }

  /** Insert a new histogram into the MRU
   *
   * @param thread_num :: thread being accessed
   * @param data :: the new data
   * @return a MantidVecWithMarker * that needs to be deleted, or NULL if nothing needs to be deleted.
   */
  MantidVecWithMarker * EventWorkspaceMRU::insertE(size_t thread_num, MantidVecWithMarker * data)
  {
    MantidVecWithMarker * out;
    PARALLEL_CRITICAL(EventWorkspace_MRUE_access)
    {
      out = m_bufferedDataE[thread_num]->insert(data);
    }
    return out;
  }


  /** Delete any entries in the MRU at the given index
   *
   * @param index :: index to delete.
   */
  void EventWorkspaceMRU::deleteIndex(size_t index)
  {
    PARALLEL_CRITICAL(EventWorkspace_MRUE_access)
    {
      for (size_t i=0; i < m_bufferedDataE.size(); i++)
        if (m_bufferedDataE[i])
          m_bufferedDataE[i]->deleteIndex(index);
    }
    PARALLEL_CRITICAL(EventWorkspace_MRUY_access)
    {
      for (size_t i=0; i < m_bufferedDataY.size(); i++)
        if (m_bufferedDataY[i])
          m_bufferedDataY[i]->deleteIndex(index);
    }

  }


} // namespace Mantid
} // namespace DataObjects

