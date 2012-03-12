//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidMDAlgorithms/QuadEnBackground.h"
#include <math.h>

#include "MantidKernel/Tolerance.h"
#include "MantidGeometry/Math/mathSupport.h"
#include "MantidKernel/Matrix.h"

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
        double QuadEnBackground::functionMD(const Mantid::API::IMDIterator& it) const
        {
            double linear = getParameter("Linear");
            double constant = getParameter("Constant");
            double quadratic = getParameter("Quadratic");
            double bgSignal = 0.;
            double eps=0.;

            // Loop over events - Note Events in this context must be detectors NOT
            // "events" in the more general sense.
            for (size_t i=0; i < it.getNumEvents(); i++)
            {
              // taking 4th coordinate = energy
              eps = it.getInnerPosition(i, 3);
              bgSignal += constant+eps*(linear+eps*quadratic);
            }

            // return normalized background - normalized by number of events NOT by box volume.
            return bgSignal/static_cast<double>(it.getNumEvents());
        }
    }
}
