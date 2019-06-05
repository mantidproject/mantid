// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/ConvertAxesToRealSpace.h"
#include "MantidAPI/NumericAxis.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidGeometry/IDetector.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/Unit.h"
#include "MantidKernel/UnitFactory.h"

#include <limits>

namespace Mantid {
namespace Algorithms {

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(ConvertAxesToRealSpace)

/// Algorithm's name
const std::string ConvertAxesToRealSpace::name() const {
  return "ConvertAxesToRealSpace";
}

/// Algorithm's version for identification. @see Algorithm::version
int ConvertAxesToRealSpace::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string ConvertAxesToRealSpace::category() const {
  return "Transforms\\Units;Transforms\\Axes";
}

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string ConvertAxesToRealSpace::summary() const {
  return "Converts the spectrum and TOF axes to real space values, integrating "
         "the data in the process";
}

/** Initialize the algorithm's properties.
 */
void ConvertAxesToRealSpace::init() {
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>(
                      "InputWorkspace", "", Direction::Input),
                  "An input workspace.");
  declareProperty(std::make_unique<WorkspaceProperty<Workspace2D>>(
                      "OutputWorkspace", "", Direction::Output),
                  "An output workspace.");

  std::vector<std::string> propOptions;
  fillUnitMap(propOptions, m_unitMap, "x", "m");
  fillUnitMap(propOptions, m_unitMap, "y", "m");
  fillUnitMap(propOptions, m_unitMap, "z", "m");
  fillUnitMap(propOptions, m_unitMap, "r", "m");
  fillUnitMap(propOptions, m_unitMap, "theta", "deg");
  fillUnitMap(propOptions, m_unitMap, "phi", "deg");
  fillUnitMap(propOptions, m_unitMap, "2theta", "rad");
  fillUnitMap(propOptions, m_unitMap, "signed2theta", "rad");

  declareProperty("VerticalAxis", "y",
                  boost::make_shared<StringListValidator>(propOptions),
                  "What will be the vertical axis ?\n");
  declareProperty("HorizontalAxis", "2theta",
                  boost::make_shared<StringListValidator>(propOptions),
                  "What will be the horizontal axis?\n");

