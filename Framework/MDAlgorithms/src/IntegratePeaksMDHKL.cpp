// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidMDAlgorithms/IntegratePeaksMDHKL.h"
#include "MantidAPI/CommonBinsValidator.h"
#include "MantidAPI/InstrumentValidator.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/MDEventWorkspace.h"
#include "MantidDataObjects/MDFramesToSpecialCoordinateSystem.h"
#include "MantidDataObjects/MDHistoWorkspace.h"
#include "MantidGeometry/Crystal/IPeak.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/CompositeValidator.h"
#include <algorithm>
#include <boost/math/special_functions/round.hpp>
#include <limits>

namespace Mantid::MDAlgorithms {

using Mantid::Kernel::Direction;
// using Mantid::API::WorkspaceProperty;
using namespace Mantid::DataObjects;
using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::Geometry;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(IntegratePeaksMDHKL)

//----------------------------------------------------------------------------------------------
/**
 * Initialize the algorithm's properties.
 */
void IntegratePeaksMDHKL::init() {
  declareProperty(std::make_unique<WorkspaceProperty<IMDWorkspace>>("InputWorkspace", "", Direction::Input),
                  "An input Sample MDHistoWorkspace or MDEventWorkspace in HKL.");
  declareProperty("DeltaHKL", 0.5, "Distance from integer HKL to integrate peak.");
  declareProperty("GridPoints", 201, "Number of grid points for each dimension of HKL box.");
  declareProperty("NeighborPoints", 10,
                  "Number of points in 5^3 surrounding "
                  "points above intensity threshold for "
                  "point to be part of peak.");
  auto fluxValidator = std::make_shared<CompositeValidator>();
  fluxValidator->add<WorkspaceUnitValidator>("Momentum");
  fluxValidator->add<InstrumentValidator>();
  fluxValidator->add<CommonBinsValidator>();
  auto solidAngleValidator = fluxValidator->clone();

  declareProperty(std::make_unique<WorkspaceProperty<>>("FluxWorkspace", "", Direction::Input, PropertyMode::Optional,
                                                        fluxValidator),
                  "An optional input workspace containing momentum dependent flux for "
                  "normalization.");
  declareProperty(std::make_unique<WorkspaceProperty<>>("SolidAngleWorkspace", "", Direction::Input,
                                                        PropertyMode::Optional, solidAngleValidator),
                  "An optional input workspace containing momentum integrated "
                  "vanadium for normalization "
                  "(a measure of the solid angle).");

  declareProperty(std::make_unique<WorkspaceProperty<PeaksWorkspace>>("PeaksWorkspace", "", Direction::Input),
                  "A PeaksWorkspace containing the peaks to integrate.");

  declareProperty(std::make_unique<WorkspaceProperty<PeaksWorkspace>>("OutputWorkspace", "", Direction::Output),
                  "The output PeaksWorkspace will be a copy of the input PeaksWorkspace "
                  "with the peaks' integrated intensities.");
  declareProperty(std::make_unique<PropertyWithValue<double>>("BackgroundInnerRadius", EMPTY_DBL(), Direction::Input),
                  "Optional:Inner radius to use to evaluate the background of the peak.\n"
                  "If omitted background is region of HKL box - peak. ");

  declareProperty(std::make_unique<PropertyWithValue<double>>("BackgroundOuterRadius", EMPTY_DBL(), Direction::Input),
                  "Optional:Outer radius to use to evaluate the background of the peak.\n"
                  "The signal density around the peak (BackgroundInnerRadius < r < "
                  "BackgroundOuterRadius) is used to estimate the background under the "
                  "peak.\n"
                  "If omitted background is region of HKL box - peak.");
}

//----------------------------------------------------------------------------------------------
/**
 * Execute the algorithm.
 */
void IntegratePeaksMDHKL::exec() {
  IMDWorkspace_sptr m_inputWS = getProperty("InputWorkspace");
  Mantid::DataObjects::MDFramesToSpecialCoordinateSystem converter;
  std::optional<Mantid::Kernel::SpecialCoordinateSystem> coordinateSystem = converter(m_inputWS.get());
  if (*coordinateSystem != Mantid::Kernel::SpecialCoordinateSystem::HKL) {
    std::stringstream errmsg;
    errmsg << "Input MDWorkspace's coordinate system is not HKL.";
    throw std::invalid_argument(errmsg.str());
  }

  /// Peak workspace to integrate
  PeaksWorkspace_sptr inPeakWS = getProperty("PeaksWorkspace");
  const double box = getProperty("DeltaHKL");
  const int gridPts = getProperty("GridPoints");
  const int neighborPts = getProperty("NeighborPoints");
  /// Output peaks workspace, create if needed
  PeaksWorkspace_sptr peakWS = getProperty("OutputWorkspace");
  if (peakWS != inPeakWS)
    peakWS = inPeakWS->clone();

  MatrixWorkspace_sptr flux = getProperty("FluxWorkspace");
  MatrixWorkspace_sptr sa = getProperty("SolidAngleWorkspace");

  IMDEventWorkspace_sptr m_eventWS = std::dynamic_pointer_cast<IMDEventWorkspace>(m_inputWS);
  IMDHistoWorkspace_sptr m_histoWS = std::dynamic_pointer_cast<IMDHistoWorkspace>(m_inputWS);
  int npeaks = peakWS->getNumberPeaks();

  auto prog = std::make_unique<Progress>(this, 0.3, 1.0, npeaks);
  PARALLEL_FOR_IF(Kernel::threadSafe(*peakWS))
  for (int i = 0; i < npeaks; i++) {
    PARALLEL_START_INTERRUPT_REGION

    IPeak &p = peakWS->getPeak(i);
    // round to integer
    int h = static_cast<int>(boost::math::iround(p.getH()));
    int k = static_cast<int>(boost::math::iround(p.getK()));
    int l = static_cast<int>(boost::math::iround(p.getL()));
    MDHistoWorkspace_sptr histoBox;
    if (m_histoWS) {
      histoBox = cropHisto(h, k, l, box, m_histoWS);
    } else if (sa && flux) {
      histoBox = normalize(h, k, l, box, gridPts, flux, sa, m_eventWS);
    } else {
      histoBox = binEvent(h, k, l, box, gridPts, m_eventWS);
    }
    double intensity = 0.0;
    double errorSquared = 0.0;
    integratePeak(neighborPts, histoBox, intensity, errorSquared);
    p.setIntensity(intensity);
    p.setSigmaIntensity(sqrt(errorSquared));
    prog->report();
    PARALLEL_END_INTERRUPT_REGION
  }
  PARALLEL_CHECK_INTERRUPT_REGION
  // Save the output
  setProperty("OutputWorkspace", peakWS);
}

MDHistoWorkspace_sptr IntegratePeaksMDHKL::normalize(int h, int k, int l, double box, int gridPts,
                                                     const MatrixWorkspace_sptr &flux, const MatrixWorkspace_sptr &sa,
                                                     const IMDEventWorkspace_sptr &ws) {
  auto normAlg = createChildAlgorithm("MDNormSCD");
  normAlg->setProperty("InputWorkspace", ws);
  normAlg->setProperty("AlignedDim0", "[H,0,0]," + boost::lexical_cast<std::string>(h - box) + "," +
                                          boost::lexical_cast<std::string>(h + box) + "," + std::to_string(gridPts));
  normAlg->setProperty("AlignedDim1", "[0,K,0]," + boost::lexical_cast<std::string>(k - box) + "," +
                                          boost::lexical_cast<std::string>(k + box) + "," + std::to_string(gridPts));
  normAlg->setProperty("AlignedDim2", "[0,0,L]," + boost::lexical_cast<std::string>(l - box) + "," +
                                          boost::lexical_cast<std::string>(l + box) + "," + std::to_string(gridPts));
  normAlg->setProperty("FluxWorkspace", flux);
  normAlg->setProperty("SolidAngleWorkspace", sa);
  normAlg->setProperty("OutputWorkspace", "mdout");
  normAlg->setProperty("OutputNormalizationWorkspace", "mdnorm");
  normAlg->executeAsChildAlg();
  Workspace_sptr mdout = normAlg->getProperty("OutputWorkspace");
  Workspace_sptr mdnorm = normAlg->getProperty("OutputNormalizationWorkspace");

  auto alg = createChildAlgorithm("DivideMD");
  alg->setProperty("LHSWorkspace", mdout);
  alg->setProperty("RHSWorkspace", mdnorm);
  alg->setPropertyValue("OutputWorkspace", "out");
  alg->execute();
  IMDWorkspace_sptr out = alg->getProperty("OutputWorkspace");
  return std::dynamic_pointer_cast<MDHistoWorkspace>(out);
}

void IntegratePeaksMDHKL::integratePeak(const int neighborPts, const MDHistoWorkspace_sptr &out, double &intensity,
                                        double &errorSquared) {
  std::vector<int> gridPts;
  /// Background (end) radius
  double BackgroundOuterRadius2 = getProperty("BackgroundOuterRadius");
  if (BackgroundOuterRadius2 != EMPTY_DBL())
    BackgroundOuterRadius2 = pow(BackgroundOuterRadius2, 2.0);
  /// Start radius of the background
  double BackgroundInnerRadius2 = getProperty("BackgroundInnerRadius");
  if (BackgroundInnerRadius2 != EMPTY_DBL())
    BackgroundInnerRadius2 = pow(BackgroundInnerRadius2, 2.0);
  const size_t dimensionality = out->getNumDims();
  for (size_t i = 0; i < dimensionality; ++i) {
    gridPts.emplace_back(static_cast<int>(out->getDimension(i)->getNBins()));
  }

  const auto F = out->getSignalArray();
  double Fmax = 0.;
  double Fmin = std::numeric_limits<double>::max();
  for (int i = 0; i < gridPts[0] * gridPts[1] * gridPts[2]; i++) {
    if (std::isnormal(F[i])) {
      Fmin = std::min(Fmin, F[i]);
      Fmax = std::max(Fmax, F[i]);
    }
  }
  const auto SqError = out->getErrorSquaredArray();

  double minIntensity = Fmin + 0.01 * (Fmax - Fmin);
  int measuredPoints = 0;
  int peakPoints = 0;
  double peakSum = 0.0;
  double measuredSum = 0.0;
  double errSqSum = 0.0;
  double measuredErrSqSum = 0.0;
  int backgroundPoints = 0;
  double backgroundSum = 0.0;
  double backgroundErrSqSum = 0.0;
  double Hcenter = gridPts[0] * 0.5;
  double Kcenter = gridPts[1] * 0.5;
  double Lcenter = gridPts[2] * 0.5;

  for (int Hindex = 0; Hindex < gridPts[0]; Hindex++) {
    for (int Kindex = 0; Kindex < gridPts[1]; Kindex++) {
      for (int Lindex = 0; Lindex < gridPts[2]; Lindex++) {
        int iHKL = Hindex + gridPts[0] * (Kindex + gridPts[1] * Lindex);
        if (BackgroundOuterRadius2 != EMPTY_DBL()) {
          double radius2 = pow((double(Hindex) - Hcenter) / gridPts[0], 2) +
                           pow((double(Kindex) - Kcenter) / gridPts[1], 2) +
                           pow((double(Lindex) - Lcenter) / gridPts[2], 2);
          if (radius2 < BackgroundOuterRadius2 && BackgroundInnerRadius2 < radius2) {
            backgroundPoints = backgroundPoints + 1;
            backgroundSum = backgroundSum + F[iHKL];
            backgroundErrSqSum = backgroundErrSqSum + SqError[iHKL];
          }
        }
        if (std::isfinite(F[iHKL])) {
          measuredPoints = measuredPoints + 1;
          measuredSum = measuredSum + F[iHKL];
          measuredErrSqSum = measuredErrSqSum + SqError[iHKL];
          if (F[iHKL] > minIntensity) {
            int neighborPoints = 0;
            for (int Hj = -2; Hj < 3; Hj++) {
              for (int Kj = -2; Kj < 3; Kj++) {
                for (int Lj = -2; Lj < 3; Lj++) {
                  int jHKL = Hindex + Hj + gridPts[0] * (Kindex + Kj + gridPts[1] * (Lindex + Lj));
                  if (Lindex + Lj >= 0 && Lindex + Lj < gridPts[2] && Kindex + Kj >= 0 && Kindex + Kj < gridPts[1] &&
                      Hindex + Hj >= 0 && Hindex + Hj < gridPts[0] && F[jHKL] > minIntensity) {
                    neighborPoints = neighborPoints + 1;
                  }
                }
              }
            }
            if (neighborPoints >= neighborPts) {
              peakPoints = peakPoints + 1;
              peakSum = peakSum + F[iHKL];
              errSqSum = errSqSum + SqError[iHKL];
            }
          }
        } else {
          double minR = sqrt(std::pow(float(Hindex) / float(gridPts[0]) - 0.5, 2) +
                             std::pow(float(Kindex) / float(gridPts[1]) - 0.5, 2) +
                             std::pow(float(Lindex) / float(gridPts[0]) - 0.5, 2));
          if (minR < 0.05) {
            intensity = 0.0;
            errorSquared = 0.0;
            return;
          }
        }
      }
    }
  }
  if (BackgroundOuterRadius2 != EMPTY_DBL()) {
    double ratio = 0.0;
    if (backgroundPoints > 0) {
      ratio = float(peakPoints) / float(backgroundPoints);
    }
    intensity = peakSum - ratio * (backgroundSum);
    errorSquared = errSqSum + ratio * ratio * (backgroundErrSqSum);
  } else {
    double ratio = float(peakPoints) / float(measuredPoints - peakPoints);
    intensity = peakSum - ratio * (measuredSum - peakSum);
    errorSquared = errSqSum + ratio * ratio * (measuredErrSqSum - errSqSum);
  }
  return;
}

/**
 * Runs the BinMD algorithm on the input to provide the output workspace
 * All slicing algorithm properties are passed along
 * @return MDHistoWorkspace as a result of the binning
 */
MDHistoWorkspace_sptr IntegratePeaksMDHKL::binEvent(int h, int k, int l, double box, int gridPts,
                                                    const IMDWorkspace_sptr &ws) {
  auto binMD = createChildAlgorithm("BinMD", 0.0, 0.3);
  binMD->setProperty("InputWorkspace", ws);
  binMD->setProperty("AlignedDim0", "[H,0,0]," + boost::lexical_cast<std::string>(h - box) + "," +
                                        boost::lexical_cast<std::string>(h + box) + "," + std::to_string(gridPts));
  binMD->setProperty("AlignedDim1", "[0,K,0]," + boost::lexical_cast<std::string>(k - box) + "," +
                                        boost::lexical_cast<std::string>(k + box) + "," + std::to_string(gridPts));
  binMD->setProperty("AlignedDim2", "[0,0,L]," + boost::lexical_cast<std::string>(l - box) + "," +
                                        boost::lexical_cast<std::string>(l + box) + "," + std::to_string(gridPts));
  binMD->setPropertyValue("AxisAligned", "1");
  binMD->setPropertyValue("OutputWorkspace", "out");
  binMD->executeAsChildAlg();
  Workspace_sptr outputWS = binMD->getProperty("OutputWorkspace");
  return std::dynamic_pointer_cast<MDHistoWorkspace>(outputWS);
}

/**
 * Runs the BinMD algorithm on the input to provide the output workspace
 * All slicing algorithm properties are passed along
 * @return MDHistoWorkspace as a result of the binning
 */
MDHistoWorkspace_sptr IntegratePeaksMDHKL::cropHisto(int h, int k, int l, double box, const IMDWorkspace_sptr &ws) {
  auto cropMD = createChildAlgorithm("IntegrateMDHistoWorkspace", 0.0, 0.3);
  cropMD->setProperty("InputWorkspace", ws);

  cropMD->setProperty("P1Bin",
                      boost::lexical_cast<std::string>(h - box) + ",0," + boost::lexical_cast<std::string>(h + box));
  cropMD->setProperty("P2Bin",
                      boost::lexical_cast<std::string>(k - box) + ",0," + boost::lexical_cast<std::string>(k + box));
  cropMD->setProperty("P3Bin",
                      boost::lexical_cast<std::string>(l - box) + ",0," + boost::lexical_cast<std::string>(l + box));

  cropMD->setPropertyValue("OutputWorkspace", "out");
  cropMD->executeAsChildAlg();
  IMDHistoWorkspace_sptr outputWS = cropMD->getProperty("OutputWorkspace");
  return std::dynamic_pointer_cast<MDHistoWorkspace>(outputWS);
}

} // namespace Mantid::MDAlgorithms
