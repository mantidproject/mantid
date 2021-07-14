// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidMDAlgorithms/CompareMDWorkspaces.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/IMDWorkspace.h"
#include "MantidDataObjects/MDEventFactory.h"
#include "MantidDataObjects/MDHistoWorkspace.h"
#include "MantidGeometry/MDGeometry/IMDDimension.h"
#include "MantidKernel/Strings.h"
#include <sstream>

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;

namespace {
/** Custom exception class to signal a failure
 * in the comparison
 */
class CompareFailsException : public std::runtime_error {
public:
  explicit CompareFailsException(const std::string &msg) : std::runtime_error(msg) {}
  std::string getMessage() const { return this->what(); }
};

/** Return a string "(a vs b)".   */
template <typename T> std::string versus(T a, T b) {
  return "(" + Strings::toString(a) + " vs " + Strings::toString(b) + ")";
}

/** Compare a and b. Return true if they are considered equal
 *
 * @param a First input
 * @param b Second input
 * @param tolerance Considered equal within this range
 */
template <class T> bool compareTol(T a, T b, double tolerance) {
  double diff = fabs(a - b);
  if (diff > tolerance) {
    const double pa = fabs(a);
    const double pb = fabs(b);
    if ((pa > 2 * tolerance) || (pb > 2 * tolerance)) {
      diff = 0.5 * diff / (pa + pb);
      if (diff > tolerance)
        return false;
    } else
      return false;
  }
  return true;
}

/** Compare a and b. Throw if they dont match within Tolerance
 *
 * @param a :: double
 * @param b :: double
 * @param message :: message for result
 * @throw CompareFailsException if the two DONT match
 */
template <class T> void throwIfCompareTol(T a, T b, double tolerance, const std::string &message) {
  if (!compareTol(a, b, tolerance)) {
    throw CompareFailsException(message + " " + versus(a, b));
  }
}

/**
 * Predicate to determine if one event is less than another within a tolerance
 * @param lhs First input MDEvent
 * @param rhs Second input MDEvent
 * @param tolerance Two events are considered equal within this tolerance
 */
template <size_t ND, template <size_t> class MDE>
bool lessThan(const MDE<ND> &lhs, const MDE<ND> &rhs, double tolerance) {
  for (size_t i = 0; i < ND; ++i) {
    if (!compareTol(lhs.getCenter(i), rhs.getCenter(i), tolerance)) {
      if (lhs.getCenter(i) < rhs.getCenter(i))
        return true;
    }
  }

  if (!compareTol(lhs.getSignal(), rhs.getSignal(), tolerance) && lhs.getSignal() < rhs.getSignal()) {
    return true;
  }
  if (!compareTol(lhs.getErrorSquared(), rhs.getErrorSquared(), tolerance) &&
      lhs.getErrorSquared() < rhs.getErrorSquared()) {
    return true;
  }

  return false;
}

/// Output an MDEvent to a stream. Used for debugging.
template <size_t ND, template <size_t> class MDE> std::ostream &operator<<(std::ostream &os, const MDE<ND> &event) {
  os << "(";
  for (size_t i = 0; i < ND; ++i) {
    os << event.getCenter(i) << ", ";
  }
  os << ")"
     << ""
     << "signal = " << event.getSignal() << ", errorSq = " << event.getErrorSquared();

  return os;
}

} // namespace

