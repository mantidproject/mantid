#include "MantidDataObjects/GroupingWorkspace.h"
#include "MantidKernel/System.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "MantidAPI/SpectraAxis.h"

using Mantid::API::SpectraAxis;
using Mantid::API::SpectraDetectorMap;

using std::size_t;
using namespace Mantid::API;

namespace Mantid
{
namespace DataObjects
{
  //Register the workspace
  DECLARE_WORKSPACE(GroupingWorkspace)

  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  GroupingWorkspace::GroupingWorkspace()
  {
  }

  //----------------------------------------------------------------------------------------------
  /** Constructor, building from an instrument
   *
   * @param inst :: input instrument that is the base for this workspace
   * @return created GroupingWorkspace
   */
  GroupingWorkspace::GroupingWorkspace(Mantid::Geometry::IInstrument_sptr inst)
  : SpecialWorkspace2D(inst)
  {
  }

  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  GroupingWorkspace::~GroupingWorkspace()
  {
  }
  

  //----------------------------------------------------------------------------------------------
  /** Fill a map with key = detector ID, value = group number
   * by using the values in Y.
   * Group values of 0 are converted to -1.
   *
   * @param detIDToGroup :: ref. to map to fill
   * @param[out] ngroups :: the number of groups found (equal to the largest group number found)
   */
  void GroupingWorkspace::makeDetectorIDToGroupMap(std::map<detid_t, int> & detIDToGroup, int64_t & ngroups) const
  {
    ngroups = 0;
    for (size_t wi=0; wi<this->m_noVectors; ++wi)
    {
      // Convert the Y value to a group number
      int group = static_cast<int>(this->dataY(wi)[0]);
      if (group == 0) group = -1;
      detid_t detID = detectorIDs[wi];
      detIDToGroup[detID] = group;
      if (group > ngroups)
        ngroups = group;
    }
  }



} // namespace Mantid
} // namespace DataObjects


///\cond TEMPLATE

namespace Mantid
{
  namespace Kernel
  {

    template<> DLLExport
    Mantid::DataObjects::GroupingWorkspace_sptr IPropertyManager::getValue<Mantid::DataObjects::GroupingWorkspace_sptr>(const std::string &name) const
    {
      PropertyWithValue<Mantid::DataObjects::GroupingWorkspace_sptr>* prop =
        dynamic_cast<PropertyWithValue<Mantid::DataObjects::GroupingWorkspace_sptr>*>(getPointerToProperty(name));
      if (prop)
      {
        return *prop;
      }
      else
      {
        std::string message = "Attempt to assign property "+ name +" to incorrect type. Expected GroupingWorkspace.";
        throw std::runtime_error(message);
      }
    }

    template<> DLLExport
    Mantid::DataObjects::GroupingWorkspace_const_sptr IPropertyManager::getValue<Mantid::DataObjects::GroupingWorkspace_const_sptr>(const std::string &name) const
    {
      PropertyWithValue<Mantid::DataObjects::GroupingWorkspace_sptr>* prop =
        dynamic_cast<PropertyWithValue<Mantid::DataObjects::GroupingWorkspace_sptr>*>(getPointerToProperty(name));
      if (prop)
      {
        return prop->operator()();
      }
      else
      {
        std::string message = "Attempt to assign property "+ name +" to incorrect type. Expected const GroupingWorkspace.";
        throw std::runtime_error(message);
      }
    }

  } // namespace Kernel
} // namespace Mantid

///\endcond TEMPLATE
