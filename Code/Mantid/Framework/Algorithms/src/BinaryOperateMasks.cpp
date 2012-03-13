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
    DataObjects::SpecialWorkspace2D_const_sptr sws1 = getProperty("InputWorkspace1");
    std::string op = getProperty("OperationType");

    std::cout <<"\nOperator = " << op << std::endl;

    // 2. Output
    API::MatrixWorkspace_sptr oprawws;
    Mantid::DataObjects::SpecialWorkspace2D_sptr outputws;

    // std::string outputwsname = getPropertyValue("OutputWorkspace");
    // std::string inputws1name = getPropertyValue("InputWorkspace1");

    outputws = getProperty("OutputWorkspace");

    // std::cout << "\nb) Output workspace is read in with name " << outputwsname << std::endl;

    if (sws1 == outputws){
      // oprawws = getProperty("OutputWorkspace");
      outputws = boost::dynamic_pointer_cast<Mantid::DataObjects::SpecialWorkspace2D>(oprawws);

    } else {
      // No existing workspace... generating a new one and copy it from sws1

      /*** This does not work
      IAlgorithm_sptr alg = createSubAlgorithm("CloneWorkspace");
      alg->setProperty("InputWorkspace", sws1);
      alg->setPropertyValue("OutputWorkspace", outputwsname);
      alg->executeAsSubAlg();
      oprawws = alg->getProperty("OutputWorkspace");
      outputws = boost::dynamic_pointer_cast<Mantid::DataObjects::SpecialWorkspace2D>(oprawws);
      ***/

      outputws = boost::dynamic_pointer_cast<DataObjects::SpecialWorkspace2D>(API::WorkspaceFactory::Instance().create(sws1));
      // std::cout << "\nc) Number of spectrum = " << outputws->getNumberHistograms() << std::endl;
      outputws->copyFrom(sws1);
    }

    // std::cout << "\nd) Here..." << std::endl;

    // 3. Call Sub Algorithm
    if (op == "NOT"){

      for (size_t ih = 0; ih < 20; ih ++){
        detid_t idet = outputws->getDetectorID(ih);
        g_log.debug() << idet << " -- RAW:  " << outputws->getValue(idet) << std::endl;
      }

      // Unary operation
      outputws->binaryOperation(Mantid::DataObjects::BinaryOperator::NOT);

      /*
      for (size_t ih = 0; ih < 20; ih ++){
        detid_t idet = outputws->getDetectorID(ih);
        g_log.debug() << idet << ": " << sws1->getValue(idet) << " --> " << outputws->getValue(idet) << std::endl;
      }
      */

    } else {
      // Binary operation
      // a. 2nd Input
      DataObjects::SpecialWorkspace2D_const_sptr sws2 = getProperty("InputWorkspace2");

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
      outputws->binaryOperation(sws2, binop);

      // Debug Output
      /*
      for (size_t ih = 0; ih < 20; ih ++){
        detid_t idet = outputws->getDetectorID(ih);
        g_log.debug() << idet << ": " << sws1->getValue(idet) << " xxx  " << sws2->getValue(idet) << " --> " << outputws->getValue(idet) << std::endl;
      }
      */

    }

    // 4. Output
    this->setProperty("OutputWorkspace", outputws);

    return;
  }





} // namespace Mantid
} // namespace Algorithms