namespace Mantid {
namespace MDAlgorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(CompareMDWorkspaces)

//----------------------------------------------------------------------------------------------
/// Algorithm's name for identification. @see Algorithm::name
const std::string CompareMDWorkspaces::name() const { return "CompareMDWorkspaces"; }

/// Algorithm's version for identification. @see Algorithm::version
int CompareMDWorkspaces::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string CompareMDWorkspaces::category() const { return "MDAlgorithms\\Utility\\Workspaces"; }

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void CompareMDWorkspaces::init() {
  declareProperty(std::make_unique<WorkspaceProperty<IMDWorkspace>>("Workspace1", "", Direction::Input),
                  "First MDWorkspace to compare.");
  declareProperty(std::make_unique<WorkspaceProperty<IMDWorkspace>>("Workspace2", "", Direction::Input),
                  "Second MDWorkspace to compare.");

  declareProperty("Tolerance", 0.0, "The maximum amount by which values may differ between the workspaces.");
  declareProperty("MDEventTolerance", EMPTY_DBL(),
                  "The maximum amount by which values may differ between 2 MDEvents to compare. Defaults to tolerance");
  declareProperty("CheckEvents", true,
                  "Whether to compare each MDEvent. If "
                  "False, will only look at the box "
                  "structure.");

  declareProperty(std::make_unique<PropertyWithValue<bool>>("Equals", false, Direction::Output),
                  "Boolean set to true if the workspaces match.");
  declareProperty(std::make_unique<PropertyWithValue<std::string>>("Result", "", Direction::Output),
                  "String describing the difference found between the workspaces");
  declareProperty("IgnoreBoxID", false,
                  "To ignore box ID-s when comparing MD "
                  "boxes as Multithreaded splitting "
                  "assigns box id-s randomly");
}

//----------------------------------------------------------------------------------------------
/** Compare a and b. Throw if they dont match
 *
 * @param a :: any type
 * @param b :: any type
 * @param message :: message for result
 * @throw CompareFailsException if the two DONT match
 */
template <typename T> void CompareMDWorkspaces::compare(T a, T b, const std::string &message) {
  if (a != b)
    throw CompareFailsException(message + " " + versus(a, b));
}

//----------------------------------------------------------------------------------------------
/** Compare the dimensions etc. of two MDWorkspaces
 */
void CompareMDWorkspaces::compareMDGeometry(const Mantid::API::IMDWorkspace_sptr &ws1,
                                            const Mantid::API::IMDWorkspace_sptr &ws2) {
  compare(ws1->getNumDims(), ws2->getNumDims(), "Workspaces have a different number of dimensions");
  for (size_t d = 0; d < ws1->getNumDims(); d++) {
    IMDDimension_const_sptr dim1 = ws1->getDimension(d);
    IMDDimension_const_sptr dim2 = ws2->getDimension(d);
    compare(dim1->getName(), dim2->getName(), "Dimension #" + Strings::toString(d) + " has a different name");
    compare(dim1->getUnits(), dim2->getUnits(), "Dimension #" + Strings::toString(d) + " has different units");
    compare(dim1->getNBins(), dim2->getNBins(),
            "Dimension #" + Strings::toString(d) + " has a different number of bins");
    throwIfCompareTol(dim1->getMinimum(), dim2->getMinimum(), m_tolerance,
                      "Dimension #" + Strings::toString(d) + " has a different minimum");
    throwIfCompareTol(dim1->getMaximum(), dim2->getMaximum(), m_tolerance,
                      "Dimension #" + Strings::toString(d) + " has a different maximum");
  }
}

//----------------------------------------------------------------------------------------------
/** Compare the dimensions etc. of two MDWorkspaces
 */
void CompareMDWorkspaces::compareMDHistoWorkspaces(const Mantid::DataObjects::MDHistoWorkspace_sptr &ws1,
                                                   const Mantid::DataObjects::MDHistoWorkspace_sptr &ws2) {
  compare(ws1->getNumDims(), ws2->getNumDims(), "Workspaces have a different number of dimensions");
  compare(ws1->getNPoints(), ws2->getNPoints(), "Workspaces have a different number of points");
  for (size_t i = 0; i < ws1->getNPoints(); i++) {
    double diff = fabs(ws1->getSignalAt(i) - ws2->getSignalAt(i));
    if (diff > m_tolerance)
      throw CompareFailsException("MDHistoWorkspaces have a different signal at index " + Strings::toString(i) + " " +
                                  versus(ws1->getSignalAt(i), ws2->getSignalAt(i)));

    double diffErr = fabs(ws1->getErrorAt(i) - ws2->getErrorAt(i));
    if (diffErr > m_tolerance)
      throw CompareFailsException("MDHistoWorkspaces have a different error at index " + Strings::toString(i) + " " +
                                  versus(ws1->getErrorAt(i), ws2->getErrorAt(i)));
  }
}

//----------------------------------------------------------------------------------------------
/** Perform the comparison on MDEventWorkspaces
 *
 * @param ws ::  MDEventWorkspace to compare
 */
template <typename MDE, size_t nd>
void CompareMDWorkspaces::compareMDEventWorkspaces(typename MDEventWorkspace<MDE, nd>::sptr ws1) {
  typename MDEventWorkspace<MDE, nd>::sptr ws2 = std::dynamic_pointer_cast<MDEventWorkspace<MDE, nd>>(inWS2);
  if (!ws1 || !ws2)
    throw std::runtime_error("Incompatible workspace types passed to PlusMD.");

  std::vector<API::IMDNode *> boxes1;
  std::vector<API::IMDNode *> boxes2;

  ws1->getBox()->getBoxes(boxes1, 1000, false);
  ws2->getBox()->getBoxes(boxes2, 1000, false);
  std::stringstream ess;
  ess << "Workspace1 has " << boxes1.size() << " boxes; Workspace2 has " << boxes2.size() << " boxes";
  std::string boxinfo(ess.str());

  this->compare(boxes1.size(), boxes2.size(), "Workspaces do not have the same number of boxes. " + boxinfo);
  g_log.information(boxinfo);

  bool boxes_same(true);
  std::string errormessage("");
  const int num_boxes = static_cast<int>(boxes1.size());
  // workspace with file backed cannot work with OpenMP
  // segmentation fault is generated on Mac build
  const bool filebacked = ws1->isFileBacked() || ws2->isFileBacked();

  // cppcheck-suppress syntaxError
  PRAGMA_OMP( parallel for if (!filebacked))
  for (int ibox = 0; ibox < num_boxes; ibox++) {
    PARALLEL_START_INTERUPT_REGION
    // No need to compare because the boxes are not same already
    if (!boxes_same) {
      continue;
    }

    bool local_fail(false);
    std::string local_error("");

    API::IMDNode *box1 = boxes1[ibox];
    API::IMDNode *box2 = boxes2[ibox];
    g_log.debug() << "Box " << ibox << "ws1 npoints = " << box1->getNPoints()
                  << "; ws2 npoints = " << box2->getNPoints() << "\n";

    try {
      CompareMDWorkspaces::compare2Boxes<MDE, nd>(box1, box2, static_cast<size_t>(ibox));
    } catch (CompareFailsException &err) {
      local_fail = true;
      local_error += err.what();
    }

    PARALLEL_CRITICAL(FindPeaks_WriteOutput) {
      if (local_fail) {
        boxes_same = false;
        errormessage += local_error;
      }
    }

    PARALLEL_END_INTERUPT_REGION
  } // for box
  PARALLEL_CHECK_INTERUPT_REGION

  // throw altogether
  if (!boxes_same) {
    throw CompareFailsException(errormessage);
  }
}

template <typename MDE, size_t nd>
void CompareMDWorkspaces::compare2Boxes(API::IMDNode *box1, API::IMDNode *box2, size_t ibox) {

  if (m_CompareBoxID)
    this->compare(box1->getID(), box2->getID(), "Boxes have different ID");
  else {
    if (box1->getID() != box2->getID())
      g_log.debug() << " Boxes N: " << ibox << " have box ID: " << box1->getID() << " and " << box2->getID()
                    << " correspondingly\n";
  }
  this->compare(size_t(box1->getDepth()), size_t(box2->getDepth()), "Boxes are at a different depth");
  this->compare(box1->getNumChildren(), box2->getNumChildren(), "Boxes do not have the same number of children");

  for (size_t i = 0; i < box1->getNumChildren(); i++) {
    if (m_CompareBoxID)
      this->compare(box1->getChild(i)->getID(), box2->getChild(i)->getID(), "Child of boxes do not match IDs");
    else {
      if (box1->getID() != box2->getID())
        g_log.debug() << " Boxes N: " << ibox << " children N: " << i << " have box ID: " << box1->getChild(i)->getID()
                      << " and " << box2->getChild(i)->getID() << " correspondingly\n";
    }
  }

  for (size_t d = 0; d < nd; d++) {
    throwIfCompareTol(box1->getExtents(d).getMin(), box2->getExtents(d).getMin(), m_tolerance,
                      "Extents of box do not match");
    throwIfCompareTol(box1->getExtents(d).getMax(), box2->getExtents(d).getMax(), m_tolerance,
                      "Extents of box do not match");
  }
  throwIfCompareTol(box1->getInverseVolume(), box2->getInverseVolume(), m_tolerance,
                    "Box inverse volume does not match");
  throwIfCompareTol(box1->getSignal(), box2->getSignal(), m_tolerance, "Box signal does not match");
  throwIfCompareTol(box1->getErrorSquared(), box2->getErrorSquared(), m_tolerance, "Box error squared does not match");
  if (m_CheckEvents)
    this->compare(box1->getNPoints(), box2->getNPoints(), "Number of points in box does not match");

  // Are both MDGridBoxes ?
  auto *gridbox1 = dynamic_cast<MDGridBox<MDE, nd> *>(box1);
  auto *gridbox2 = dynamic_cast<MDGridBox<MDE, nd> *>(box2);
  if (gridbox1 && gridbox2) {
    // MDGridBox: compare box size on each dimension
    for (size_t d = 0; d < nd; d++)
      throwIfCompareTol(gridbox1->getBoxSize(d), gridbox2->getBoxSize(d), m_tolerance, "Box sizes do not match");
  } else {
    // Could be both MDBoxes (with events)
    auto *mdbox1 = dynamic_cast<MDBox<MDE, nd> *>(box1);
    auto *mdbox2 = dynamic_cast<MDBox<MDE, nd> *>(box2);
    g_log.debug() << "Box " << ibox << "ws1 npoints = " << box1->getNPoints()
                  << "; ws2 npoints = " << box2->getNPoints() << "\n";

    // Rule out the case if one and only one box is MDBox
    if (mdbox1 && !mdbox2) {
      // workspace 2's is MDBox but workspace 1 is not
      throw CompareFailsException("Worksapce 2's Box " + std::to_string(ibox) + " is not MDBox");
    } else if (!mdbox1 && mdbox2) {
      // workspace 2's is MDBox but workspace 1 is not
      throw CompareFailsException("Worksapce 1's Box " + std::to_string(ibox) + " is not MDBox");
    }

    // Both boxes are MDBoxes:
    if (m_CheckEvents) {
      const std::vector<MDE> &events1 = mdbox1->getConstEvents();
      const std::vector<MDE> &events2 = mdbox2->getConstEvents();

      try {
        this->compare(events1.size(), events2.size(), "Box event vectors are not the same length");

        if (events1.size() == events2.size()) {
          // Sorting requires assignment. Use index into original array and sort that
          std::vector<size_t> events1Indexes(events1.size()), events2Indexes(events2.size());
          std::iota(events1Indexes.begin(), events1Indexes.end(), 0);
          std::iota(events2Indexes.begin(), events2Indexes.end(), 0);

          std::stable_sort(events1Indexes.begin(), events1Indexes.end(),
                           [this, &events1](const size_t lhsIndex, const size_t rhsIndex) {
                             return lessThan(events1[lhsIndex], events1[rhsIndex], m_mdeventTolerance);
                           });
          std::stable_sort(events2Indexes.begin(), events2Indexes.end(),
                           [this, &events2](const size_t lhsIndex, const size_t rhsIndex) {
                             return lessThan(events2[lhsIndex], events2[rhsIndex], m_mdeventTolerance);
                           });
          bool same{true};
          size_t numdiff{0};
          for (size_t i = 0; i < events1Indexes.size(); ++i) {
            const auto &lhs(events1[events1Indexes[i]]), rhs(events2[events2Indexes[i]]);
            try {
              // coordinate
              for (size_t d = 0; d < nd; ++d) {
                throwIfCompareTol(lhs.getCenter(d), rhs.getCenter(d), m_mdeventTolerance,
                                  "dim " + std::to_string(d) + " ");
              }
              // signal
              throwIfCompareTol(lhs.getSignal(), rhs.getSignal(), m_mdeventTolerance, "");
              // error
              throwIfCompareTol(lhs.getErrorSquared(), rhs.getErrorSquared(), m_mdeventTolerance, "");
            } catch (CompareFailsException &e) {
              g_log.debug() << "Box " << ibox << " Event " << i << ": " << e.what() << "\n    [ws1] : " << lhs
                            << "\n    [ws2] : " << rhs << "\n";
              numdiff++;
              same = false;
            }
          }

          if (!same) {
            const std::string diffmessage("Box " + std::to_string(ibox) + " contains " + std::to_string(numdiff) +
                                          " different events\n");
            throw CompareFailsException("MDEvents are not the same: " + diffmessage);
          }
        }
      } catch (CompareFailsException &) {
        // Boxes must release events if the check fails
        mdbox1->releaseEvents();
        mdbox2->releaseEvents();
        // Rethrow with the same type of error message!
        throw;
      }
      mdbox1->releaseEvents();
      mdbox2->releaseEvents();
    } // if check events
  }   // if-else for MDGridBox or MDBox
}

//----------------------------------------------------------------------------------------------
/** Perform comparison, set m_result if not matching.
 */
void CompareMDWorkspaces::doComparison() {
  m_tolerance = getProperty("Tolerance");
  m_mdeventTolerance = getProperty("MDEventTolerance");
  if (isEmpty(m_mdeventTolerance))
    m_mdeventTolerance = m_tolerance;
  m_CheckEvents = getProperty("CheckEvents");

  IMDWorkspace_sptr ws1 = getProperty("Workspace1");
  IMDWorkspace_sptr ws2 = getProperty("Workspace2");
  inWS2 = ws2;
  if (!ws1 || !ws2)
    throw std::invalid_argument("Invalid workspace given.");

  MatrixWorkspace_sptr mws1 = std::dynamic_pointer_cast<MatrixWorkspace>(ws1);
  MatrixWorkspace_sptr mws2 = std::dynamic_pointer_cast<MatrixWorkspace>(ws2);
  if (mws1 || mws2)
    throw std::invalid_argument("Cannot compare MatrixWorkspaces. Please use "
                                "CompareWorkspaces algorithm instead.");

  MDHistoWorkspace_sptr histo1 = std::dynamic_pointer_cast<MDHistoWorkspace>(ws1);
  MDHistoWorkspace_sptr histo2 = std::dynamic_pointer_cast<MDHistoWorkspace>(ws2);
  IMDEventWorkspace_sptr event1 = std::dynamic_pointer_cast<IMDEventWorkspace>(ws1);
  IMDEventWorkspace_sptr event2 = std::dynamic_pointer_cast<IMDEventWorkspace>(ws2);

  try {
    compare(ws1->id(), ws2->id(), "Workspaces are of different types");

    this->compareMDGeometry(ws1, ws2);

    if (histo1 && histo2) {
      this->compareMDHistoWorkspaces(histo1, histo2);
    } else if (event1 && event2) {
      CALL_MDEVENT_FUNCTION(this->compareMDEventWorkspaces, event1);
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

  if (!m_result.empty()) {
    g_log.notice() << "The workspaces did not match: " << m_result << '\n';
    this->setProperty("Equals", false);
  } else {
    m_result = "Success!";
    g_log.notice("The workspaces did match");
    this->setProperty("Equals", true);
  }
  setProperty("Result", m_result);
}

} // namespace MDAlgorithms
} // namespace Mantid
