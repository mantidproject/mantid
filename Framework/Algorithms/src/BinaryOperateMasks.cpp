// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/BinaryOperateMasks.h"
#include "MantidDataObjects/MaskWorkspace.h"
#include "MantidKernel/ListValidator.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid {
namespace Algorithms {

DECLARE_ALGORITHM(BinaryOperateMasks)

// enum BinaryOperator{AND, OR, XOR, NOT};

//----------------------------------------------------------------------------------------------
/** Constructor
 */
BinaryOperateMasks::BinaryOperateMasks() {
  // TODO Auto-generated constructor stub
}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
BinaryOperateMasks::~BinaryOperateMasks() {
  // TODO Auto-generated destructor stub
}

void BinaryOperateMasks::init() {

  std::vector<std::string> operators{"AND", "OR", "XOR", "NOT"};

  declareProperty(
      std::make_unique<WorkspaceProperty<DataObjects::MaskWorkspace>>(
          "InputWorkspace1", "", Direction::Input),
      "MaskWorkspace 1 for binary operation");
  declareProperty(
      std::make_unique<WorkspaceProperty<DataObjects::MaskWorkspace>>(
          "InputWorkspace2", "", Direction::Input, PropertyMode::Optional),
      "Optional MaskWorkspace 2 for binary operation");
  declareProperty("OperationType", "AND",
                  boost::make_shared<StringListValidator>(operators),
                  "Operator for Workspace1 and Workspace2");
  declareProperty(
      std::make_unique<WorkspaceProperty<DataObjects::MaskWorkspace>>(
          "OutputWorkspace", "", Direction::Output),
      "Output MaskWorkspace as result of binary operation");
}

//----------------------------------------------------------------------------------------------
/** Main execution body
 */
void BinaryOperateMasks::exec() {

  // 1. Read input
  DataObjects::MaskWorkspace_const_sptr inputws1 =
      getProperty("InputWorkspace1");
  std::string op = getProperty("OperationType");

  // 2. Output
  DataObjects::MaskWorkspace_sptr outputws = getProperty("OutputWorkspace");

  if (outputws != inputws1) {
    // if the input and output are not the same, then create a new workspace for
    // the output.
    outputws = boost::make_shared<DataObjects::MaskWorkspace>(
        inputws1->getInstrument());
    outputws->copyFrom(inputws1);
  }

  // 3. Call Child Algorithm
  if (op == "NOT") {
    // Unary operation
    outputws->binaryOperation(Mantid::DataObjects::BinaryOperator::NOT);
  } else {
    // Binary operation
    // a. 2nd Input
    DataObjects::MaskWorkspace_const_sptr inputws2 =
        getProperty("InputWorkspace2");
    DataObjects::SpecialWorkspace2D_const_sptr inputws2_special(inputws2);

    unsigned int binop;
    if (op == "AND") {
      binop =
          static_cast<unsigned int>(Mantid::DataObjects::BinaryOperator::AND);
    } else if (op == "OR") {
      binop =
          static_cast<unsigned int>(Mantid::DataObjects::BinaryOperator::OR);
    } else if (op == "XOR") {
      binop =
          static_cast<unsigned int>(Mantid::DataObjects::BinaryOperator::XOR);
    } else {
      binop = 1000;
    }
    outputws->binaryOperation(inputws2_special, binop);
  }

  // 4. Output
  this->setProperty("OutputWorkspace", outputws);
}

} // namespace Algorithms
} // namespace Mantid
