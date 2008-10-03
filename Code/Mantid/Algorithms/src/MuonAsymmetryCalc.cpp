//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/MuonAsymmetryCalc.h"
#include "MantidAPI/Workspace.h"
#include "MantidDataObjects/Workspace2D.h"

namespace Mantid
{
  namespace Algorithms
  {

   using namespace Kernel;

    // Register the class into the algorithm factory
    DECLARE_ALGORITHM(MuonAsymmetryCalc)

    // Get a reference to the logger
    Logger& MuonAsymmetryCalc::g_log = Logger::get("MuonAsymmetryCalc");

    /** Initialisation method. Declares properties to be used in algorithm.
    *
    */
    void MuonAsymmetryCalc::init()
    {
       declareProperty(new API::WorkspaceProperty<API::Workspace>("InputWorkspace","",Direction::Input));
       declareProperty(new API::WorkspaceProperty<API::Workspace>("OutputWorkspace","",Direction::Output));
	    
       BoundedValidator<int> *zeroOrGreater = new BoundedValidator<int>();
       zeroOrGreater->setLower(0);
       declareProperty("ForwardSpectrum",0,zeroOrGreater );
       declareProperty("BackwardSpectrum",0,zeroOrGreater->clone() );
       declareProperty("Alpha",0,Direction::Input);
    }

    /** Executes the algorithm
     *
     */
    void MuonAsymmetryCalc::exec()
    {
	    
    }

  } // namespace Algorithm
} // namespace Mantid




