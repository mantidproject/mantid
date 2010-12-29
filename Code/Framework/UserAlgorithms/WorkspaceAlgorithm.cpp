#include "WorkspaceAlgorithm.h"

namespace Mantid
{
namespace Algorithms
{

// Algorithm must be declared
DECLARE_ALGORITHM(WorkspaceAlgorithm)

using namespace Kernel;
using namespace API;

/**  Initialization code
 *
 *   Properties have to be declared here before they can be used
*/
void WorkspaceAlgorithm::init()
{

    // Declare a 1D workspace property.
    declareProperty(new WorkspaceProperty<>("Workspace","",Direction::Input));

}

/** Executes the algorithm
 */
void WorkspaceAlgorithm::exec()
{
  	// g_log is a reference to the logger. It is used to print out information,
		// warning, and error messages  
		g_log.information() << "Running algorithm " << name() << " version " << version() << std::endl;

    // Get the input workspace
    MatrixWorkspace_const_sptr workspace = getProperty("Workspace");

    // Number of single indexable items in the workspace
    g_log.information() << "Number of items = " << workspace->size() << std::endl;

    int count = 0;
    // Iterate over the workspace
    for(MatrixWorkspace::const_iterator ti(*workspace); ti != ti.end(); ++ti)
    {
        // Get the reference to a data point
        LocatedDataRef tr = *ti;
        g_log.information() << "Point number " << count++ << " values: "
            << tr.X() << ' ' << tr.Y() << ' ' << tr.E() << std::endl;
    }

    count = 0;
    int loopCount = 2;
    // Do several loops
    for(MatrixWorkspace::const_iterator ti(*workspace,loopCount,LoopOrientation::Horizontal); ti != ti.end(); ++ti)
    {
        // Get the reference to a data point
        LocatedDataRef tr = *ti;
        g_log.information() << "Point number " << count++ << " values: "
            << tr.X() << ' ' << tr.Y() << ' ' << tr.E() << std::endl;
    }

}

}
}

