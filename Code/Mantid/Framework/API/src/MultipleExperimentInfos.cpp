#include "MantidAPI/ExperimentInfo.h"
#include "MantidAPI/MultipleExperimentInfos.h"
#include "MantidKernel/System.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid {
namespace API {

//----------------------------------------------------------------------------------------------
/** Constructor
 */
MultipleExperimentInfos::MultipleExperimentInfos() {}

//----------------------------------------------------------------------------------------------
/** Copy constructor
 *
 * @param other :: other workspace to copy    */
MultipleExperimentInfos::MultipleExperimentInfos(
    const MultipleExperimentInfos &other) {
  this->copyExperimentInfos(other);
}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
MultipleExperimentInfos::~MultipleExperimentInfos() {}

//-----------------------------------------------------------------------------------------------
/** Get the ExperimentInfo for the given run Index
 *
 * @param runIndex :: 0-based index of the run to get.
 * @return shared ptr to the ExperimentInfo class
 */
ExperimentInfo_sptr
MultipleExperimentInfos::getExperimentInfo(const uint16_t runIndex) {
  if (size_t(runIndex) >= m_expInfos.size())
    throw std::invalid_argument(
        "MDWorkspace::getExperimentInfo(): runIndex is out of range.");
  return m_expInfos[runIndex];
}

//-----------------------------------------------------------------------------------------------
/** Get the ExperimentInfo for the given run Index
 *
 * @param runIndex :: 0-based index of the run to get.
 * @return shared ptr to the ExperimentInfo class
 */
ExperimentInfo_const_sptr
MultipleExperimentInfos::getExperimentInfo(const uint16_t runIndex) const {
  if (size_t(runIndex) >= m_expInfos.size())
    throw std::invalid_argument(
        "MDWorkspace::getExperimentInfo() const: runIndex is out of range.");
  return m_expInfos[runIndex];
}

//-----------------------------------------------------------------------------------------------
/** Add a new ExperimentInfo to this MDEventWorkspace
 *
 * @param ei :: shared ptr to the ExperimentInfo class to add
 * @return the runIndex at which it was added
 * @throw std::runtime_error if you reach the limit of 65536 entries.
 */
uint16_t MultipleExperimentInfos::addExperimentInfo(ExperimentInfo_sptr ei) {
  m_expInfos.push_back(ei);
  if (m_expInfos.size() >=
      static_cast<size_t>(std::numeric_limits<uint16_t>::max()))
    throw std::runtime_error("MDWorkspace: Reached the capacity for the number "
                             "of ExperimentInfos of 65536.");
  return uint16_t(m_expInfos.size() - 1);
}

//-----------------------------------------------------------------------------------------------
/** Replace the ExperimentInfo entry at a given place
 *
 * @param runIndex :: 0-based index of the run to replace
 * @param ei :: shared ptr to the ExperimentInfo class to add
 */
void MultipleExperimentInfos::setExperimentInfo(const uint16_t runIndex,
                                                ExperimentInfo_sptr ei) {
  if (size_t(runIndex) >= m_expInfos.size())
    throw std::invalid_argument(
        "MDEventWorkspace::setExperimentInfo(): runIndex is out of range.");
  m_expInfos[runIndex] = ei;
}

//-----------------------------------------------------------------------------------------------
/// @return the number of ExperimentInfo's in this workspace
uint16_t MultipleExperimentInfos::getNumExperimentInfo() const {
  return uint16_t(m_expInfos.size());
}

//-----------------------------------------------------------------------------------------------
/** Copy the experiment infos from another. Deep copy.
 * @param other :: other workspace to copy    */
void MultipleExperimentInfos::copyExperimentInfos(
    const MultipleExperimentInfos &other) {
  m_expInfos.clear();
  // Do a deep copy of ExperimentInfo's
  for (size_t i = 0; i < other.m_expInfos.size(); i++) {
    ExperimentInfo_sptr copy(new ExperimentInfo(*other.m_expInfos[i]));
    m_expInfos.push_back(copy);
  }
}

const std::string MultipleExperimentInfos::toString() const {
  //    if (m_expInfos.size() == 1)
  //      return m_expInfos[0]->toString();

  // mess with things in multiple case
  std::ostringstream os;
  for (std::size_t i = 0; i < m_expInfos.size(); ++i) {
    os << m_expInfos[i]->toString();
    if (i + 1 != m_expInfos.size())
      os << "\n";
  }

  return os.str();
}

} // namespace Mantid
} // namespace API
