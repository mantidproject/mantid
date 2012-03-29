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
    m_bufferedDataY.clear();
    m_bufferedDataE.clear();
  }
  

  //---------------------------------------------------------------------------
  /// Clear all the data in the MRU buffers
  void EventWorkspaceMRU::clear()
  {
    m_bufferedDataY.clear();
    m_bufferedDataE.clear();
  }

  //---------------------------------------------------------------------------
  /** Find a Y histogram in the MRU
   *
   * @param thread_num :: number of the thread in which this is run
   * @param index :: index of the data to return
   * @return pointer to the MantidVecWithMarker that has the data; NULL if not found.
   */
  MantidVecWithMarker * EventWorkspaceMRU::findY(size_t index)
  {
    return m_bufferedDataY.find(index);
  }

  /** Find a Y histogram in the MRU
   *
   * @param thread_num :: number of the thread in which this is run
   * @param index :: index of the data to return
   * @return pointer to the MantidVecWithMarker that has the data; NULL if not found.
   */
  MantidVecWithMarker * EventWorkspaceMRU::findE(size_t index)
  {
    return m_bufferedDataE.find(index);
  }

  /** Insert a new histogram into the MRU
   *
   * @param thread_num :: thread being accessed
   * @param data :: the new data
   * @return a MantidVecWithMarker * that needs to be deleted, or NULL if nothing needs to be deleted.
   */
  MantidVecWithMarker * EventWorkspaceMRU::insertY(MantidVecWithMarker * data)
  {
    return m_bufferedDataY.insert(data);
  }

  /** Insert a new histogram into the MRU
   *
   * @param thread_num :: thread being accessed
   * @param data :: the new data
   * @return a MantidVecWithMarker * that needs to be deleted, or NULL if nothing needs to be deleted.
   */
  MantidVecWithMarker * EventWorkspaceMRU::insertE(MantidVecWithMarker * data)
  {
    return m_bufferedDataE.insert(data);
  }


  /** Delete any entries in the MRU at the given index
   *
   * @param index :: index to delete.
   */
  void EventWorkspaceMRU::deleteIndex(size_t index)
  {
    m_bufferedDataY.deleteIndex(index);
    m_bufferedDataE.deleteIndex(index);
  }


} // namespace Mantid
} // namespace DataObjects

