//TODO

//#include "MantidKernel/DynamicFactory.h"
//#include "DllExport.h"
//#include "IImplicitFunction.h"
//
//namespace Mantid
//{
//  namespace API
//  {
//   class EXPORT_OPT_MANTID_API ImplicitFunctionCreator
//   {
//   private:
//     
//     void FunctionParser* setUp(std::string setupXML)
//     {
//       FunctionParser* planeParser = new PlaneFunctionParser;
//       rootParser = new CompositeParser();
//       rootParser.setSuccessor(planeParser);
//       return rootParser;
//
//     }
//     
//     static IImplicitFunction* create(std::string& xmlFunctionString);
//     {
//       setUp(xmlFunctionString).parse(xmlFunctionString);
//     }
//   };
//  }
//}