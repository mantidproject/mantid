#include "MantidMDEvents/ReflectometryTransformP.h"
#include "MantidKernel/System.h"
#include "MantidMDEvents/MDEventWorkspace.h"
#include "MantidGeometry/MDGeometry/MDHistoDimension.h"
#include <stdexcept>

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Geometry;

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
    ReflectometryTransformP::ReflectometryTransformP(double kiMin, double kiMax, double kfMin, double kfMax, double incidentTheta)
      : m_kiMin(kiMin), m_kiMax(kiMax), m_kfMin(kfMin), m_kfMax(kfMax), m_pSumCalculation(incidentTheta), m_pDiffCalculation(incidentTheta)
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
    ReflectometryTransformP::~ReflectometryTransformP()
    {
    }

    Mantid::API::IMDEventWorkspace_sptr ReflectometryTransformP::execute(Mantid::API::MatrixWorkspace_const_sptr inputWs) const
    {
      const size_t nbinsx = 10;
      const size_t nbinsz = 10;

      auto ws = boost::make_shared<MDEventWorkspace<MDLeanEvent<2>,2> >();
      MDHistoDimension_sptr pSumDim = MDHistoDimension_sptr(new MDHistoDimension("Pz_i + Pz_f","sum_pz","(Ang^-1)", static_cast<Mantid::coord_t>(m_kiMin + m_kfMin), static_cast<Mantid::coord_t>(m_kiMax + m_kfMax), nbinsx)); 
      MDHistoDimension_sptr pDiffDim = MDHistoDimension_sptr(new MDHistoDimension("Pz_i - Pz_f","diff_pz","(Ang^-1)", static_cast<Mantid::coord_t>(m_kiMin - m_kfMin), static_cast<Mantid::coord_t>(m_kiMax - m_kfMax), nbinsz)); 

      ws->addDimension(pSumDim);
      ws->addDimension(pDiffDim);

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
        const size_t nInputBins = wavelengths.size() -1;
        const double theta_final = spectraAxis->getValue(index);
        m_pSumCalculation.setThetaFinal(theta_final);
        m_pDiffCalculation.setThetaFinal(theta_final);
        //Loop over all bins in spectra 
        for(size_t binIndex = 0; binIndex < nInputBins; ++binIndex)
        {
          const double& wavelength = 0.5*(wavelengths[binIndex] + wavelengths[binIndex+1]);
          double _qx = m_pSumCalculation.execute(wavelength);
          double _qz = m_pDiffCalculation.execute(wavelength);
          double centers[2] = {_qx, _qz};

          ws->addEvent(MDLeanEvent<2>(float(counts[binIndex]), float(errors[binIndex]*errors[binIndex]), centers));
        }
        ws->splitAllIfNeeded(NULL);
      }
      return ws;
    }



  } // namespace Mantid
} // namespace MDEvents