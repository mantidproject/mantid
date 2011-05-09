#include "MantidDataObjects/SpecialWorkspace2D.h"
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
  DECLARE_WORKSPACE(SpecialWorkspace2D)


  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  SpecialWorkspace2D::SpecialWorkspace2D()
  {
  }

  //----------------------------------------------------------------------------------------------
  /** Constructor, building from an instrument
   *
   * @param inst :: input instrument that is the base for this workspace
   * @return created SpecialWorkspace2D
   */
  SpecialWorkspace2D::SpecialWorkspace2D(Mantid::Geometry::IInstrument_sptr inst)
  {
    // Get all the detectors IDs
    std::vector<int> detIDs = inst->getDetectorIDs(true);

    // Init the Workspace2D with one spectrum per detector
    this->init(int(detIDs.size()), 1, 1);

    // Copy the instrument
    this->setInstrument( inst );

    // Initialize the spectra-det-map
    this->mutableSpectraMap().populateWithVector(detIDs);

    // Make a simple 1-1 workspaceIndex to spectrumNumber axis.
    SpectraAxis * ax1 = dynamic_cast<SpectraAxis *>(this->m_axes[1]);
    ax1->populateSimple(int(detIDs.size()));
  }

  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  SpecialWorkspace2D::~SpecialWorkspace2D()
  {
  }
  

  //----------------------------------------------------------------------------------------------
  /** Sets the size of the workspace and initializes arrays to zero
  *  @param NVectors :: The number of vectors/histograms/detectors in the workspace
  *  @param XLength :: Must be 1
  *  @param YLength :: Must be 1
  */
  void SpecialWorkspace2D::init(const int &NVectors, const int &XLength, const int &YLength)
  {
    if ((XLength != 1) || (YLength != 1))
      throw std::invalid_argument("SpecialWorkspace2D must have 'spectra' of length 1 only.");
    // Continue with standard initialization
    Workspace2D::init(NVectors, XLength, YLength);
  }




} // namespace Mantid
} // namespace DataObjects


///\cond TEMPLATE

namespace Mantid
{
  namespace Kernel
  {

    template<> DLLExport
    Mantid::DataObjects::SpecialWorkspace2D_sptr IPropertyManager::getValue<Mantid::DataObjects::SpecialWorkspace2D_sptr>(const std::string &name) const
    {
      PropertyWithValue<Mantid::DataObjects::SpecialWorkspace2D_sptr>* prop =
        dynamic_cast<PropertyWithValue<Mantid::DataObjects::SpecialWorkspace2D_sptr>*>(getPointerToProperty(name));
      if (prop)
      {
        return *prop;
      }
      else
      {
        std::string message = "Attempt to assign property "+ name +" to incorrect type. Expected SpecialWorkspace2D.";
        throw std::runtime_error(message);
      }
    }

    template<> DLLExport
    Mantid::DataObjects::SpecialWorkspace2D_const_sptr IPropertyManager::getValue<Mantid::DataObjects::SpecialWorkspace2D_const_sptr>(const std::string &name) const
    {
      PropertyWithValue<Mantid::DataObjects::SpecialWorkspace2D_sptr>* prop =
        dynamic_cast<PropertyWithValue<Mantid::DataObjects::SpecialWorkspace2D_sptr>*>(getPointerToProperty(name));
      if (prop)
      {
        return prop->operator()();
      }
      else
      {
        std::string message = "Attempt to assign property "+ name +" to incorrect type. Expected const SpecialWorkspace2D.";
        throw std::runtime_error(message);
      }
    }

  } // namespace Kernel
} // namespace Mantid
