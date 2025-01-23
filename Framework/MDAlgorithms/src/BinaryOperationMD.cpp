// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidMDAlgorithms/BinaryOperationMD.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/IMDWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidDataObjects/MDEventFactory.h"
#include "MantidDataObjects/MDHistoWorkspace.h"
#include "MantidDataObjects/WorkspaceSingleValue.h"
#include "MantidGeometry/MDGeometry/IMDDimension.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::DataObjects;
using Mantid::Geometry::IMDDimension_const_sptr;

namespace Mantid::MDAlgorithms {

/// Algorithm's name for identification. @see Algorithm::name
const std::string BinaryOperationMD::name() const { return "BinaryOperationMD"; }

/// Algorithm's version for identification. @see Algorithm::version
int BinaryOperationMD::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string BinaryOperationMD::category() const { return "MDAlgorithms\\MDArithmetic"; }

/** Initialize the algorithm's properties.
 */
void BinaryOperationMD::init() {
  declareProperty(std::make_unique<WorkspaceProperty<IMDWorkspace>>(inputPropName1(), "", Direction::Input),
                  "An MDEventWorkspace, MDHistoWorkspace or "
                  "WorkspaceSingleValue as the left-hand side of the "
                  "operation.");
  declareProperty(std::make_unique<WorkspaceProperty<IMDWorkspace>>(inputPropName2(), "", Direction::Input),
                  "An MDEventWorkspace, MDHistoWorkspace or "
                  "WorkspaceSingleValue as the right-hand side of the "
                  "operation.");
  declareProperty(std::make_unique<WorkspaceProperty<IMDWorkspace>>(outputPropName(), "", Direction::Output),
                  "Name of the output MDEventWorkspace or MDHistoWorkspace.");
  this->initExtraProperties();
}

/// Optional extra properties
void BinaryOperationMD::initExtraProperties() {}

/** Execute the algorithm.
 */
void BinaryOperationMD::exec() {
  // Get the properties
  m_lhs = getProperty(inputPropName1());
  m_rhs = getProperty(inputPropName2());
  m_out = getProperty(outputPropName());

  // Flip LHS and RHS if commutative and :
  //  1. A = B + A -> becomes -> A += B
  //  1. C = 1 + A -> becomes -> C = A + 1   (number is always on RHS if
  //  possible)
  if (this->commutative() && ((m_out == m_rhs) || std::dynamic_pointer_cast<WorkspaceSingleValue>(m_lhs))) {
    // So we flip RHS/LHS
    Mantid::API::IMDWorkspace_sptr temp = m_lhs;
    m_lhs = m_rhs;
    m_rhs = temp;
  }

  // Do not compare conventions if one is single value
  if (!std::dynamic_pointer_cast<WorkspaceSingleValue>(m_rhs)) {
    if (m_lhs->getConvention() != m_rhs->getConvention()) {
      throw std::runtime_error("Workspaces have different conventions for Q. "
                               "Use algorithm ChangeQConvention on one workspace. ");
    }
  }

  // Can't do A = 1 / B
  if (std::dynamic_pointer_cast<MatrixWorkspace>(m_lhs))
    throw std::invalid_argument("BinaryOperationMD: can't have a "
                                "MatrixWorkspace (e.g. WorkspaceSingleValue) "
                                "as the LHS argument of " +
                                this->name() + ".");

  // Check the inputs. First cast to everything
  m_lhs_event = std::dynamic_pointer_cast<IMDEventWorkspace>(m_lhs);
  m_lhs_histo = std::dynamic_pointer_cast<MDHistoWorkspace>(m_lhs);
  m_lhs_scalar = std::dynamic_pointer_cast<WorkspaceSingleValue>(m_lhs);
  m_rhs_event = std::dynamic_pointer_cast<IMDEventWorkspace>(m_rhs);
  m_rhs_histo = std::dynamic_pointer_cast<MDHistoWorkspace>(m_rhs);
  m_rhs_scalar = std::dynamic_pointer_cast<WorkspaceSingleValue>(m_rhs);

  // MDEventWorkspaces only:
  // If you have to clone any WS, and the operation is commutative, and is NOT
  // in-place, then clone the one that is file-backed.
  if (this->commutative() && (m_lhs_event && m_rhs_event) && (m_out != m_lhs)) {
    if (m_rhs_event->isFileBacked() && !m_lhs_event->isFileBacked()) {
      // So we flip RHS/LHS
      Mantid::API::IMDWorkspace_sptr temp = m_lhs;
      m_lhs = m_rhs;
      m_rhs = temp;
      m_lhs_event = std::dynamic_pointer_cast<IMDEventWorkspace>(m_lhs);
      m_rhs_event = std::dynamic_pointer_cast<IMDEventWorkspace>(m_rhs);
    }
  }

  this->checkInputs();

  if (m_out == m_lhs) {
    // A = A * B. -> we will do A *= B
  } else {
    // C = A + B. -> So first we clone A (lhs) into C
    auto clone = createChildAlgorithm("CloneMDWorkspace", 0.0, 0.5, true);
    clone->setProperty("InputWorkspace", m_lhs);
    clone->executeAsChildAlg();
    m_out = clone->getProperty("OutputWorkspace");
  }

  // Okay, at this point we are ready to do, e.g.,
  //  "m_out /= m_rhs"
  if (!m_out)
    throw std::runtime_error("Error creating the output workspace");
  if (!m_rhs)
    throw std::runtime_error("No RHS workspace specified!");

  m_operand_event = std::dynamic_pointer_cast<IMDEventWorkspace>(m_rhs);
  m_operand_histo = std::dynamic_pointer_cast<MDHistoWorkspace>(m_rhs);
  m_operand_scalar = std::dynamic_pointer_cast<WorkspaceSingleValue>(m_rhs);

  m_out_event = std::dynamic_pointer_cast<IMDEventWorkspace>(m_out);
  m_out_histo = std::dynamic_pointer_cast<MDHistoWorkspace>(m_out);

  if (m_out_event) {
    // Call the templated virtual function for this type of MDEventWorkspace
    this->execEvent();
  } else if (m_out_histo) {
    // MDHistoWorkspace as the output
    if (m_operand_histo) {
      if (m_out_histo->getNumDims() != m_operand_histo->getNumDims())
        throw std::invalid_argument("Cannot perform " + this->name() +
                                    " on MDHistoWorkspace's with a different number of dimensions.");
      if (m_out_histo->getNPoints() != m_operand_histo->getNPoints())
        throw std::invalid_argument("Cannot perform " + this->name() +
                                    " on MDHistoWorkspace's with a different number of points.");

      // Check that the dimensions span the same size, warn if they don't
      for (size_t d = 0; d < m_out_histo->getNumDims(); d++) {
        IMDDimension_const_sptr dimA = m_out_histo->getDimension(0);
        IMDDimension_const_sptr dimB = m_operand_histo->getDimension(0);
        if (dimA->getMinimum() != dimB->getMinimum() || dimA->getMaximum() != dimB->getMaximum())
          g_log.warning() << "Dimension " << d << " (" << dimA->getName()
                          << ") has different extents in the two "
                             "MDHistoWorkspaces. The operation may not make "
                             "sense!\n";
      }
      this->execHistoHisto(m_out_histo, m_operand_histo);
    } else if (m_operand_scalar)
      this->execHistoScalar(m_out_histo, m_operand_scalar);
    else
      throw std::runtime_error("Unexpected operand workspace type. Expected MDHistoWorkspace or "
                               "WorkspaceSingleValue, got " +
                               m_rhs->id());

    // Clear any masking flags from the output workspace
    if (m_out) {
      m_out->clearMDMasking();
    }

    // When operating on MDHistoWorkspaces, add a simple flag
    // that will be checked in BinMD to avoid binning a modified workspace
    if (m_out_histo->getNumExperimentInfo() == 0) // Create a run if needed
      m_out_histo->addExperimentInfo(ExperimentInfo_sptr(new ExperimentInfo()));
    m_out_histo->getExperimentInfo(0)->mutableRun().addProperty(
        new PropertyWithValue<std::string>("mdhisto_was_modified", "1"), true);
  } else {
    throw std::runtime_error("Unexpected output workspace type. Expected MDEventWorkspace or "
                             "MDHistoWorkspace, got " +
                             m_out->id());
  }

  // Give the output
  setProperty("OutputWorkspace", m_out);
}

} // namespace Mantid::MDAlgorithms
