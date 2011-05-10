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
    detectorIDs = inst->getDetectorIDs(true);

    // Init the Workspace2D with one spectrum per detector, in the same order.
    this->init(int(detectorIDs.size()), 1, 1);

    // Make the mapping, which will be used for speed later.
    detID_to_WI.clear();
    for (size_t wi=0; wi<detectorIDs.size(); wi++)
      detID_to_WI[detectorIDs[wi]] = int(wi);

    // Copy the instrument
    this->setInstrument( inst );

    // Initialize the spectra-det-map
    m_spectramap.access().populateWithVector(detectorIDs);

    // Make a simple 1-1 workspaceIndex to spectrumNumber axis.
    SpectraAxis * ax1 = dynamic_cast<SpectraAxis *>(this->m_axes[1]);
    ax1->populateSimple(int(detectorIDs.size()));
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


  //----------------------------------------------------------------------------------------------
  /** Non-const access to the spectra map is disallowed!
   * Always throws std::runtime_error
   * @return nothing, it always throws.
   */
  SpectraDetectorMap& SpecialWorkspace2D::mutableSpectraMap()
  {
    throw std::runtime_error("Non-const access to the spectra map in a SpecialWorkspace2D is disallowed!");
  }

  //----------------------------------------------------------------------------------------------
  /** Return the special value (Y) in the workspace at the given detector ID
   *
   * @param detectorID :: detector ID to look up
   * @return the Y value for that detector ID.
   * @throw std::invalid_argument if the detector ID was not found
   */
  double SpecialWorkspace2D::getValue(const int detectorID) const
  {
    std::map<int,int>::const_iterator it = detID_to_WI.find(detectorID);
    if (it == detID_to_WI.end())
      throw std::invalid_argument("SpecialWorkspace2D::getValue(): Invalid detectorID provided.");
    else
    {
      return this->dataY(it->second)[0];
    }
  }

  //----------------------------------------------------------------------------------------------
  /** Return the special value (Y) in the workspace at the given detector ID,
   * but returns a default value instead of throwing if detector is not found.
   *
   * @param detectorID :: detector ID to look up
   * @param defaultValue :: value returned if the ID is not found.
   * @return the Y value for that detector ID.
   */
  double SpecialWorkspace2D::getValue(const int detectorID, const double defaultValue) const
  {
    std::map<int,int>::const_iterator it = detID_to_WI.find(detectorID);
    if (it == detID_to_WI.end())
      return defaultValue;
    else
    {
      return this->dataY(it->second)[0];
    }
  }


  //----------------------------------------------------------------------------------------------
  /** Return the special value (Y) in the workspace at the given detector ID
   *
   * @param detectorID :: detector ID to look up
   * @return the Y value for that detector ID.
   * @throw std::invalid_argument if the detector ID was not found
   */
  void SpecialWorkspace2D::setValue(const int detectorID, double value)
  {
    std::map<int,int>::iterator it = detID_to_WI.find(detectorID);
    if (it == detID_to_WI.end())
      throw std::invalid_argument("SpecialWorkspace2D::setValue(): Invalid detectorID provided.");
    else
    {
      this->dataY(it->second)[0] = value;
    }
  }

  //----------------------------------------------------------------------------------------------
  /** Return the detector ID at the given workspace index
   *
   * @param workspaceIndex
   * @return
   */
  int SpecialWorkspace2D::getDetectorID(const int workspaceIndex) const
  {
    if (size_t(workspaceIndex) > detectorIDs.size())
      throw std::invalid_argument("SpecialWorkspace2D::getDetectorID(): Invalid workspaceIndex given.");
    return detectorIDs[workspaceIndex];
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
