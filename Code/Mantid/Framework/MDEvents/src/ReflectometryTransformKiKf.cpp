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
    @param boxController: box controller to apply on output workspace.
  */
  ReflectometryTransformKiKf::ReflectometryTransformKiKf(double kiMin, double kiMax, double kfMin, double kfMax, double incidentTheta, BoxController_sptr boxController)
    : ReflectometryMDTransform(boxController),  m_kiMin(kiMin), m_kiMax(kiMax), m_kfMin(kfMin), m_kfMax(kfMax), m_KiCalculation(incidentTheta)
  {
      if(kiMin >= kiMax)
      {
        throw std::invalid_argument("min ki bounds must be < max ki bounds");
      }
      if(kfMin >= kfMax)
      {
        throw std::invalid_argument("min kf bounds must be < max kf bounds");
      }
      if(incidentTheta < 0 || incidentTheta > 90)
      {
        throw std::out_of_range("incident theta angle must be > 0 and < 90");
      }
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
      MDHistoDimension_sptr kiDim = MDHistoDimension_sptr(new MDHistoDimension("Ki","ki","(Ang^-1)", static_cast<Mantid::coord_t>(m_kiMin), static_cast<Mantid::coord_t>(m_kiMax), m_nbinsx));
      MDHistoDimension_sptr kfDim = MDHistoDimension_sptr(new MDHistoDimension("Kf","kf","(Ang^-1)", static_cast<Mantid::coord_t>(m_kfMin), static_cast<Mantid::coord_t>(m_kfMax), m_nbinsz));

      auto ws = createWorkspace(kiDim, kfDim);

      auto spectraAxis = inputWs->getAxis(1);
      for(size_t index = 0; index < inputWs->getNumberHistograms(); ++index)
      {
        auto counts = inputWs->readY(index);
        auto wavelengths = inputWs->readX(index);
        auto errors = inputWs->readE(index);
        const size_t nInputBins = wavelengths.size() -1 ;
        const double theta_final = spectraAxis->getValue(index);
        CalculateReflectometryK kfCalculation(theta_final);
        //Loop over all bins in spectra 
        for(size_t binIndex = 0; binIndex < nInputBins; ++binIndex)
        {
          const double& wavelength = 0.5*(wavelengths[binIndex] + wavelengths[binIndex+1]);
          double _ki = m_KiCalculation.execute(wavelength);
          double _kf = kfCalculation.execute(wavelength);
          double centers[2] = {_ki, _kf};

          ws->addEvent(MDLeanEvent<2>(float(counts[binIndex]), float(errors[binIndex]*errors[binIndex]), centers));
        }
      }
      ws->splitAllIfNeeded(NULL);
      ws->refreshCache();
      return ws;
  }


} // namespace Mantid
} // namespace MDEvents
