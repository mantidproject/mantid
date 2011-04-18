//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidMDAlgorithms/QuadEnBackground.h"
#include <math.h>

#include "MantidGeometry/Tolerance.h"
#include "MantidGeometry/Math/mathSupport.h"
#include "MantidGeometry/Math/Matrix.h"

namespace Mantid
{
    namespace MDAlgorithms
    {

        using namespace Mantid::Kernel;
        using namespace Mantid::API;

        DECLARE_FUNCTION(QuadEnBackground)

        QuadEnBackground::QuadEnBackground()
        {
            declareParameter("Constant", 0.0);
            declareParameter("Linear", 0.0);
            declareParameter("Quadratic", 0.0);
        }

        // quadratic term in energy background defined for MDiterator 
        double QuadEnBackground::function(Mantid::API::IMDIterator& it) const
        {
            double linear = getParameter("Linear");
            double constant = getParameter("Constant");
            double quadratic = getParameter("Quadratic");
            double bgSignal = 0.;
            double eps=0.;
            const Mantid::Geometry::SignalAggregate& cell = m_workspace->getPoint(it.getPointer());
            std::vector<boost::shared_ptr<Mantid::Geometry::MDPoint> > points = cell.getContributingPoints();
            // assume that the first point in the vertexes array is the centre point anf that the gett getter
            // returns a energy value in the appropriate units.
            for(size_t i=0; i<points.size();i++){
                std::vector<Mantid::Geometry::coordinate> vertexes = points[i]->getVertexes();
                eps=vertexes[0].gett();
                //int run=points[i]->getRunId(); // testing
                bgSignal+=constant+eps*(linear+eps*quadratic);
            }
            return bgSignal;
        }
    }
}
