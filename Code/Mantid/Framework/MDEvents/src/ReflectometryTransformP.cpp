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
    @param pSumMin: p sum min value (extent)
    @param pSumMax: p sum max value (extent)
    @param pDiffMin: p diff min value (extent)
    @param pDiffMax: p diff max value (extent)
    @param incidentTheta: Predetermined incident theta value
    */
    ReflectometryTransformP::ReflectometryTransformP(double pSumMin, double pSumMax, double pDiffMin, double pDiffMax, double incidentTheta)
      :  m_pSumMin(pSumMin), m_pSumMax(pSumMax), m_pDiffMin(pDiffMin), m_pDiffMax(pDiffMax), m_pSumCalculation(incidentTheta), m_pDiffCalculation(incidentTheta)
    {
      if(pSumMin >= m_pSumMax)
      {
        throw std::invalid_argument("min sum p bounds must be < max sum p bounds");
      }
      if(pDiffMin >= pDiffMax)
      {
        throw std::invalid_argument("min diff p bounds must be < max diff p bounds");
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

    Mantid::API::IMDEventWorkspace_sptr ReflectometryTransformP::executeMD(Mantid::API::MatrixWorkspace_const_sptr inputWs, BoxController_sptr boxController) const
    {
      MDHistoDimension_sptr pSumDim = MDHistoDimension_sptr(new MDHistoDimension("Pz_i + Pz_f","sum_pz","(Ang^-1)", static_cast<Mantid::coord_t>(m_pSumMin), static_cast<Mantid::coord_t>(m_pSumMax), m_nbinsx));
      MDHistoDimension_sptr pDiffDim = MDHistoDimension_sptr(new MDHistoDimension("Pz_i - Pz_f","diff_pz","(Ang^-1)", static_cast<Mantid::coord_t>(m_pDiffMin), static_cast<Mantid::coord_t>(m_pDiffMax), m_nbinsz));

      auto ws = createMDWorkspace(pSumDim, pDiffDim, boxController);

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
      }
      ws->splitAllIfNeeded(NULL);
      ws->refreshCache();
      return ws;
    }



  } // namespace Mantid
} // namespace MDEvents
