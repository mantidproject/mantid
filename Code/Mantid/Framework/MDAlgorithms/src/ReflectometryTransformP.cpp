#include "MantidMDAlgorithms/ReflectometryTransformP.h"

#include "MantidDataObjects/MDEventFactory.h"
#include "MantidDataObjects/MDEventWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidGeometry/MDGeometry/MDHistoDimension.h"
#include "MantidKernel/Exception.h"

using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;

namespace Mantid {
namespace MDAlgorithms {
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
    : ReflectometryTransform("Pz_i + Pz_f", "sum_pz", pSumMin, pSumMax,
                             "Pz_i - Pz_f", "diff_pz", pDiffMin, pDiffMax,
                             numberOfBinsQx, numberOfBinsQz) {
  if (incidentTheta < 0 || incidentTheta > 90) {
    throw std::out_of_range("incident theta angle must be > 0 and < 90");
  }
  m_pCalculation.setThetaIncident(incidentTheta);
}

//----------------------------------------------------------------------------------------------
/** Destructor
*/
ReflectometryTransformP::~ReflectometryTransformP() {}

Mantid::API::IMDEventWorkspace_sptr ReflectometryTransformP::executeMD(
    Mantid::API::MatrixWorkspace_const_sptr inputWs,
    BoxController_sptr boxController) const {
  MDHistoDimension_sptr pSumDim = MDHistoDimension_sptr(new MDHistoDimension(
      m_d0Label, m_d0ID, "(Ang^-1)", static_cast<Mantid::coord_t>(m_d0Min),
      static_cast<Mantid::coord_t>(m_d0Max), m_d0NumBins));
  MDHistoDimension_sptr pDiffDim = MDHistoDimension_sptr(new MDHistoDimension(
      m_d1Label, m_d1ID, "(Ang^-1)", static_cast<Mantid::coord_t>(m_d1Min),
      static_cast<Mantid::coord_t>(m_d1Max), m_d1NumBins));

  auto ws = createMDWorkspace(pSumDim, pDiffDim, boxController);

  auto spectraAxis = inputWs->getAxis(1);
  for (size_t index = 0; index < inputWs->getNumberHistograms(); ++index) {
    auto counts = inputWs->readY(index);
    auto wavelengths = inputWs->readX(index);
    auto errors = inputWs->readE(index);
    const size_t nInputBins = wavelengths.size() - 1;
    const double theta_final = spectraAxis->getValue(index);
    m_pCalculation.setThetaFinal(theta_final);
    // Loop over all bins in spectra
    for (size_t binIndex = 0; binIndex < nInputBins; ++binIndex) {
      const double &wavelength =
          0.5 * (wavelengths[binIndex] + wavelengths[binIndex + 1]);
      double _qx = m_pCalculation.calculateDim0(wavelength);
      double _qz = m_pCalculation.calculateDim1(wavelength);
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

  ws->initialize(m_d1NumBins, m_d0NumBins,
                 m_d0NumBins); // Create the output workspace as a distribution

  // Mapping so that Psum and Pdiff values calculated can be added to the matrix
  // workspace at the correct index.
  const double gradPSum =
      double(m_d0NumBins) / (m_d0Max - m_d0Min); // The x - axis
  const double gradPDiff =
      double(m_d1NumBins) / (m_d1Max - m_d1Min); // Actually the y-axis
  const double cxToIndex = -gradPSum * m_d0Min;
  const double cyToIndex = -gradPDiff * m_d1Min;
  const double cxToPSum = m_d0Min - (1 / gradPSum);
  const double cyToPDiff = m_d1Min - (1 / gradPDiff);

  // Create an X - Axis.
  MantidVec xAxisVec = createXAxis(ws.get(), gradPSum, cxToPSum, m_d0NumBins,
                                   m_d0Label, "1/Angstroms");
  // Create a Y (vertical) Axis
  createVerticalAxis(ws.get(), xAxisVec, gradPDiff, cyToPDiff, m_d1NumBins,
                     m_d1Label, "1/Angstroms");

  // Loop over all entries in the input workspace and calculate Psum and Pdiff
  // for each.
  auto spectraAxis = inputWs->getAxis(1);
  for (size_t index = 0; index < inputWs->getNumberHistograms(); ++index) {
    auto counts = inputWs->readY(index);
    auto wavelengths = inputWs->readX(index);
    auto errors = inputWs->readE(index);
    const size_t nInputBins = wavelengths.size() - 1;
    const double theta_final = spectraAxis->getValue(index);
    m_pCalculation.setThetaFinal(theta_final);
    // Loop over all bins in spectra
    for (size_t binIndex = 0; binIndex < nInputBins; ++binIndex) {
      const double wavelength =
          0.5 * (wavelengths[binIndex] + wavelengths[binIndex + 1]);
      double _pSum = m_pCalculation.calculateDim0(wavelength);
      double _pDiff = m_pCalculation.calculateDim1(wavelength);

      if (_pSum >= m_d0Min && _pSum <= m_d0Max && _pDiff >= m_d1Min &&
          _pDiff <= m_d1Max) // Check that the calculated ki and kf are in range
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
} // namespace MDAlgorithms
