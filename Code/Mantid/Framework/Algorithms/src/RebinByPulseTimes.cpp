#include "MantidAlgorithms/RebinByPulseTimes.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidKernel/VectorHelper.h"
#include "MantidKernel/Unit.h"
#include <boost/make_shared.hpp>
#include <algorithm>

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;

namespace Mantid
{
  namespace Algorithms
  {

    // Register the algorithm into the AlgorithmFactory
    DECLARE_ALGORITHM(RebinByPulseTimes)

    //----------------------------------------------------------------------------------------------
    /** Constructor
     */
    RebinByPulseTimes::RebinByPulseTimes()
    {
    }

    //----------------------------------------------------------------------------------------------
    /** Destructor
     */
    RebinByPulseTimes::~RebinByPulseTimes()
    {
    }

    //----------------------------------------------------------------------------------------------
    /// Algorithm's name for identification. @see Algorithm::name
    const std::string RebinByPulseTimes::name() const
    {
      return "RebinByPulseTimes";
    }
    ;

    /// Algorithm's version for identification. @see Algorithm::version
    int RebinByPulseTimes::version() const
    {
      return 1;
    }
    ;

    /// Algorithm's category for identification. @see Algorithm::category
    const std::string RebinByPulseTimes::category() const
    {
      return "Transforms\\Rebin";
    }

    //----------------------------------------------------------------------------------------------

    /// Do the algorithm specific histogramming.
    void RebinByPulseTimes::doHistogramming(IEventWorkspace_sptr inWS,
        MatrixWorkspace_sptr outputWS, MantidVecPtr& XValues_new,
        MantidVec& OutXValues_scaled, Progress& prog)
    {

      // workspace independent determination of length
      const int histnumber = static_cast<int>(inWS->getNumberHistograms());

      PARALLEL_FOR2(inWS, outputWS)
      for (int i = 0; i < histnumber; ++i)
      {
        PARALLEL_START_INTERUPT_REGION

        const IEventList* el = inWS->getEventListPtr(i);
        MantidVec y_data, e_data;
        // The EventList takes care of histogramming.
        el->generateHistogramPulseTime(*XValues_new, y_data, e_data);

        //Set the X axis for each output histogram
        outputWS->setX(i, OutXValues_scaled);

        //Copy the data over.
        outputWS->dataY(i).assign(y_data.begin(), y_data.end());
        outputWS->dataE(i).assign(e_data.begin(), e_data.end());

        //Report progress
        prog.report(name());
      PARALLEL_END_INTERUPT_REGION
    }
  PARALLEL_CHECK_INTERUPT_REGION
}


} // namespace Algorithms
} // namespace Mantid
