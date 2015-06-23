#include "MantidMDAlgorithms/ReflectometryTransformQxQz.h"

#include "MantidAPI/BinEdgeAxis.h"
#include "MantidAPI/SpectrumDetectorMapping.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataObjects/FractionalRebinning.h"
#include "MantidDataObjects/MDEventWorkspace.h"
#include "MantidDataObjects/RebinnedOutput.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidGeometry/Instrument/DetectorGroup.h"
#include "MantidGeometry/Instrument/ReferenceFrame.h"
#include "MantidGeometry/Math/Quadrilateral.h"
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
    : ReflectometryTransform(numberOfBinsQx, numberOfBinsQz), m_qxMin(qxMin),
      m_qxMax(qxMax), m_qzMin(qzMin), m_qzMax(qzMax), m_inTheta(incidentTheta) {
  if (qxMin >= qxMax) {
    throw std::invalid_argument("min qx bounds must be < max qx bounds");
  }
  if (qzMin >= qzMax) {
    throw std::invalid_argument("min qz bounds must be < max qz bounds");
  }
  if (incidentTheta < 0 || incidentTheta > 90) {
    throw std::out_of_range("incident theta angle must be > 0 and < 90");
  }
}

/*
 Execute the transformation. Generates an output IMDEventWorkspace.
 @return the constructed IMDEventWorkspace following the transformation.
 @param ws: Input MatrixWorkspace, with a vertical axis of signed-theta, and an
 x-axis of wavelength
 @param boxController: Box controller to apply to output workspace
 */
