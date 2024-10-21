// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidCrystal/PeakIntensityVsRadius.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/TextAxis.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataObjects/PeaksWorkspace.h"

#include "MantidKernel/BoundedValidator.h"

#include "MantidKernel/Strings.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;

namespace Mantid::Crystal {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(PeakIntensityVsRadius)

//----------------------------------------------------------------------------------------------
/// Algorithm's name for identification. @see Algorithm::name
const std::string PeakIntensityVsRadius::name() const { return "PeakIntensityVsRadius"; }

/// Algorithm's version for identification. @see Algorithm::version
int PeakIntensityVsRadius::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string PeakIntensityVsRadius::category() const { return "Crystal\\Integration"; }

//----------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void PeakIntensityVsRadius::init() {
  declareProperty(std::make_unique<WorkspaceProperty<IMDEventWorkspace>>("InputWorkspace", "", Direction::Input),
                  "An input MDEventWorkspace containing the SCD data.");
  declareProperty(std::make_unique<WorkspaceProperty<PeaksWorkspace>>("PeaksWorkspace", "", Direction::Input),
                  "The list of peaks to integrate, matching the InputWorkspace.");

  declareProperty("RadiusStart", 0.0, "Radius at which to start integrating.");
  declareProperty("RadiusEnd", 1.0, "Radius at which to stop integrating.");
  auto mustBePositive = std::make_shared<BoundedValidator<int>>();
  mustBePositive->setLower(0);
  declareProperty("NumSteps", 10, mustBePositive, "Number of steps, between start and end, to calculate radius.");

  declareProperty("BackgroundInnerFactor", 0.0,
                  "For background subtraction: the peak radius will be multiplied\n"
                  "by this factor and passed to the BackgroundInnerRadius parameter.\n"
                  "Default 0.0 (no background).");
  declareProperty("BackgroundOuterFactor", 0.0,
                  "For background subtraction: the peak radius will be multiplied\n"
                  "by this factor and passed to the BackgroundOuterRadius parameter.\n"
                  "Default 0.0 (no background).");

  setPropertyGroup("BackgroundInnerFactor", "Variable Background Shell");
  setPropertyGroup("BackgroundOuterFactor", "Variable Background Shell");

  declareProperty("BackgroundInnerRadius", 0.0,
                  "For background subtraction:\n"
                  "Specify a fixed BackgroundInnerRadius, which does not "
                  "change with PeakRadius.\n"
                  "Default 0.0 (no background).");

  declareProperty("BackgroundOuterRadius", 0.0,
                  "For background subtraction:\n"
                  "Specify a fixed BackgroundOuterRadius, which does not "
                  "change with PeakRadius.\n"
                  "Default 0.0 (no background).");

  setPropertyGroup("BackgroundInnerRadius", "Fixed Background Shell");
  setPropertyGroup("BackgroundOuterRadius", "Fixed Background Shell");

  declareProperty(std::make_unique<WorkspaceProperty<>>("OutputWorkspace", "", Direction::Output),
                  "An output workspace2D containing intensity vs radius.");
  declareProperty(std::make_unique<WorkspaceProperty<>>("OutputWorkspace2", "NumberPeaksIntegrated", Direction::Output),
                  "An output workspace2D containing number of peaks at levels "
                  "of I/sigI vs radius.");
}

