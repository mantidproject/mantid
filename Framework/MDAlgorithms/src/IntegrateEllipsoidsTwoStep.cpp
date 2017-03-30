#include "MantidMDAlgorithms/IntegrateEllipsoidsTwoStep.h"

#include <string>

namespace Mantid {
namespace MDAlgorithms {

    //---------------------------------------------------------------------
    /// Algorithm's name for identification. @see Algorithm::name
    const std::string IntegrateEllipsoidsTwoStep::name() const {
        return "IntegrateEllipsoidsTwoStep";
    }

    /// Algorithm's version for identification. @see Algorithm::version
    int IntegrateEllipsoidsTwoStep::version() const { return 1; }

    /// Algorithm's category for identification. @see Algorithm::category
    const std::string IntegrateEllipsoidsTwoStep::category() const {
        return "Crystal\\Integration";
    }


    void IntegrateEllipsoidsTwoStep::init() {
    }

    void IntegrateEllipsoidsTwoStep::exec() {
    }
}
}
