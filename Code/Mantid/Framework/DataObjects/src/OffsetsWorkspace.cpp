#include "MantidDataObjects/OffsetsWorkspace.h"
#include "MantidKernel/System.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "MantidAPI/SpectraAxis.h"

using Mantid::API::SpectraAxis;
using Mantid::API::SpectraDetectorMap;

namespace Mantid
{
namespace DataObjects
{
  //Register the workspace
  DECLARE_WORKSPACE(OffsetsWorkspace)

  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  OffsetsWorkspace::OffsetsWorkspace()
  {
  }

  //----------------------------------------------------------------------------------------------
  /** Constructor, building from an instrument
   *
   * @param inst :: input instrument that is the base for this workspace
   * @return created OffsetsWorkspace
   */
  OffsetsWorkspace::OffsetsWorkspace(Mantid::Geometry::IInstrument_sptr inst)
  : SpecialWorkspace2D(inst)
  {
  }

  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  OffsetsWorkspace::~OffsetsWorkspace()
  {
  }
  


} // namespace Mantid
} // namespace DataObjects


///\cond TEMPLATE

namespace Mantid
{
  namespace Kernel
  {

    template<> DLLExport
    Mantid::DataObjects::OffsetsWorkspace_sptr IPropertyManager::getValue<Mantid::DataObjects::OffsetsWorkspace_sptr>(const std::string &name) const
    {
      PropertyWithValue<Mantid::DataObjects::OffsetsWorkspace_sptr>* prop =
        dynamic_cast<PropertyWithValue<Mantid::DataObjects::OffsetsWorkspace_sptr>*>(getPointerToProperty(name));
      if (prop)
      {
        return *prop;
      }
      else
      {
        std::string message = "Attempt to assign property "+ name +" to incorrect type. Expected OffsetsWorkspace.";
        throw std::runtime_error(message);
      }
    }

    template<> DLLExport
    Mantid::DataObjects::OffsetsWorkspace_const_sptr IPropertyManager::getValue<Mantid::DataObjects::OffsetsWorkspace_const_sptr>(const std::string &name) const
    {
      PropertyWithValue<Mantid::DataObjects::OffsetsWorkspace_sptr>* prop =
        dynamic_cast<PropertyWithValue<Mantid::DataObjects::OffsetsWorkspace_sptr>*>(getPointerToProperty(name));
      if (prop)
      {
        return prop->operator()();
      }
      else
      {
        std::string message = "Attempt to assign property "+ name +" to incorrect type. Expected const OffsetsWorkspace.";
        throw std::runtime_error(message);
      }
    }

  } // namespace Kernel
} // namespace Mantid