/** Check for reasonable values */
std::map<std::string, std::string> PeakIntensityVsRadius::validateInputs() {
  std::map<std::string, std::string> out;
  double BackgroundInnerFactor = getProperty("BackgroundInnerFactor");
  double BackgroundOuterFactor = getProperty("BackgroundOuterFactor");
  double BackgroundInnerRadius = getProperty("BackgroundInnerRadius");
  double BackgroundOuterRadius = getProperty("BackgroundOuterRadius");
  if ((BackgroundInnerRadius > 0) && (BackgroundInnerFactor > 0)) {
    out["BackgroundInnerRadius"] = "Do not specify both BackgroundInnerRadius and BackgroundInnerFactor.";
    out["BackgroundInnerFactor"] = "Do not specify both BackgroundInnerRadius and BackgroundInnerFactor.";
  }
  if ((BackgroundOuterRadius > 0) && (BackgroundOuterFactor > 0)) {
    out["BackgroundOuterRadius"] = "Do not specify both BackgroundOuterRadius and BackgroundOuterFactor.";
    out["BackgroundOuterFactor"] = "Do not specify both BackgroundOuterRadius and BackgroundOuterFactor.";
  }
  return out;
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void PeakIntensityVsRadius::exec() {
  IMDEventWorkspace_sptr inWS = getProperty("InputWorkspace");
  PeaksWorkspace_sptr peaksWS = getProperty("PeaksWorkspace");
  double RadiusStart = getProperty("RadiusStart");
  double RadiusEnd = getProperty("RadiusEnd");
  double BackgroundInnerFactor = getProperty("BackgroundInnerFactor");
  double BackgroundOuterFactor = getProperty("BackgroundOuterFactor");
  double BackgroundInnerRadius = getProperty("BackgroundInnerRadius");
  double BackgroundOuterRadius = getProperty("BackgroundOuterRadius");
  int NumSteps = getProperty("NumSteps");

  // Create a workspace with one spectrum per peak, and one point per radius
  // step
  MatrixWorkspace_sptr outWS =
      WorkspaceFactory::Instance().create("Workspace2D", peaksWS->getNumberPeaks(), NumSteps, NumSteps);

  // Create a text axis for axis(1), with H K L of each peak
  auto ax = std::make_unique<TextAxis>(outWS->getNumberHistograms());
  for (int i = 0; i < peaksWS->getNumberPeaks(); i++) {
    V3D hkl = peaksWS->getPeak(i).getHKL();
    hkl.round(); // Round HKL to make the string prettier
    ax->setLabel(size_t(i), hkl.toString());
  }
  outWS->replaceAxis(1, std::move(ax));

  MatrixWorkspace_sptr outWS2 = WorkspaceFactory::Instance().create("Workspace2D", 4, NumSteps, NumSteps);
  // Create a text axis for axis(1), with H K L of each peak
  auto ax2 = std::make_unique<TextAxis>(outWS2->getNumberHistograms());
  ax2->setLabel(0, "I/SigI=2");
  ax2->setLabel(1, "I/SigI=3");
  ax2->setLabel(2, "I/SigI=5");
  ax2->setLabel(3, "I/SigI=10");
  outWS2->replaceAxis(1, std::move(ax2));

  Progress prog(this, 0.0, 1.0, NumSteps);
  double progStep = 1.0 / double(NumSteps);
  for (int step = 0; step < NumSteps; step++) {
    // Step from RadiusStart to RadiusEnd, inclusively
    double radius = RadiusStart + double(step) * (RadiusEnd - RadiusStart) / (double(NumSteps - 1));
    g_log.debug() << "Integrating radius " << radius << '\n';
    prog.report("Radius " + Kernel::Strings::toString(radius));

    double OuterRadius = 0;
    if (BackgroundOuterRadius > 0)
      OuterRadius = BackgroundOuterRadius;
    if (BackgroundOuterFactor > 0)
      OuterRadius = BackgroundOuterFactor * radius;
    double InnerRadius = 0;
    if (BackgroundInnerRadius > 0)
      InnerRadius = BackgroundInnerRadius;
    if (BackgroundInnerFactor > 0)
      InnerRadius = BackgroundInnerFactor * radius;

    // Run the integrate algo with this background
    auto alg = createChildAlgorithm("IntegratePeaksMD", progStep * double(step), progStep * double(step + 1), false);
    alg->setProperty("InputWorkspace", inWS);
    alg->setProperty("PeaksWorkspace", peaksWS);
    alg->setProperty<std::vector<double>>("PeakRadius", {radius});
    alg->setProperty<std::vector<double>>("BackgroundOuterRadius", {OuterRadius});
    alg->setProperty<std::vector<double>>("BackgroundInnerRadius", {InnerRadius});
    alg->setPropertyValue("OutputWorkspace", "__tmp__PeakIntensityVsRadius");
    alg->execute();
    if (alg->isExecuted()) {
      size_t ISigI2 = 0;
      size_t ISigI3 = 0;
      size_t ISigI5 = 0;
      size_t ISigI10 = 0;
      for (int i = 0; i < 4; i++) {
        outWS2->mutableX(i)[step] = radius;
      }
      // Retrieve the integrated workspace
      IPeaksWorkspace_sptr outPeaks = alg->getProperty("OutputWorkspace");
      for (int i = 0; i < outPeaks->getNumberPeaks(); i++) {
        auto wi = size_t(i); // workspace index in output
        Geometry::IPeak &p = outPeaks->getPeak(i);
        outWS->mutableX(wi)[step] = radius;
        outWS->mutableY(wi)[step] = p.getIntensity();
        outWS->mutableE(wi)[step] = p.getSigmaIntensity();
        double ISigI = p.getIntensity() / p.getSigmaIntensity();
        if (ISigI > 10.0) {
          ISigI2++;
          ISigI3++;
          ISigI5++;
          ISigI10++;
        } else if (ISigI > 5.0) {
          ISigI2++;
          ISigI3++;
          ISigI5++;
        } else if (ISigI > 3.0) {
          ISigI2++;
          ISigI3++;
        } else if (ISigI > 2.0) {
          ISigI2++;
        }
      }
      outWS2->mutableY(0)[step] = static_cast<double>(ISigI2);
      outWS2->mutableY(1)[step] = static_cast<double>(ISigI3);
      outWS2->mutableY(2)[step] = static_cast<double>(ISigI5);
      outWS2->mutableY(3)[step] = static_cast<double>(ISigI10);
    } else {
      // TODO: Clear the point
    }
  }

  // Fix units and labels
  outWS->setYUnit("Integrated Intensity");
  outWS->getAxis(0)->title() = "Radius";
  outWS2->setYUnit("Number Peaks");
  outWS2->getAxis(0)->title() = "Radius";

  setProperty("OutputWorkspace", outWS);
  setProperty("OutputWorkspace2", outWS2);
}

} // namespace Mantid::Crystal
