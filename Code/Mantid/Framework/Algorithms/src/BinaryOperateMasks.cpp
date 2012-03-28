/*WIKI* 


*WIKI*/
#include "MantidAlgorithms/BinaryOperateMasks.h"
#include "MantidKernel/System.h"
#include "MantidDataObjects/SpecialWorkspace2D.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidKernel/ListValidator.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid
{
namespace Algorithms
{

  DECLARE_ALGORITHM(BinaryOperateMasks)

  //enum BinaryOperator{AND, OR, XOR, NOT};

  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  BinaryOperateMasks::BinaryOperateMasks()
  {
    // TODO Auto-generated constructor stub
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  BinaryOperateMasks::~BinaryOperateMasks()
  {
    // TODO Auto-generated destructor stub
  }
  
  void BinaryOperateMasks::initDocs(){

    return;
  }

  void BinaryOperateMasks::init(){

    std::vector<std::string> operators;
    operators.push_back("AND");
    operators.push_back("OR");
    operators.push_back("XOR");
    operators.push_back("NOT");

    declareProperty(new WorkspaceProperty<DataObjects::SpecialWorkspace2D>("InputWorkspace1", "", Direction::Input),
        "SpecialWorkspace2D 1 for binary operation");
    declareProperty(new WorkspaceProperty<DataObjects::SpecialWorkspace2D>("InputWorkspace2", "", Direction::Input, PropertyMode::Optional),
        "Optional SpecialWorkspace2D 2 for binary operation");
    declareProperty("OperationType", "AND", boost::make_shared<StringListValidator>(operators),
        "Operator for Workspace1 and Workspace2");
    declareProperty(new WorkspaceProperty<DataObjects::SpecialWorkspace2D>("OutputWorkspace", "", Direction::Output),
        "Output SpecialWorkspace2D as result of binary operation");

    return;
  }

  void BinaryOperateMasks::exec(){

    // 1. Read input
    DataObjects::SpecialWorkspace2D_const_sptr inputws1 = getProperty("InputWorkspace1");
    std::string op = getProperty("OperationType");

    // 2. Output
    Mantid::DataObjects::SpecialWorkspace2D_sptr outputws = getProperty("OutputWorkspace");

    if (outputws != inputws1)
    {
        // if the input and output are not the same, then create a new workspace for the output.
         outputws = boost::dynamic_pointer_cast<DataObjects::SpecialWorkspace2D>(API::WorkspaceFactory::Instance().create(inputws1));
         outputws->copyFrom(inputws1);
    }

    // 3. Call Sub Algorithm
    if (op == "NOT"){

        // Unary operation
        outputws->binaryOperation(Mantid::DataObjects::BinaryOperator::NOT);

    } else {
        // Binary operation
        // a. 2nd Input
        DataObjects::SpecialWorkspace2D_const_sptr inputws2 = getProperty("InputWorkspace2");

        unsigned int binop;
        if (op == "AND"){
            binop = (unsigned int)Mantid::DataObjects::BinaryOperator::AND;
        } else if (op == "OR"){
            binop = (unsigned int)Mantid::DataObjects::BinaryOperator::OR;
        } else if (op == "XOR"){
            binop = (unsigned int)Mantid::DataObjects::BinaryOperator::XOR;
        } else{
            binop = 1000;
        }
        outputws->binaryOperation(inputws2, binop);

    }

    // 4. Output
    this->setProperty("OutputWorkspace", outputws);

    return;
  }

} // namespace Mantid
} // namespace Algorithms

