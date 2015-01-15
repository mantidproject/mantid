#include "MantidMDEvents/ReflectometryTransformP.h"
#include "MantidKernel/System.h"
#include "MantidMDEvents/MDEventWorkspace.h"
#include "MantidGeometry/MDGeometry/MDHistoDimension.h"
#include "MantidDataObjects/Workspace2D.h"
#include <stdexcept>

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Geometry;

namespace Mantid {
namespace MDEvents {
/*
Constructor
@param pSumMin: p sum min value (extent)
@param pSumMax: p sum max value (extent)
@param pDiffMin: p diff min value (extent)
@param pDiffMax: p diff max value (extent)
@param incidentTheta: Predetermined incident theta value
@param numberOfBinsQx : Number of bins along the qx axis
@param numberOfBinsQz : Number of bins along the qz axis
*/
ReflectometryTransformP::ReflectometryTransformP(
    double pSumMin, double pSumMax, double pDiffMin, double pDiffMax,
    double incidentTheta, int numberOfBinsQx, int numberOfBinsQz)
    : ReflectometryTransform(numberOfBinsQx, numberOfBinsQz),
      m_pSumMin(pSumMin), m_pSumMax(pSumMax), m_pDiffMin(pDiffMin),
      m_pDiffMax(pDiffMax), m_pSumCalculation(incidentTheta),
      m_pDiffCalculation(incidentTheta) {
  if (pSumMin >= m_pSumMax) {
    throw std::invalid_argument("min sum p bounds must be < max sum p bounds");
  }
  if (pDiffMin >= pDiffMax) {
    throw std::invalid_argument(
        "min diff p bounds must be < max diff p bounds");
  }
  if (incidentTheta < 0 || incidentTheta > 90) {
    throw std::out_of_range("incident theta angle must be > 0 and < 90");
  }
}

//----------------------------------------------------------------------------------------------
/** Destructor
*/
ReflectometryTransformP::~ReflectometryTransformP() {}

Mantid::API::IMDEventWorkspace_sptr ReflectometryTransformP::executeMD(
    Mantid::API::MatrixWorkspace_const_sptr inputWs,
    BoxController_sptr boxController) const {
  MDHistoDimension_sptr pSumDim = MDHistoDimension_sptr(
      new MDHistoDimension("Pz_i + Pz_f", "sum_pz", "(Ang^-1)",
                           static_cast<Mantid::coord_t>(m_pSumMin),
                           static_cast<Mantid::coord_t>(m_pSumMax), m_nbinsx));
  MDHistoDimension_sptr pDiffDim = MDHistoDimension_sptr(
      new MDHistoDimension("Pz_i - Pz_f", "diff_pz", "(Ang^-1)",
                           static_cast<Mantid::coord_t>(m_pDiffMin),
                           static_cast<Mantid::coord_t>(m_pDiffMax), m_nbinsz));

  auto ws = createMDWorkspace(pSumDim, pDiffDim, boxController);

  auto spectraAxis = inputWs->getAxis(1);
  for (size_t index = 0; index < inputWs->getNumberHistograms(); ++index) {
    auto counts = inputWs->readY(index);
    auto wavelengths = inputWs->readX(index);
    auto errors = inputWs->readE(index);
    const size_t nInputBins = wavelengths.size() - 1;
    const double theta_final = spectraAxis->getValue(index);
    m_pSumCalculation.setThetaFinal(theta_final);
    m_pDiffCalculation.setThetaFinal(theta_final);
    // Loop over all bins in spectra
    for (size_t binIndex = 0; binIndex < nInputBins; ++binIndex) {
      const double &wavelength =
          0.5 * (wavelengths[binIndex] + wavelengths[binIndex + 1]);
      double _qx = m_pSumCalculation.execute(wavelength);
      double _qz = m_pDiffCalculation.execute(wavelength);
      double centers[2] = {_qx, _qz};

      ws->addEvent(MDLeanEvent<2>(float(counts[binIndex]),
                                  float(errors[binIndex] * errors[binIndex]),
                                  centers));
    }
  }
  ws->splitAllIfNeeded(NULL);
  ws->refreshCache();
  return ws;
}

/**
 * Convert to Pi-Pf, Pi+Pf
 * @param inputWs : Input Matrix workspace
 * @return workspace group containing output matrix workspaces of ki and kf
 */
Mantid::API::MatrixWorkspace_sptr ReflectometryTransformP::execute(
    Mantid::API::MatrixWorkspace_const_sptr inputWs) const {
  auto ws = boost::make_shared<Mantid::DataObjects::Workspace2D>();

  ws->initialize(m_nbinsz, m_nbinsx,
                 m_nbinsx); // Create the output workspace as a distribution

  // Mapping so that Psum and Pdiff values calculated can be added to the matrix
  // workspace at the correct index.
  const double gradPSum =
      double(m_nbinsx) / (m_pSumMax - m_pSumMin); // The x - axis
  const double gradPDiff =
      double(m_nbinsz) / (m_pDiffMax - m_pDiffMin); // Actually the y-axis
  const double cxToIndex = -gradPSum * m_pSumMin;
  const double cyToIndex = -gradPDiff * m_pDiffMin;
  const double cxToPSum = m_pSumMin - (1 / gradPSum);
  const double cyToPDiff = m_pDiffMin - (1 / gradPDiff);

  // Create an X - Axis.
  MantidVec xAxisVec = createXAxis(ws.get(), gradPSum, cxToPSum, m_nbinsx,
                                   "Pi + Pf", "1/Angstroms");
  // Create a Y (vertical) Axis
  createVerticalAxis(ws.get(), xAxisVec, gradPDiff, cyToPDiff, m_nbinsz,
                     "Pi - Pf", "1/Angstroms");

  // Loop over all entries in the input workspace and calculate Psum and Pdiff
  // for each.
  auto spectraAxis = inputWs->getAxis(1);
  for (size_t index = 0; index < inputWs->getNumberHistograms(); ++index) {
    auto counts = inputWs->readY(index);
    auto wavelengths = inputWs->readX(index);
    auto errors = inputWs->readE(index);
    const size_t nInputBins = wavelengths.size() - 1;
    const double theta_final = spectraAxis->getValue(index);
    m_pSumCalculation.setThetaFinal(theta_final);
    m_pDiffCalculation.setThetaFinal(theta_final);
    // Loop over all bins in spectra
    for (size_t binIndex = 0; binIndex < nInputBins; ++binIndex) {
      const double wavelength =
          0.5 * (wavelengths[binIndex] + wavelengths[binIndex + 1]);
      double _pSum = m_pSumCalculation.execute(wavelength);
      double _pDiff = m_pDiffCalculation.execute(wavelength);

      if (_pSum >= m_pSumMin && _pSum <= m_pSumMax && _pDiff >= m_pDiffMin &&
          _pDiff <=
              m_pDiffMax) // Check that the calculated ki and kf are in range
      {
        const int outIndexX = (int)((gradPSum * _pSum) + cxToIndex);
        const int outIndexZ = (int)((gradPDiff * _pDiff) + cyToIndex);

        ws->dataY(outIndexZ)[outIndexX] += counts[binIndex];
        ws->dataE(outIndexZ)[outIndexX] += errors[binIndex];
      }
    }
  }
  return ws;
}

} // namespace Mantid
} // namespace MDEvents
