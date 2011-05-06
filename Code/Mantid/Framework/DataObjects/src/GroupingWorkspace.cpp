#include "MantidDataObjects/GroupingWorkspace.h"
#include "MantidKernel/System.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "MantidAPI/SpectraAxis.h"

using Mantid::API::SpectraAxis;

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
  {
    // Get all the detectors IDs
    std::vector<int> detIDs = inst->getDetectorIDs(true);

    // Init the Workspace2D with one spectrum per detector
    this->init(detIDs.size(), 1, 1);

    // Copy the instrument
    this->setInstrument( inst );

    // Initialize the spectra-det-map
    this->mutableSpectraMap().populateWithVector(detIDs);

    // Make a simple 1-1 workspaceIndex to spectrumNumber axis.
    SpectraAxis * ax1 = dynamic_cast<SpectraAxis *>(this->m_axes[1]);
    ax1->populateSimple(detIDs.size());
  }

  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  GroupingWorkspace::~GroupingWorkspace()
  {
  }
  

  //----------------------------------------------------------------------------------------------
  /** Sets the size of the workspace and initializes arrays to zero
  *  @param NVectors :: The number of vectors/histograms/detectors in the workspace
  *  @param XLength :: Must be 1
  *  @param YLength :: Must be 1
  */
  void GroupingWorkspace::init(const int &NVectors, const int &XLength, const int &YLength)
  {
    if ((XLength != 1) || (YLength != 1))
      throw std::invalid_argument("GroupingWorkspace must have 'spectra' of length 1 only.");
    // Continue with standard initialization
    Workspace2D::init(NVectors, XLength, YLength);
  }




} // namespace Mantid
} // namespace DataObjects

