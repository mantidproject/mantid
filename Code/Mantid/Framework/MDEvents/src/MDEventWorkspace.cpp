#include "MantidMDEvents/IMDBox.h"
#include "MantidMDEvents/MDBox.h"
#include "MantidMDEvents/MDGridBox.h"
#include "MantidMDEvents/MDEvent.h"
#include "MantidMDEvents/MDEventWorkspace.h"

namespace Mantid
{
namespace MDEvents
{

//-----------------------------------------------------------------------------------------------
  /** Default constructor
   */
  TMDE(
  MDEventWorkspace)::MDEventWorkspace()
  {
    data = new MDBox<MDE, nd>();
  }

  //-----------------------------------------------------------------------------------------------
  /** Destructor
   */
  TMDE(
  MDEventWorkspace)::~MDEventWorkspace()
  {
    delete data;
  }


  //-----------------------------------------------------------------------------------------------
  /** Perform initialization after dimensions (and others) have been set.
   * This sets the size of the box.
   */
  TMDE(
  void MDEventWorkspace)::initialize()
  {
    if (dimensions.size() != nd)
      throw std::runtime_error("MDEventWorkspace::initialize() called with an incorrect number of dimensions set. Use addDimension() first to add the right number of dimension info objects.");
    if (isGridBox())
        throw std::runtime_error("MDEventWorkspace::initialize() called on a MDEventWorkspace containing a MDGridBox. You should call initialize() before adding any events!");
    for (size_t d=0; d<nd; d++)
      data->setExtents(d, dimensions[d].getMin(), dimensions[d].getMax());
  }

  //-----------------------------------------------------------------------------------------------
  /** Get the data type (id) of the workspace */
  TMDE(
  const std::string MDEventWorkspace)::id() const
  {
    std::ostringstream out;
    out << "MDEventWorkspace<MDEvent," << getNumDims() << ">";
    return out.str();
  }


  //-----------------------------------------------------------------------------------------------
  /** Returns the number of dimensions in this workspace */
  TMDE(
  size_t MDEventWorkspace)::getNumDims() const
  {
    return nd;
  }

  //-----------------------------------------------------------------------------------------------
  /** Returns the total number of points (events) in this workspace */
  TMDE(
  size_t MDEventWorkspace)::getNPoints() const
  {
    return data->getNPoints();
  }

  //-----------------------------------------------------------------------------------------------
  /** Set the box controller that the contained GridBoxes will use
   *
   * @param controller :: BoxController_sptr
   */
  TMDE(
  void MDEventWorkspace)::setBoxController(BoxController_sptr controller)
  {
    m_BoxController = controller;
    data->m_BoxController = m_BoxController;
  }


  //-----------------------------------------------------------------------------------------------
  /** Returns the number of bytes of memory
   * used by the workspace. */
  TMDE(
  size_t MDEventWorkspace)::getMemorySize() const
  {
    return this->getNPoints() * sizeof(MDE);
  }


  //-----------------------------------------------------------------------------------------------
  /** Add a vector of MDEvents to the workspace.
   *
   * @param events :: const ref. to a vector of events; they will be copied.
   */
  TMDE(
  void MDEventWorkspace)::addEvents(const std::vector<MDE> & events, API::Progress * prog)
  {
    // Always split the MDBox into a grid box
    // TODO: Should this be a decision point?
    MDGridBox<MDE,nd> * gridBox = dynamic_cast<MDGridBox<MDE,nd> *>(data);
    if (!gridBox)
    {
      MDBox<MDE,nd> * box = dynamic_cast<MDBox<MDE,nd> *>(data);
      gridBox = new MDGridBox<MDE,nd>(box);
      data = gridBox;
    }
    // Do the add manye events one
    gridBox->addManyEvents(events, prog);
  }



}//namespace MDEvents

}//namespace Mantid