  declareProperty(std::make_unique<Kernel::PropertyWithValue<int>>(
                      "NumberVerticalBins", 100),
                  "The number of bins along the vertical axis.");
  declareProperty(std::make_unique<Kernel::PropertyWithValue<int>>(
                      "NumberHorizontalBins", 100),
                  "The number of bins along the horizontal axis.");
}

/** Execute the algorithm.
 */
void ConvertAxesToRealSpace::exec() {
  MatrixWorkspace_sptr inputWs = getProperty("InputWorkspace");

  // set up axes data
  std::vector<AxisData> axisVector = std::vector<AxisData>(2);
  std::string hAxis = getProperty("HorizontalAxis");
  axisVector[0].label = hAxis;
  axisVector[0].bins = getProperty("NumberHorizontalBins");
  std::string vAxis = getProperty("VerticalAxis");
  axisVector[1].label = vAxis;
  axisVector[1].bins = getProperty("NumberVerticalBins");
  for (int axisIndex = 0; axisIndex < 2; ++axisIndex) {
    axisVector[axisIndex].max = std::numeric_limits<double>::min();
    axisVector[axisIndex].min = std::numeric_limits<double>::max();
  }

  // Create the output workspace. Can't re-use the input one because we'll be
  // re-ordering the spectra.
  MatrixWorkspace_sptr outputWs = WorkspaceFactory::Instance().create(
      inputWs, axisVector[1].bins, axisVector[0].bins, axisVector[0].bins);

  // first integrate the data
  IAlgorithm_sptr alg = this->createChildAlgorithm("Integration", 0, 0.4);
  alg->setProperty<MatrixWorkspace_sptr>("InputWorkspace", inputWs);
  std::string outName = "_" + inputWs->getName() + "_integrated";
  alg->setProperty("OutputWorkspace", outName);
  alg->executeAsChildAlg();
  MatrixWorkspace_sptr summedWs = alg->getProperty("OutputWorkspace");

  int nHist = static_cast<int>(summedWs->getNumberHistograms());
  Progress progress(this, 0.4, 1.0, nHist * 4);

  std::vector<SpectraData> dataVector(nHist);

  int failedCount = 0;

  const auto &spectrumInfo = summedWs->spectrumInfo();
  // for each spectra
  PARALLEL_FOR_IF(Kernel::threadSafe(*summedWs, *outputWs))
  for (int i = 0; i < nHist; ++i) {
    try {
      V3D pos = spectrumInfo.position(i);
      double r, theta, phi;
      pos.getSpherical(r, theta, phi);

      // for each axis
      for (int axisIndex = 0; axisIndex < 2; ++axisIndex) {
        double axisValue = std::numeric_limits<double>::min();
        std::string axisSelection = axisVector[axisIndex].label;

        // get the selected value for this axis
        if (axisSelection == "x") {
          axisValue = pos.X();
        } else if (axisSelection == "y") {
          axisValue = pos.Y();
        } else if (axisSelection == "z") {
          axisValue = pos.Z();
        } else if (axisSelection == "r") {
          axisValue = r;
        } else if (axisSelection == "theta") {
          axisValue = theta;
        } else if (axisSelection == "phi") {
          axisValue = phi;
        } else if (axisSelection == "2theta") {
          axisValue = spectrumInfo.twoTheta(i);
        } else if (axisSelection == "signed2theta") {
          axisValue = spectrumInfo.signedTwoTheta(i);
        }

        if (axisIndex == 0) {
          dataVector[i].horizontalValue = axisValue;
        } else {
          dataVector[i].verticalValue = axisValue;
        }

        // record the max and min values
        if (axisValue > axisVector[axisIndex].max)
          axisVector[axisIndex].max = axisValue;
        if (axisValue < axisVector[axisIndex].min)
          axisVector[axisIndex].min = axisValue;
      }
    } catch (const Exception::NotFoundError &) {
      g_log.debug() << "Could not find detector for workspace index " << i
                    << '\n';
      failedCount++;
      // flag this is the datavector
      dataVector[i].horizontalValue = std::numeric_limits<double>::min();
      dataVector[i].verticalValue = std::numeric_limits<double>::min();
    }

    // take the values from the integrated data
    dataVector[i].intensity = summedWs->y(i)[0];
    dataVector[i].error = summedWs->e(i)[0];

    progress.report("Calculating new coords");
  }

  g_log.warning() << "Could not find detector for " << failedCount
                  << " spectra, see the debug log for more details.\n";

  // set up the axes on the output workspace
  std::vector<double> x_tmp(axisVector[0].bins);
  MantidVecPtr y;
  fillAxisValues(x_tmp, axisVector[0], false);
  HistogramData::Points x(std::move(x_tmp));

  outputWs->getAxis(0)->unit() = UnitFactory::Instance().create("Label");
  Unit_sptr xUnit = outputWs->getAxis(0)->unit();
  boost::shared_ptr<Units::Label> xlabel =
      boost::dynamic_pointer_cast<Units::Label>(xUnit);
  xlabel->setLabel(axisVector[0].label, m_unitMap[axisVector[0].label]);

  MantidVec &yRef = y.access();
  yRef.resize(axisVector[1].bins);
  fillAxisValues(yRef, axisVector[1], false);

  auto const yAxis = new NumericAxis(yRef);
  boost::shared_ptr<Units::Label> ylabel =
      boost::dynamic_pointer_cast<Units::Label>(
          UnitFactory::Instance().create("Label"));
  ylabel->setLabel(axisVector[1].label, m_unitMap[axisVector[1].label]);
  yAxis->unit() = ylabel;
  outputWs->replaceAxis(1, yAxis);

  // work out where to put the data into the output workspace, but don't do it
  // yet as that needs to be single threaded
  PARALLEL_FOR_NO_WSP_CHECK()
  for (int i = 0; i < nHist; ++i) {
    // find write index for data point
    if (dataVector[i].horizontalValue == std::numeric_limits<double>::min()) {
      dataVector[i].horizontalIndex = -1;
      dataVector[i].verticalIndex = -1;
    } else {
      int xIndex = static_cast<int>(std::distance(
          x.cbegin(), std::lower_bound(x.cbegin(), x.cend(),
                                       dataVector[i].horizontalValue)));
      if (xIndex > 0)
        --xIndex;
      int yIndex = static_cast<int>(std::distance(
          y->begin(),
          std::lower_bound(y->begin(), y->end(), dataVector[i].verticalValue)));
      if (yIndex > 0)
        --yIndex;

      dataVector[i].horizontalIndex = xIndex;
      dataVector[i].verticalIndex = yIndex;
    }
    progress.report("Calculating Rebinning");
  }

  // set all the X arrays - share the same vector
  int nOutputHist = static_cast<int>(outputWs->getNumberHistograms());
  PARALLEL_FOR_IF(Kernel::threadSafe(*outputWs))
  for (int i = 0; i < nOutputHist; ++i) {
    outputWs->setPoints(i, x);
  }

  // insert the data into the new workspace
  // single threaded
  for (int i = 0; i < nHist; ++i) {
    int xIndex = dataVector[i].horizontalIndex;
    int yIndex = dataVector[i].verticalIndex;

    // using -1 as a flag for could not find detector
    if ((xIndex == -1) || (yIndex == -1)) {
      // do nothing the detector could not be found
      g_log.warning() << "here " << i << '\n';
    } else {
      // update the data
      auto &yVec = outputWs->mutableY(yIndex);
      yVec[xIndex] = yVec[xIndex] + dataVector[i].intensity;
      auto &eVec = outputWs->mutableE(yIndex);
      eVec[xIndex] = eVec[xIndex] + (dataVector[i].error * dataVector[i].error);
    }

    progress.report("Assigning to new grid");
  }

  // loop over the data and sqrt the errors to complete the error calculation
  PARALLEL_FOR_IF(Kernel::threadSafe(*outputWs))
  for (int i = 0; i < nOutputHist; ++i) {
    auto &errorVec = outputWs->mutableE(i);
    std::transform(errorVec.begin(), errorVec.end(), errorVec.begin(),
                   static_cast<double (*)(double)>(sqrt));
    progress.report("Completing Error Calculation");
  }

  // Execute the transform and bind to the output.
  setProperty("OutputWorkspace", outputWs);
}

/** Fills the values in an axis linearly from min to max for a given number of
 * steps
 * @param vector the vector to fill
 * @param axisData the data about the axis
 * @param isHistogram true if the data should be a histogram rather than point
 * data
 */
void ConvertAxesToRealSpace::fillAxisValues(MantidVec &vector,
                                            const AxisData &axisData,
                                            bool isHistogram) {
  int numBins = axisData.bins;
  double binDelta =
      (axisData.max - axisData.min) / static_cast<double>(numBins);

  if (isHistogram)
    numBins++;

  for (int i = 0; i < numBins; ++i) {
    vector[i] = axisData.min + i * binDelta;
  }
}

/** Fills the unit map and ordered vector with the same data
 * @param orderedVector the vector to fill
 * @param unitMap the map to fill
 * @param caption the caption of the unit
 * @param unit the unit of measure of the unit
 */
void ConvertAxesToRealSpace::fillUnitMap(
    std::vector<std::string> &orderedVector,
    std::map<std::string, std::string> &unitMap, const std::string &caption,
    const std::string &unit) {
  unitMap.emplace(caption, unit);
  orderedVector.push_back(caption);
}

} // namespace Algorithms
} // namespace Mantid
