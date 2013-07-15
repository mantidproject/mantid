#include "MantidMDEvents/ReflectometryTransformQxQz.h"
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

    //----------------------------------------------------------------------------------------------
    /** Destructor
    */
    ReflectometryTransformQxQz::~ReflectometryTransformQxQz()
    {
    }

    /*
    Constructor
    @param qxMin: min qx value (extent)
    @param qxMax: max qx value (extent)
    @param qzMin: min qz value (extent)
    @param qzMax; max qz value (extent)
    @param incidentTheta: Predetermined incident theta value
    @param boxController: Box controller to apply to output workspace
    */
    ReflectometryTransformQxQz::ReflectometryTransformQxQz(double qxMin, double qxMax, double qzMin, double qzMax, double incidentTheta, BoxController_sptr boxController):
        ReflectometryMDTransform(boxController), m_qxMin(qxMin), m_qxMax(qxMax), m_qzMin(qzMin), m_qzMax(qzMax), m_QxCalculation(incidentTheta), m_QzCalculation(incidentTheta)
    {
      if(qxMin >= qxMax)
      {
        throw std::invalid_argument("min qx bounds must be < max qx bounds");
      }
      if(qzMin >= qzMax)
      {
        throw std::invalid_argument("min qz bounds must be < max qz bounds");
      }
      if(incidentTheta < 0 || incidentTheta > 90)
      {
        throw std::out_of_range("incident theta angle must be > 0 and < 90");
      }
    }

    /*
    Execute the transformtion. Generates an output IMDEventWorkspace.
    @return the constructed IMDEventWorkspace following the transformation.
    @param ws: Input MatrixWorkspace const shared pointer
    */
    IMDEventWorkspace_sptr ReflectometryTransformQxQz::executeMD(MatrixWorkspace_const_sptr inputWs) const
    {

      MDHistoDimension_sptr qxDim = MDHistoDimension_sptr(new MDHistoDimension("Qx","qx","(Ang^-1)", static_cast<Mantid::coord_t>(m_qxMin), static_cast<Mantid::coord_t>(m_qxMax), m_nbinsx));
      MDHistoDimension_sptr qzDim = MDHistoDimension_sptr(new MDHistoDimension("Qz","qz","(Ang^-1)", static_cast<Mantid::coord_t>(m_qzMin), static_cast<Mantid::coord_t>(m_qzMax), m_nbinsz));

      auto ws = createWorkspace(qxDim, qzDim);

      auto spectraAxis = inputWs->getAxis(1);
      for(size_t index = 0; index < inputWs->getNumberHistograms(); ++index)
      {
        auto counts = inputWs->readY(index);
        auto wavelengths = inputWs->readX(index);
        auto errors = inputWs->readE(index);
        const size_t nInputBins =  wavelengths.size() -1;
        const double theta_final = spectraAxis->getValue(index);
        m_QxCalculation.setThetaFinal(theta_final);
        m_QzCalculation.setThetaFinal(theta_final);
        //Loop over all bins in spectra 
        for(size_t binIndex = 0; binIndex < nInputBins; ++binIndex)
        {
          const double& wavelength = 0.5*(wavelengths[binIndex] + wavelengths[binIndex+1]);
          double _qx = m_QxCalculation.execute(wavelength);
          double _qz = m_QzCalculation.execute(wavelength);
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
