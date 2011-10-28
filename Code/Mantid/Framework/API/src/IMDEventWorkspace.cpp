#include "MantidGeometry/MDGeometry/IMDDimension.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidKernel/IPropertyManager.h"

using Mantid::coord_t;
using namespace Mantid::Geometry;

namespace Mantid
{
namespace API
{

  //-----------------------------------------------------------------------------------------------
  /** Empty constructor */
  IMDEventWorkspace::IMDEventWorkspace()
  : IMDWorkspace()
  {
  }

  //-----------------------------------------------------------------------------------------------
  /** Copy constructor */
  IMDEventWorkspace::IMDEventWorkspace(const IMDEventWorkspace & other)
  : IMDWorkspace(other)
  {
    // Do a deep copy of ExperimentInfo's
    for (size_t i=0; i<other.m_expInfos.size(); i++)
    {
      ExperimentInfo_sptr copy(new ExperimentInfo(*other.m_expInfos[i]));
      m_expInfos.push_back(copy);
    }
  }


  //-----------------------------------------------------------------------------------------------
  /** Get the ExperimentInfo for the given run Index
   *
   * @param runIndex :: 0-based index of the run to get.
   * @return shared ptr to the ExperimentInfo class
   */
  ExperimentInfo_sptr IMDEventWorkspace::getExperimentInfo(const uint16_t runIndex)
  {
    if (size_t(runIndex) >= m_expInfos.size())
      throw std::invalid_argument("MDEventWorkspace::getExperimentInfo(): runIndex is out of range.");
    return m_expInfos[runIndex];
  }

  //-----------------------------------------------------------------------------------------------
  /** Get the ExperimentInfo for the given run Index
   *
   * @param runIndex :: 0-based index of the run to get.
   * @return shared ptr to the ExperimentInfo class
   */
  ExperimentInfo_const_sptr IMDEventWorkspace::getExperimentInfo(const uint16_t runIndex) const
  {
    if (size_t(runIndex) >= m_expInfos.size())
      throw std::invalid_argument("MDEventWorkspace::getExperimentInfo(): runIndex is out of range.");
    return m_expInfos[runIndex];
  }

  //-----------------------------------------------------------------------------------------------
  /** Add a new ExperimentInfo to this MDEventWorkspace
   *
   * @param ei :: shared ptr to the ExperimentInfo class to add
   * @return the runIndex at which it was added
   */
  uint16_t IMDEventWorkspace::addExperimentInfo(ExperimentInfo_sptr ei)
  {
    m_expInfos.push_back(ei);
    return uint16_t(m_expInfos.size()-1);
  }

  //-----------------------------------------------------------------------------------------------
  /** Replace the ExperimentInfo entry at a given place
   *
   * @param runIndex :: 0-based index of the run to replace
   * @param ei :: shared ptr to the ExperimentInfo class to add
   */
  void IMDEventWorkspace::setExperimentInfo(const uint16_t runIndex, ExperimentInfo_sptr ei)
  {
    if (size_t(runIndex) >= m_expInfos.size())
      throw std::invalid_argument("MDEventWorkspace::setExperimentInfo(): runIndex is out of range.");
    m_expInfos[runIndex] = ei;
  }

  //-----------------------------------------------------------------------------------------------
  /// @return the number of ExperimentInfo's in this workspace
  uint16_t IMDEventWorkspace::getNumExperimentInfo() const
  {
    return uint16_t(m_expInfos.size());
  }




  //-----------------------------------------------------------------------------------------------
  /** @return the marker set to true when a file-backed workspace needs its back-end file updated (by calling SaveMD(UpdateFileBackEnd=1) )
   */
  bool IMDEventWorkspace::fileNeedsUpdating() const
  {
    return m_fileNeedsUpdating;
  }

  //-----------------------------------------------------------------------------------------------
  /** Sets the marker set to true when a file-backed workspace needs its back-end file updated (by calling SaveMD(UpdateFileBackEnd=1) )
   * @param value :: marker value
   */
  void IMDEventWorkspace::setFileNeedsUpdating(bool value)
  {
	  m_fileNeedsUpdating = value;
  }



}//namespace MDEvents

}//namespace Mantid






namespace Mantid
{
namespace Kernel
{
  /** In order to be able to cast PropertyWithValue classes correctly a definition for the PropertyWithValue<IMDEventWorkspace> is required */
  template<> MANTID_API_DLL
  Mantid::API::IMDEventWorkspace_sptr IPropertyManager::getValue<Mantid::API::IMDEventWorkspace_sptr>(const std::string &name) const
  {
    PropertyWithValue<Mantid::API::IMDEventWorkspace_sptr>* prop =
                      dynamic_cast<PropertyWithValue<Mantid::API::IMDEventWorkspace_sptr>*>(getPointerToProperty(name));
    if (prop)
    {
      return *prop;
    }
    else
    {
      std::string message = "Attempt to assign property "+ name +" to incorrect type. Expected IMDEventWorkspace.";
      throw std::runtime_error(message);
    }
  }

  /** In order to be able to cast PropertyWithValue classes correctly a definition for the PropertyWithValue<IMDEventWorkspace> is required */
  template<> MANTID_API_DLL
  Mantid::API::IMDEventWorkspace_const_sptr IPropertyManager::getValue<Mantid::API::IMDEventWorkspace_const_sptr>(const std::string &name) const
  {
    PropertyWithValue<Mantid::API::IMDEventWorkspace_const_sptr>* prop =
                      dynamic_cast<PropertyWithValue<Mantid::API::IMDEventWorkspace_const_sptr>*>(getPointerToProperty(name));
    if (prop)
    {
      return prop->operator()();
    }
    else
    {
      std::string message = "Attempt to assign property "+ name +" to incorrect type. Expected const IMDEventWorkspace.";
      throw std::runtime_error(message);
    }
  }

} // namespace Kernel
} // namespace Mantid

