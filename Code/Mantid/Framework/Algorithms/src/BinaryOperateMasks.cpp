#include "MantidAlgorithms/BinaryOperateMasks.h"
#include "MantidKernel/System.h"
#include "MantidDataObjects/MaskWorkspace.h"
#include "MantidAPI/WorkspaceValidators.h"
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

  std::vector<std::string> operators;
  operators.push_back("AND");
  operators.push_back("OR");
  operators.push_back("XOR");
  operators.push_back("NOT");

  declareProperty(new WorkspaceProperty<DataObjects::MaskWorkspace>(
                      "InputWorkspace1", "", Direction::Input),
                  "MaskWorkspace 1 for binary operation");
  declareProperty(
      new WorkspaceProperty<DataObjects::MaskWorkspace>(
          "InputWorkspace2", "", Direction::Input, PropertyMode::Optional),
      "Optional MaskWorkspace 2 for binary operation");
  declareProperty("OperationType", "AND",
                  boost::make_shared<StringListValidator>(operators),
                  "Operator for Workspace1 and Workspace2");
  declareProperty(new WorkspaceProperty<DataObjects::MaskWorkspace>(
                      "OutputWorkspace", "", Direction::Output),
                  "Output MaskWorkspace as result of binary operation");

  return;
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
  Mantid::DataObjects::MaskWorkspace_sptr outputws =
      getProperty("OutputWorkspace");

  if (outputws != inputws1) {
    // if the input and output are not the same, then create a new workspace for
    // the output.
    outputws = boost::dynamic_pointer_cast<DataObjects::MaskWorkspace>(
        API::WorkspaceFactory::Instance().create(inputws1));
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
      binop = (unsigned int)Mantid::DataObjects::BinaryOperator::AND;
    } else if (op == "OR") {
      binop = (unsigned int)Mantid::DataObjects::BinaryOperator::OR;
    } else if (op == "XOR") {
      binop = (unsigned int)Mantid::DataObjects::BinaryOperator::XOR;
    } else {
      binop = 1000;
    }
    outputws->binaryOperation(inputws2_special, binop);
  }

  // 4. Output
  this->setProperty("OutputWorkspace", outputws);

  return;
}

} // namespace Mantid
} // namespace Algorithms
