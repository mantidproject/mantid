#include "MantidMDAlgorithms/UnaryOperationMD.h"
#include "MantidKernel/System.h"
#include "MantidAPI/IMDWorkspace.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidMDEvents/MDHistoWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::MDEvents;

namespace Mantid {
namespace MDAlgorithms {

//----------------------------------------------------------------------------------------------
/** Constructor
 */
UnaryOperationMD::UnaryOperationMD() {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
UnaryOperationMD::~UnaryOperationMD() {}

//----------------------------------------------------------------------------------------------
/// Algorithm's name for identification. @see Algorithm::name
const std::string UnaryOperationMD::name() const { return "UnaryOperationMD"; }

/// Algorithm's version for identification. @see Algorithm::version
int UnaryOperationMD::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string UnaryOperationMD::category() const {
  return "MDAlgorithms\\MDArithmetic";
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void UnaryOperationMD::init() {
  declareProperty(new WorkspaceProperty<IMDWorkspace>(inputPropName(), "",
                                                      Direction::Input),
                  "A MDEventWorkspace or MDHistoWorkspace on which to apply "
                  "the operation.");
  declareProperty(new WorkspaceProperty<IMDWorkspace>(outputPropName(), "",
                                                      Direction::Output),
                  "Name of the output MDEventWorkspace or MDHistoWorkspace.");
  this->initExtraProperties();
}

//----------------------------------------------------------------------------------------------
/// Optional method to be subclassed to add properties
void UnaryOperationMD::initExtraProperties() {}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void UnaryOperationMD::exec() {
  // Get the properties
  m_in = getProperty(inputPropName());
  m_out = getProperty(outputPropName());

  // For MatrixWorkspace's ...
  if (boost::dynamic_pointer_cast<MatrixWorkspace>(m_in)) {
    // Pass-through to the same function without "MD"
    std::string matrixAlg = this->name();
    matrixAlg = matrixAlg.substr(0, matrixAlg.size() - 2);
    IAlgorithm_sptr alg = this->createChildAlgorithm(matrixAlg);
    // Copy all properties from THIS to the non-MD version
    std::vector<Property *> props = this->getProperties();
    for (size_t i = 0; i < props.size(); i++) {
      Property *prop = props[i];
      alg->setPropertyValue(prop->name(), prop->value());
    }
    alg->execute();
    // Copy the output too
    MatrixWorkspace_sptr outMW = alg->getProperty("OutputWorkspace");
    IMDWorkspace_sptr out = boost::dynamic_pointer_cast<IMDWorkspace>(outMW);
    setProperty("OutputWorkspace", out);
    return;
  }

  // Check for validity
  m_in_event = boost::dynamic_pointer_cast<IMDEventWorkspace>(m_in);
  m_in_histo = boost::dynamic_pointer_cast<MDHistoWorkspace>(m_in);
  this->checkInputs();

  if (m_out != m_in) {
    // B = f(A) -> So first we clone A (lhs) into B
    IAlgorithm_sptr clone =
        this->createChildAlgorithm("CloneMDWorkspace", 0.0, 0.5, true);
    clone->setProperty("InputWorkspace", m_in);
    clone->executeAsChildAlg();
    m_out = clone->getProperty("OutputWorkspace");
  }

  // Okay, at this point we are ready to do, e.g.,
  //  "log(m_out)"
  if (!m_out)
    throw std::runtime_error("Error creating the output workspace");

  IMDEventWorkspace_sptr m_out_event =
      boost::dynamic_pointer_cast<IMDEventWorkspace>(m_out);
  MDHistoWorkspace_sptr m_out_histo =
      boost::dynamic_pointer_cast<MDHistoWorkspace>(m_out);

  // Call the appropriate sub-function
  if (m_out_event)
    this->execEvent(m_out_event);
  else if (m_out_histo)
    this->execHisto(m_out_histo);
  else {
    throw std::runtime_error(
        "Unexpected output workspace type. Expected MDEventWorkspace or "
        "MDHistoWorkspace, got " +
        m_out->id());
  }

  // Give the output
  setProperty("OutputWorkspace", m_out);
}

} // namespace Mantid
} // namespace MDAlgorithms
