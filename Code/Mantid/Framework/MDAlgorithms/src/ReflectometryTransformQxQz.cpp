#include "MantidMDAlgorithms/ReflectometryTransformQxQz.h"

#include "MantidAPI/BinEdgeAxis.h"
#include "MantidAPI/SpectrumDetectorMapping.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataObjects/FractionalRebinning.h"
#include "MantidDataObjects/MDEventWorkspace.h"
#include "MantidDataObjects/RebinnedOutput.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidGeometry/MDGeometry/MDHistoDimension.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/V2D.h"
#include "MantidKernel/VectorHelper.h"
#include <stdexcept>
#include <boost/assign.hpp>

using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;

namespace Mantid {
namespace MDAlgorithms {

//----------------------------------------------------------------------------------------------
/** Destructor
 */
ReflectometryTransformQxQz::~ReflectometryTransformQxQz() {}

/*
 Constructor
 @param qxMin: min qx value (extent)
 @param qxMax: max qx value (extent)
 @param qzMin: min qz value (extent)
 @param qzMax; max qz value (extent)
 @param incidentTheta: Predetermined incident theta value
 @param numberOfBinsQx : Number of bins along the qx axis
 @param numberOfBinsQz : Number of bins along the qz axis
 */
ReflectometryTransformQxQz::ReflectometryTransformQxQz(
    double qxMin, double qxMax, double qzMin, double qzMax,
    double incidentTheta, int numberOfBinsQx, int numberOfBinsQz)
    : ReflectometryTransform("Qx", "qx", qxMin, qxMax, "Qz", "qz", qzMin, qzMax,
                             numberOfBinsQx, numberOfBinsQz,
                             new CalculateReflectometryQxQz()),
      m_inTheta(incidentTheta) {
  if (incidentTheta < 0 || incidentTheta > 90) {
    throw std::out_of_range("incident theta angle must be > 0 and < 90");
  }
}

MatrixWorkspace_sptr ReflectometryTransformQxQz::executeNormPoly(
    MatrixWorkspace_const_sptr inputWS) const {

  MatrixWorkspace_sptr temp = WorkspaceFactory::Instance().create(
      "RebinnedOutput", m_d1NumBins, m_d0NumBins, m_d0NumBins);
  RebinnedOutput_sptr outWS = boost::static_pointer_cast<RebinnedOutput>(temp);

  const double widthQx = (m_d0Max - m_d0Min) / double(m_d0NumBins);
  const double widthQz = (m_d1Max - m_d1Min) / double(m_d1NumBins);

  std::vector<double> xBinsVec;
  std::vector<double> zBinsVec;
  VectorHelper::createAxisFromRebinParams(
      boost::assign::list_of(m_d1Min)(widthQz)(m_d1Max), zBinsVec);
  VectorHelper::createAxisFromRebinParams(
      boost::assign::list_of(m_d0Min)(widthQx)(m_d0Max), xBinsVec);

  // Put the correct bin boundaries into the workspace
  outWS->replaceAxis(1, new BinEdgeAxis(zBinsVec));
  for (size_t i = 0; i < zBinsVec.size() - 1; ++i)
    outWS->setX(i, xBinsVec);

  // Prepare the required theta values
  initAngularCaches(inputWS);

  const size_t nHistos = inputWS->getNumberHistograms();
  const size_t nBins = inputWS->blocksize();

  // Holds the spectrum-detector mapping
  std::vector<specid_t> specNumberMapping;
  std::vector<detid_t> detIDMapping;

  CalculateReflectometryQxQz qcThetaLower;
  CalculateReflectometryQxQz qcThetaUpper;
  qcThetaLower.setThetaIncident(m_inTheta);
  qcThetaUpper.setThetaIncident(m_inTheta);

  for (size_t i = 0; i < nHistos; ++i) {
    IDetector_const_sptr detector = inputWS->getDetector(i);
    if (!detector || detector->isMasked() || detector->isMonitor()) {
      continue;
    }

    // Compute polygon points
    const double theta = m_theta[i];
    const double thetaWidth = m_thetaWidths[i];
    const double thetaHalfWidth = 0.5 * thetaWidth;
    const double thetaLower = theta - thetaHalfWidth;
    const double thetaUpper = theta + thetaHalfWidth;

    qcThetaLower.setThetaFinal(thetaLower);
    qcThetaUpper.setThetaFinal(thetaUpper);

    const MantidVec &X = inputWS->readX(0);

    for (size_t j = 0; j < nBins; ++j) {
      const double lamLower = X[j];
      const double lamUpper = X[j + 1];

      // fractional rebin
      const V2D ll(qcThetaLower.calculateDim0(lamLower),
                   qcThetaLower.calculateDim1(lamLower));
      const V2D lr(qcThetaLower.calculateDim0(lamUpper),
                   qcThetaLower.calculateDim1(lamUpper));
      const V2D ul(qcThetaUpper.calculateDim0(lamLower),
                   qcThetaUpper.calculateDim1(lamLower));
      const V2D ur(qcThetaUpper.calculateDim0(lamUpper),
                   qcThetaUpper.calculateDim1(lamUpper));

      Quadrilateral inputQ(ll, lr, ur, ul);
      FractionalRebinning::rebinToFractionalOutput(inputQ, inputWS, i, j, outWS,
                                                   zBinsVec);

      // Find which qy bin this point lies in
      const auto qIndex =
          std::upper_bound(zBinsVec.begin(), zBinsVec.end(), ll.Y()) -
          zBinsVec.begin();
      if (qIndex != 0 && qIndex < static_cast<int>(zBinsVec.size())) {
        // Add this spectra-detector pair to the mapping
        specNumberMapping.push_back(
            outWS->getSpectrum(qIndex - 1)->getSpectrumNo());
        detIDMapping.push_back(detector->getID());
      }
    }
  }
  outWS->finalize();
  FractionalRebinning::normaliseOutput(outWS, inputWS);

  // Set the output spectrum-detector mapping
  SpectrumDetectorMapping outputDetectorMap(specNumberMapping, detIDMapping);
  outWS->updateSpectraUsing(outputDetectorMap);

  return outWS;
}

} // namespace Mantid
} // namespace MDAlgorithms
