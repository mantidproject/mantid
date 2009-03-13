//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include <cmath>
#include "MantidAlgorithms/MinusExplicit.h"
#include "MantidKernel/VectorHelper.h"
using namespace Mantid::API;
using namespace Mantid::Kernel;

namespace Mantid
{
  namespace Algorithms
  {
    // Register the class into the algorithm factory
    DECLARE_ALGORITHM(MinusExplicit)

    // Get a reference to the logger
    Logger& MinusExplicit::g_log = Logger::get("MinusExplicit");

    void MinusExplicit::init()
    {
    	declareProperty(new WorkspaceProperty<>("Input1","",Direction::Input));
    	declareProperty(new WorkspaceProperty<>("Input2","",Direction::Input));
    	declareProperty(new WorkspaceProperty<>("OutputWorkspace","",Direction::Output));
    }

    void MinusExplicit::exec()
    {
			API::MatrixWorkspace_const_sptr iW1=getProperty("Input1");
			API::MatrixWorkspace_const_sptr iW2=getProperty("Input2");
			API::MatrixWorkspace_sptr out=API::WorkspaceFactory::Instance().create(iW1);

			const int nHist=iW1->getNumberHistograms();

			for (int i=0;i<nHist;i++)
			{
				const std::vector<double>& y1=iW1->readY(i);
				const std::vector<double>& y2=iW2->readY(i);
				std::vector<double>& yout=out->dataY(i);
				std::transform(y1.begin(),y1.end(),y2.begin(),yout.begin(),std::minus<double>());
				const std::vector<double>& e1=iW1->readE(i);
				const std::vector<double>& e2=iW2->readE(i);
				std::vector<double>& eout=out->dataE(i);
				std::transform(e1.begin(),e1.end(),e2.begin(),eout.begin(),SumGaussError<double>());

			}
			setProperty("OutputWorkspace",out);
			return;
    }

  } // Namespace Algorithms
}// Namespace Mantid