IMDEventWorkspace_sptr
ReflectometryTransformQxQz::executeMD(MatrixWorkspace_const_sptr inputWs,
                                      BoxController_sptr boxController) const {

  MDHistoDimension_sptr qxDim = MDHistoDimension_sptr(new MDHistoDimension(
      "Qx", "qx", "(Ang^-1)", static_cast<Mantid::coord_t>(m_qxMin),
      static_cast<Mantid::coord_t>(m_qxMax), m_nbinsx));
  MDHistoDimension_sptr qzDim = MDHistoDimension_sptr(new MDHistoDimension(
      "Qz", "qz", "(Ang^-1)", static_cast<Mantid::coord_t>(m_qzMin),
      static_cast<Mantid::coord_t>(m_qzMax), m_nbinsz));

  auto ws = createMDWorkspace(qxDim, qzDim, boxController);

  CalculateReflectometryQxQz qCalc(m_inTheta);

  auto spectraAxis = inputWs->getAxis(1);
  for (size_t index = 0; index < inputWs->getNumberHistograms(); ++index) {
    auto counts = inputWs->readY(index);
    auto wavelengths = inputWs->readX(index);
    auto errors = inputWs->readE(index);
    const size_t nInputBins = wavelengths.size() - 1;
    const double theta_final = spectraAxis->getValue(index);
    qCalc.setThetaFinal(theta_final);
    // Loop over all bins in spectra
    for (size_t binIndex = 0; binIndex < nInputBins; ++binIndex) {
      const double wavelength =
          0.5 * (wavelengths[binIndex] + wavelengths[binIndex + 1]);
      double _qx = qCalc.calculateX(wavelength);
      double _qz = qCalc.calculateZ(wavelength);
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
 * Execute the transformation. Generates an output Matrix workspace.
 * @param inputWs : Input workspace with a vertical axis of signed-theta and an
 * x-axis of wavelength
 * @return : A 2D workspace with qz on the vertical axis and qx on the
 * horizontal axis.
 */
MatrixWorkspace_sptr
ReflectometryTransformQxQz::execute(MatrixWorkspace_const_sptr inputWs) const {
  auto ws = boost::make_shared<Mantid::DataObjects::Workspace2D>();

  ws->initialize(m_nbinsz, m_nbinsx,
                 m_nbinsx); // Create the output workspace as a distribution

  // Mapping so that qx and qz values calculated can be added to the matrix
  // workspace at the correct index.
  const double gradQx = double(m_nbinsx) / (m_qxMax - m_qxMin); // The x - axis
  const double gradQz =
      double(m_nbinsz) / (m_qzMax - m_qzMin); // Actually the y-axis
  const double cxToIndex = -gradQx * m_qxMin;
  const double czToIndex = -gradQz * m_qzMin;
  const double cxToQ = m_qxMin - (1 / gradQx);
  const double czToQ = m_qzMin - (1 / gradQz);

  // Create an X - Axis.
  MantidVec xAxisVec =
      createXAxis(ws.get(), gradQx, cxToQ, m_nbinsx, "qx", "1/Angstroms");
  // Create a Y (vertical) Axis
  createVerticalAxis(ws.get(), xAxisVec, gradQz, czToQ, m_nbinsz, "qz",
                     "1/Angstroms");

  CalculateReflectometryQxQz qCalc(m_inTheta);

  // Loop over all entries in the input workspace and calculate qx and qz for
  // each.
  auto spectraAxis = inputWs->getAxis(1);
  for (size_t index = 0; index < inputWs->getNumberHistograms(); ++index) {
    auto counts = inputWs->readY(index);
    auto wavelengths = inputWs->readX(index);
    auto errors = inputWs->readE(index);
    const size_t nInputBins = wavelengths.size() - 1;
    const double theta_final = spectraAxis->getValue(index);
    qCalc.setThetaFinal(theta_final);
    // Loop over all bins in spectra
    for (size_t binIndex = 0; binIndex < nInputBins; ++binIndex) {
      const double wavelength =
          0.5 * (wavelengths[binIndex] + wavelengths[binIndex + 1]);
      const double _qx = qCalc.calculateX(wavelength);
      const double _qz = qCalc.calculateZ(wavelength);

      if (_qx >= m_qxMin && _qx <= m_qxMax && _qz >= m_qzMin &&
          _qz <= m_qzMax) // Check that the calculated qx and qz are in range
      {
        const int outIndexX = int((gradQx * _qx) + cxToIndex);
        const int outIndexZ = int((gradQz * _qz) + czToIndex);

        ws->dataY(outIndexZ)[outIndexX] += counts[binIndex];
        ws->dataE(outIndexZ)[outIndexX] += errors[binIndex];
      }
    }
  }
  return ws;
}

MatrixWorkspace_sptr ReflectometryTransformQxQz::executeNormPoly(
    MatrixWorkspace_const_sptr inputWS) const {

  MatrixWorkspace_sptr temp = WorkspaceFactory::Instance().create(
      "RebinnedOutput", m_nbinsz, m_nbinsx, m_nbinsx);
  RebinnedOutput_sptr outWS = boost::static_pointer_cast<RebinnedOutput>(temp);

  const double widthQx = (m_qxMax - m_qxMin) / double(m_nbinsx);
  const double widthQz = (m_qzMax - m_qzMin) / double(m_nbinsz);

  std::vector<double> xBinsVec;
  std::vector<double> zBinsVec;
  VectorHelper::createAxisFromRebinParams(
      boost::assign::list_of(m_qzMin)(widthQz)(m_qzMax), zBinsVec);
  VectorHelper::createAxisFromRebinParams(
      boost::assign::list_of(m_qxMin)(widthQx)(m_qxMax), xBinsVec);

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

  CalculateReflectometryQxQz qcThetaLower(m_inTheta);
  CalculateReflectometryQxQz qcThetaUpper(m_inTheta);

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
      const V2D ll(qcThetaLower.calculateX(lamLower),
                   qcThetaLower.calculateZ(lamLower));
      const V2D lr(qcThetaLower.calculateX(lamUpper),
                   qcThetaLower.calculateZ(lamUpper));
      const V2D ul(qcThetaUpper.calculateX(lamLower),
                   qcThetaUpper.calculateZ(lamLower));
      const V2D ur(qcThetaUpper.calculateX(lamUpper),
                   qcThetaUpper.calculateZ(lamUpper));

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
