#include "MantidMDEvents/ReflectometryTransformKiKf.h"
#include "MantidMDEvents/MDEventWorkspace.h"
#include "MantidGeometry/MDGeometry/MDHistoDimension.h"
#include <stdexcept>

using namespace Mantid::Kernel;
using namespace Mantid::Geometry;
using namespace Mantid::API;

namespace Mantid
{
namespace MDEvents
{

  /*
    Constructor
    @param kiMin: min ki value (extent)
    @param kiMax: max ki value (extent)
    @param kfMin: min kf value (extent)
    @param kfMax; max kf value (extent)
    @param incidentTheta: Predetermined incident theta value
  */
  ReflectometryTransformKiKf::ReflectometryTransformKiKf(double kiMin, double kiMax, double kfMin, double kfMax, double incidentTheta) 
    : m_kiMin(kiMin), m_kiMax(kiMax), m_kfMin(kfMin), m_kfMax(kfMax), m_KiCalculation(incidentTheta)
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  ReflectometryTransformKiKf::~ReflectometryTransformKiKf()
  {
  }
  
  /*
    Execute the transformtion. Generates an output IMDEventWorkspace.
    @return the constructed IMDEventWorkspace following the transformation.
    @param ws: Input MatrixWorkspace const shared pointer
  */
  Mantid::API::IMDEventWorkspace_sptr ReflectometryTransformKiKf::execute(Mantid::API::MatrixWorkspace_const_sptr inputWs) const
  {
      const size_t nbinsx = 10;
      const size_t nbinsz = 10;

      auto ws = boost::make_shared<MDEventWorkspace<MDLeanEvent<2>,2> >();
      MDHistoDimension_sptr kiDim = MDHistoDimension_sptr(new MDHistoDimension("Ki","ki","(Ang^-1)", static_cast<Mantid::coord_t>(m_kiMin), static_cast<Mantid::coord_t>(m_kiMax), nbinsx)); 
      MDHistoDimension_sptr kfDim = MDHistoDimension_sptr(new MDHistoDimension("Kf","kf","(Ang^-1)", static_cast<Mantid::coord_t>(m_kfMin), static_cast<Mantid::coord_t>(m_kfMax), nbinsz)); 

      ws->addDimension(kiDim);
      ws->addDimension(kfDim);

      // Set some reasonable values for the box controller
      BoxController_sptr bc = ws->getBoxController();
      bc->setSplitInto(2);
      bc->setSplitThreshold(10);

      // Initialize the workspace.
      ws->initialize();

      // Start with a MDGridBox.
      ws->splitBox();

      auto spectraAxis = inputWs->getAxis(1);
      for(size_t index = 0; index < inputWs->getNumberHistograms(); ++index)
      {
        auto counts = inputWs->readY(index);
        auto wavelengths = inputWs->readX(index);
        auto errors = inputWs->readE(index);
        size_t nInputBins = inputWs->isHistogramData() ? wavelengths.size() -1 : wavelengths.size();
        const double theta_final = spectraAxis->getValue(index);
        CalculateReflectometryK kfCalculation(theta_final);
        //Loop over all bins in spectra 
        for(size_t binIndex = 0; binIndex < nInputBins; ++binIndex)
        {
          const double& lambda = wavelengths[binIndex];
          double _ki = m_KiCalculation.execute(lambda);
          double _kf = kfCalculation.execute(lambda);
          double centers[2] = {_ki, _kf};

          ws->addEvent(MDLeanEvent<2>(float(counts[binIndex]), float(errors[binIndex]*errors[binIndex]), centers));
        }
        ws->splitAllIfNeeded(NULL);
      }
      return ws;
  }


} // namespace Mantid
} // namespace MDEvents