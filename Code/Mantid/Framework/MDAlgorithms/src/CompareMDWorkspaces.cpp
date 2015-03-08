#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/IMDWorkspace.h"
#include "MantidKernel/Strings.h"
#include "MantidKernel/System.h"
#include "MantidMDAlgorithms/CompareMDWorkspaces.h"
#include "MantidDataObjects/MDHistoWorkspace.h"
#include "MantidGeometry/MDGeometry/IMDDimension.h"
#include "MantidDataObjects/MDEventFactory.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;

//=============================================================================
/** Custom exception class to signal a failure
* in the comparison
*/
class CompareFailsException : public std::runtime_error {
public:
  CompareFailsException(const std::string &msg) : std::runtime_error(msg) {}
  ~CompareFailsException() throw() {}
  std::string getMessage() const { return this->what(); }
};

namespace Mantid {
namespace MDAlgorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(CompareMDWorkspaces)

//----------------------------------------------------------------------------------------------
/** Constructor
*/
CompareMDWorkspaces::CompareMDWorkspaces() {}

//----------------------------------------------------------------------------------------------
/** Destructor
*/
CompareMDWorkspaces::~CompareMDWorkspaces() {}

//----------------------------------------------------------------------------------------------
/// Algorithm's name for identification. @see Algorithm::name
const std::string CompareMDWorkspaces::name() const {
  return "CompareMDWorkspaces";
};

/// Algorithm's version for identification. @see Algorithm::version
int CompareMDWorkspaces::version() const { return 1; };

/// Algorithm's category for identification. @see Algorithm::category
const std::string CompareMDWorkspaces::category() const {
  return "MDAlgorithms";
}

//----------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
*/
void CompareMDWorkspaces::init() {
  declareProperty(
      new WorkspaceProperty<IMDWorkspace>("Workspace1", "", Direction::Input),
      "First MDWorkspace to compare.");
  declareProperty(
      new WorkspaceProperty<IMDWorkspace>("Workspace2", "", Direction::Input),
      "Second MDWorkspace to compare.");

  declareProperty(
      "Tolerance", 0.0,
      "The maximum amount by which values may differ between the workspaces.");
  declareProperty("CheckEvents", true, "Whether to compare each MDEvent. If "
                                       "False, will only look at the box "
                                       "structure.");

  declareProperty(
      new PropertyWithValue<bool>("Equals", false, Direction::Output),
      "Boolean set to true if the workspaces match.");
  declareProperty(
      new PropertyWithValue<std::string>("Result", "", Direction::Output),
      "String describing the difference found between the workspaces");
  declareProperty("IgnoreBoxID", false, "To ignore box ID-s when comparing MD "
                                        "boxes as Multithreaded splitting "
                                        "assigns box id-s randomly");
}

//----------------------------------------------------------------------------------------------
/** Return a string "(a vs b)".   */
template <typename T> std::string versus(T a, T b) {
  return "(" + Strings::toString(a) + " vs " + Strings::toString(b) + ")";
}

//----------------------------------------------------------------------------------------------
/** Compare a and b. Throw if they dont match
*
* @param a :: any type
* @param b :: any type
* @param message :: message for result
* @throw CompareFailsException if the two DONT match
*/
template <typename T>
void CompareMDWorkspaces::compare(T a, T b, const std::string &message) {
  if (a != b)
    throw CompareFailsException(message + " " + versus(a, b));
}

//----------------------------------------------------------------------------------------------
/** Compare a and b. Throw if they dont match within Tolerance
*
* @param a :: double
* @param b :: double
* @param message :: message for result
* @throw CompareFailsException if the two DONT match
*/
template <class T>
void CompareMDWorkspaces::compareTol(T a, T b, const std::string &message) {
  double diff = fabs(a - b);
  if (diff > m_tolerance) {
    double pa = fabs(a);
    double pb = fabs(b);
    if ((pa > 2 * m_tolerance) || (pb > 2 * m_tolerance)) {
      diff = 0.5 * diff / (pa + pb);
      if (diff > m_tolerance)
        throw CompareFailsException(message + " " + versus(a, b));
    } else
      throw CompareFailsException(message + " " + versus(a, b));
  }
}

//----------------------------------------------------------------------------------------------
/** Compare the dimensions etc. of two MDWorkspaces
*/
void
CompareMDWorkspaces::compareMDGeometry(Mantid::API::IMDWorkspace_sptr ws1,
                                       Mantid::API::IMDWorkspace_sptr ws2) {
  compare(ws1->getNumDims(), ws2->getNumDims(),
          "Workspaces have a different number of dimensions");
  for (size_t d = 0; d < ws1->getNumDims(); d++) {
    IMDDimension_const_sptr dim1 = ws1->getDimension(d);
    IMDDimension_const_sptr dim2 = ws2->getDimension(d);
    compare(dim1->getName(), dim2->getName(),
            "Dimension #" + Strings::toString(d) + " has a different name");
    compare(dim1->getUnits(), dim2->getUnits(),
            "Dimension #" + Strings::toString(d) + " has different units");
    compare(dim1->getNBins(), dim2->getNBins(),
            "Dimension #" + Strings::toString(d) +
                " has a different number of bins");
    compareTol(dim1->getMinimum(), dim2->getMinimum(),
               "Dimension #" + Strings::toString(d) +
                   " has a different minimum");
    compareTol(dim1->getMaximum(), dim2->getMaximum(),
               "Dimension #" + Strings::toString(d) +
                   " has a different maximum");
  }
}

//----------------------------------------------------------------------------------------------
/** Compare the dimensions etc. of two MDWorkspaces
*/
void CompareMDWorkspaces::compareMDHistoWorkspaces(
    Mantid::DataObjects::MDHistoWorkspace_sptr ws1,
    Mantid::DataObjects::MDHistoWorkspace_sptr ws2) {
  compare(ws1->getNumDims(), ws2->getNumDims(),
          "Workspaces have a different number of dimensions");
  compare(ws1->getNPoints(), ws2->getNPoints(),
          "Workspaces have a different number of points");
  for (size_t i = 0; i < ws1->getNPoints(); i++) {
    double diff = fabs(ws1->getSignalAt(i) - ws2->getSignalAt(i));
    if (diff > m_tolerance)
      throw CompareFailsException(
          "MDHistoWorkspaces have a different signal at index " +
          Strings::toString(i) + " " +
          versus(ws1->getSignalAt(i), ws2->getSignalAt(i)));

    double diffErr = fabs(ws1->getErrorAt(i) - ws2->getErrorAt(i));
    if (diffErr > m_tolerance)
      throw CompareFailsException(
          "MDHistoWorkspaces have a different error at index " +
          Strings::toString(i) + " " +
          versus(ws1->getErrorAt(i), ws2->getErrorAt(i)));
  }
}

//----------------------------------------------------------------------------------------------
/** Perform the comparison on MDEventWorkspaces
*
* @param ws ::  MDEventWorkspace to compare
*/
template <typename MDE, size_t nd>
void CompareMDWorkspaces::compareMDWorkspaces(
    typename MDEventWorkspace<MDE, nd>::sptr ws) {
  typename MDEventWorkspace<MDE, nd>::sptr ws1 = ws;
  typename MDEventWorkspace<MDE, nd>::sptr ws2 =
      boost::dynamic_pointer_cast<MDEventWorkspace<MDE, nd>>(inWS2);
  if (!ws1 || !ws2)
    throw std::runtime_error("Incompatible workspace types passed to PlusMD.");

  std::vector<API::IMDNode *> boxes1;
  std::vector<API::IMDNode *> boxes2;

  ws1->getBox()->getBoxes(boxes1, 1000, false);
  ws2->getBox()->getBoxes(boxes2, 1000, false);

  this->compare(boxes1.size(), boxes2.size(),
                "Workspaces do not have the same number of boxes");

  for (size_t j = 0; j < boxes1.size(); j++) {

    API::IMDNode *box1 = boxes1[j];
    API::IMDNode *box2 = boxes2[j];

    if (m_CompareBoxID)
      this->compare(box1->getID(), box2->getID(), "Boxes have different ID");
    else {
      if (box1->getID() != box2->getID())
        g_log.debug() << " Boxes N: " << j << " have box ID: " << box1->getID()
                      << " and " << box2->getID() << " correspondingly\n";
    }
    this->compare(size_t(box1->getDepth()), size_t(box2->getDepth()),
                  "Boxes are at a different depth");
    this->compare(box1->getNumChildren(), box2->getNumChildren(),
                  "Boxes do not have the same number of children");

    for (size_t i = 0; i < box1->getNumChildren(); i++) {
      if (m_CompareBoxID)
        this->compare(box1->getChild(i)->getID(), box2->getChild(i)->getID(),
                      "Child of boxes do not match IDs");
      else {
        if (box1->getID() != box2->getID())
          g_log.debug() << " Boxes N: " << j << " children N: " << i
                        << " have box ID: " << box1->getChild(i)->getID()
                        << " and " << box2->getChild(i)->getID()
                        << " correspondingly\n";
      }
    }

    for (size_t d = 0; d < nd; d++) {
      this->compareTol(box1->getExtents(d).getMin(),
                       box2->getExtents(d).getMin(),
                       "Extents of box do not match");
      this->compareTol(box1->getExtents(d).getMax(),
                       box2->getExtents(d).getMax(),
                       "Extents of box do not match");
    }
    this->compareTol(box1->getInverseVolume(), box2->getInverseVolume(),
                     "Box inverse volume does not match");
    this->compareTol(box1->getSignal(), box2->getSignal(),
                     "Box signal does not match");
    this->compareTol(box1->getErrorSquared(), box2->getErrorSquared(),
                     "Box error squared does not match");
    if (m_CheckEvents)
      this->compare(box1->getNPoints(), box2->getNPoints(),
                    "Number of points in box does not match");

    // Are both MDGridBoxes ?
    MDGridBox<MDE, nd> *gridbox1 = dynamic_cast<MDGridBox<MDE, nd> *>(box1);
    MDGridBox<MDE, nd> *gridbox2 = dynamic_cast<MDGridBox<MDE, nd> *>(box2);
    if (gridbox1 && gridbox2) {
      for (size_t d = 0; d < nd; d++)
        this->compareTol(gridbox1->getBoxSize(d), gridbox2->getBoxSize(d),
                         "Box sizes do not match");
    }

    // Are both MDBoxes (with events)
    MDBox<MDE, nd> *mdbox1 = dynamic_cast<MDBox<MDE, nd> *>(box1);
    MDBox<MDE, nd> *mdbox2 = dynamic_cast<MDBox<MDE, nd> *>(box2);
    if (mdbox1 && mdbox2) {
      if (m_CheckEvents) {
        const std::vector<MDE> &events1 = mdbox1->getConstEvents();
        const std::vector<MDE> &events2 = mdbox2->getConstEvents();
        try {
          this->compare(events1.size(), events2.size(),
                        "Box event vectors are not the same length");
          if (events1.size() == events2.size() && events1.size() > 2) {
            // Check first and last event
            for (size_t i = 0; i < events1.size(); i++) {
              for (size_t d = 0; d < nd; d++) {
                this->compareTol(events1[i].getCenter(d),
                                 events2[i].getCenter(d),
                                 "Event center does not match");
              }
              this->compareTol(events1[i].getSignal(), events2[i].getSignal(),
                               "Event signal does not match");
              this->compareTol(events1[i].getErrorSquared(),
                               events2[i].getErrorSquared(),
                               "Event error does not match");
            }
          }
        } catch (CompareFailsException &) {
          // Boxes must release events if the check fails
          mdbox1->releaseEvents();
          mdbox2->releaseEvents();
          throw;
        }
        mdbox1->releaseEvents();
        mdbox2->releaseEvents();
      } // Don't compare if BoxStructureOnly
    }   // is mdbox1
  }
}

//----------------------------------------------------------------------------------------------
/** Perform comparison, set m_result if not matching.
*/
void CompareMDWorkspaces::doComparison() {
  m_tolerance = getProperty("Tolerance");
  m_CheckEvents = getProperty("CheckEvents");

  IMDWorkspace_sptr ws1 = getProperty("Workspace1");
  IMDWorkspace_sptr ws2 = getProperty("Workspace2");
  inWS2 = ws2;
  if (!ws1 || !ws2)
    throw std::invalid_argument("Invalid workspace given.");

  MatrixWorkspace_sptr mws1 = boost::dynamic_pointer_cast<MatrixWorkspace>(ws1);
  MatrixWorkspace_sptr mws2 = boost::dynamic_pointer_cast<MatrixWorkspace>(ws2);
  if (mws1 || mws2)
    throw std::invalid_argument("Cannot compare MatrixWorkspaces. Please use "
                                "CheckWorkspacesMatch algorithm instead.");

  MDHistoWorkspace_sptr histo1 =
      boost::dynamic_pointer_cast<MDHistoWorkspace>(ws1);
  MDHistoWorkspace_sptr histo2 =
      boost::dynamic_pointer_cast<MDHistoWorkspace>(ws2);
  IMDEventWorkspace_sptr event1 =
      boost::dynamic_pointer_cast<IMDEventWorkspace>(ws1);
  IMDEventWorkspace_sptr event2 =
      boost::dynamic_pointer_cast<IMDEventWorkspace>(ws2);

  try {
    compare(ws1->id(), ws2->id(), "Workspaces are of different types");

    this->compareMDGeometry(ws1, ws2);

    if (histo1 && histo2) {
      this->compareMDHistoWorkspaces(histo1, histo2);
    } else if (event1 && event2) {
      CALL_MDEVENT_FUNCTION(this->compareMDWorkspaces, event1);
    } else
      m_result = "Workspaces are of different types.";
  } catch (CompareFailsException &e) {
    m_result = e.getMessage();
  }
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
*/
void CompareMDWorkspaces::exec() {
  m_result.clear();
  m_CompareBoxID = !getProperty("IgnoreBoxID");

  this->doComparison();

  if (m_result != "") {
    g_log.notice() << "The workspaces did not match: " << m_result << std::endl;
    this->setProperty("Equals", false);
  } else {
    m_result = "Success!";
    this->setProperty("Equals", true);
  }
  setProperty("Result", m_result);
}

} // namespace Mantid
} // namespace MDAlgorithms
