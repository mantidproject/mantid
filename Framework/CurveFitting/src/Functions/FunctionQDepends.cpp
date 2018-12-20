// Mantid Coding standars <http://www.mantidproject.org/Coding_Standards>

// Main Module Header
#include "MantidCurveFitting/Functions/FunctionQDepends.h"
// Mantid Headers from the same project
// N/A
// Mantid headers from other projects
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/NumericAxis.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidGeometry/IDetector.h"
#include "MantidKernel/UnitConversion.h"
// third party libraries
// N/A
// standard library
// N/A

using Attr = Mantid::API::IFunction::Attribute;

namespace {
Mantid::Kernel::Logger g_log("FunctionQDepends");
}

namespace Mantid {
namespace CurveFitting {
namespace Functions {

/* ===========
   Public
   ===========*/

/**
 * @brief declare commonattributes Q and WorkspaceIndex.
 * Subclasses containing additional attributes should override this method by
 * declaring the additional
 * attributes and then calling the parent (this) method to declare Q and
 * WorkspaceIndex.
 */
void FunctionQDepends::declareAttributes() {
  this->declareAttribute("Q", Attr(EMPTY_DBL()));
  this->declareAttribute("WorkspaceIndex", Attr(EMPTY_INT()));
}

/**
 * @brief Update attributes WorkspaceIndex and Q according to certain precedence
 *rules.
 * Subclasses featuring additional attributes should override and insert a call
 *to
 * this method within the overriding setAttribute function.
 * There are two ways to update Q: (i) loading the value from the spectrum, and
 *(ii) manual
 * input from the user. Therefore, rules of precedence must be set to prevent
 *conflict. The
 * priority is to accept Q from the spectrum, if a Q value can be derived from
 *such. In
 * this case the existing Q value will be overwritten, irrespective of the
 *mannier in which
 * the old Q value was set.
 *
 * @param attName name of the attribute
 * @param attValue  value of the attribute
 */
void FunctionQDepends::setAttribute(const std::string &attName,
                                    const Attr &attValue) {
  // Q value is tied to WorkspaceIndex if we have a list of Q values
  if (attName == "WorkspaceIndex") {
    size_t wi{static_cast<size_t>(
        attValue.asInt())}; // ah!, the "joys" of C++ strong typing.
    if (!m_vQ.empty() && wi < m_vQ.size()) {
      Mantid::API::IFunction::setAttribute(attName, attValue);
      Mantid::API::IFunction::setAttribute("Q", Attribute(m_vQ.at(wi)));
    }
  }
  // Q can be manually changed by user only if list of Q values is empty
  else if (attName == "Q") {
    if (m_vQ.empty()) {
      Mantid::API::IFunction::setAttribute(attName, attValue);
    }
  } else {
    Mantid::API::IFunction::setAttribute(attName, attValue);
  }
}

/**
 * @brief Learn the Q values from the workspace, if possible, and update
 * attribute Q accordingly.
 * @param workspace Matrix workspace
 * @param wi selected spectrum to initialize attributes
 * @param startX unused
 * @param endX unused
 */
void FunctionQDepends::setMatrixWorkspace(
    boost::shared_ptr<const Mantid::API::MatrixWorkspace> workspace, size_t wi,
    double startX, double endX) {
  UNUSED_ARG(startX);
  UNUSED_ARG(endX);
  // reset attributes if new workspace is passed
  if (!m_vQ.empty()) {
    Mantid::API::IFunction::setAttribute("WorkspaceIndex", Attr(EMPTY_INT()));
    Mantid::API::IFunction::setAttribute("Q", Attr(EMPTY_DBL()));
  }
  // Obtain Q values from the passed workspace, if possible. m_vQ will be
  // cleared if unsuccessful.
  if (workspace) {
    m_vQ = this->extractQValues(*workspace);
  }
  if (!m_vQ.empty()) {
    this->setAttribute("WorkspaceIndex", Attr(static_cast<int>(wi)));
  }
}

/* ===========
   Private
   ===========*/

/**
 * @brief Extract Q values from vertical dimension of the workspace, or compute
 * them.
 * @param workspace workspace possibly containing Q values.
 */
std::vector<double> FunctionQDepends::extractQValues(
    const Mantid::API::MatrixWorkspace &workspace) {
  std::vector<double> qs;
  // Check if the vertical axis has units of momentum transfer, then extract Q
  // values...
  auto axis_ptr =
      dynamic_cast<Mantid::API::NumericAxis *>(workspace.getAxis(1));
  if (axis_ptr) {
    const boost::shared_ptr<Kernel::Unit> &unit_ptr = axis_ptr->unit();
    if (unit_ptr->unitID() == "MomentumTransfer") {
      qs = axis_ptr->getValues();
    }
  }
  // ...otherwise, compute the momentum transfer for each spectrum, if possible
  else {
    const auto &spectrumInfo = workspace.spectrumInfo();
    size_t numHist = workspace.getNumberHistograms();
    for (size_t wi = 0; wi < numHist; wi++) {
      if (spectrumInfo.hasDetectors(wi)) {
        const auto detID = spectrumInfo.detector(wi).getID();
        double efixed = workspace.getEFixed(detID);
        double usignTheta = 0.5 * spectrumInfo.twoTheta(wi);
        double q = Mantid::Kernel::UnitConversion::convertToElasticQ(usignTheta,
                                                                     efixed);
        qs.push_back(q);
      } else {
        g_log.debug("Cannot populate Q values from workspace");
        qs.clear();
        break;
      }
    }
  }
  return qs;
}

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid
