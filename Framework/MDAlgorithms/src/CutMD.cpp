// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidMDAlgorithms/CutMD.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/IMDHistoWorkspace.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/Projection.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/Sample.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/Matrix.h"

#include <boost/algorithm/string.hpp>
#include <boost/regex.hpp>
#include <memory>

using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;

namespace {

// Typedef to simplify function signatures
using MinMax = std::pair<double, double>;

MinMax getDimensionExtents(const IMDEventWorkspace_sptr &ws, size_t index) {
  if (!ws)
    throw std::runtime_error("Invalid workspace passed to getDimensionExtents.");
  auto dim = ws->getDimension(index);
  return std::make_pair(dim->getMinimum(), dim->getMaximum());
}

std::string numToStringWithPrecision(const double num) {
  std::stringstream s;
  s.precision(2);
  s.setf(std::ios::fixed, std::ios::floatfield);
  s << num;
  return s.str();
}

DblMatrix scaleProjection(const DblMatrix &inMatrix, const std::vector<std::string> &inUnits,
                          std::vector<std::string> &outUnits, const IMDEventWorkspace_sptr &inWS) {
  DblMatrix ret(inMatrix);
  // Check if we actually need to do anything
  if (std::equal(inUnits.begin(), inUnits.end(), outUnits.begin()))
    return ret;

  if (inUnits.size() != outUnits.size())
    throw std::runtime_error("scaleProjection given different quantity of input and output units");

  assert(inWS->getNumExperimentInfo() > 0);
  const OrientedLattice &orientedLattice = inWS->getExperimentInfo(0)->sample().getOrientedLattice();

  const size_t numDims = inUnits.size();
  for (size_t i = 0; i < numDims; ++i) {
    const double dStar = 2 * M_PI * orientedLattice.dstar(inMatrix[i][0], inMatrix[i][1], inMatrix[i][2]);
    if (inUnits[i] == outUnits[i])
      continue;
    else if (inUnits[i] == Mantid::MDAlgorithms::CutMD::InvAngstromSymbol) {
      // inv angstroms to rlu
      outUnits[i] = "in " + numToStringWithPrecision(dStar) + " A^-1";
      for (size_t j = 0; j < numDims; ++j)
        ret[i][j] *= dStar;
    } else {
      // rlu to inv angstroms
      for (size_t j = 0; j < numDims; ++j)
        ret[i][j] /= dStar;
    }
  }

  return ret;
}

std::vector<MinMax> calculateExtents(const DblMatrix &inMatrix, const std::vector<MinMax> &limits) {
  DblMatrix invMat(inMatrix);
  invMat.Invert();

  // iterate through min/max of each dimension, calculate dot(vert, inv_mat)
  // and store min/max value for each dimension
  std::vector<double> hRange(2), kRange(2), lRange(2);

  hRange[0] = limits[0].first;
  hRange[1] = limits[0].second;
  kRange[0] = limits[1].first;
  kRange[1] = limits[1].second;
  lRange[0] = limits[2].first;
  lRange[1] = limits[2].second;

  // Calculate the minimums and maximums of transformed coordinates
  // Use maxDbl as a "not-yet-set" placeholder
  const double maxDbl = std::numeric_limits<double>::max();
  std::vector<MinMax> extents(3, std::make_pair(maxDbl, maxDbl));

  for (double &hIt : hRange) {
    for (double &kIt : kRange) {
      for (double &lIt : lRange) {
        V3D origPos(hIt, kIt, lIt);
        for (size_t i = 0; i < 3; ++i) {
          const V3D other(invMat[i][0], invMat[i][1], invMat[i][2]);
          double val = origPos.scalar_prod(other);
          // Check if min needs updating
          if (extents[i].first == maxDbl || extents[i].first > val)
            extents[i].first = val;
          // Check if max needs updating
          if (extents[i].second == maxDbl || extents[i].second < val)
            extents[i].second = val;
        }
      }
    }
  }

  return extents;
}

std::pair<std::vector<MinMax>, std::vector<int>> calculateSteps(const std::vector<MinMax> &inExtents,
                                                                const std::vector<std::vector<double>> &binning) {
  std::vector<MinMax> outExtents(inExtents);
  std::vector<int> outBins;

  for (size_t i = 0; i < inExtents.size(); ++i) {
    const size_t nArgs = binning[i].size();
    int outBin = -1;

    if (nArgs == 0) {
      throw std::runtime_error("Binning parameter cannot be empty");

    } else if (nArgs == 1) {
      const double dimRange = inExtents[i].second - inExtents[i].first;
      const double stepSize = binning[i][0] < dimRange ? binning[i][0] : dimRange;
      outBin = static_cast<int>(dimRange / stepSize);
      outExtents[i].second = inExtents[i].first + outBin * stepSize;

    } else if (nArgs == 2) {
      outExtents[i].first = binning[i][0];
      outExtents[i].second = binning[i][1];
      outBin = 1;

    } else if (nArgs == 3) {
      const double dimMin = binning[i][0];
      const double dimMax = binning[i][2];
      const double dimRange = dimMax - dimMin;
      const double stepSize = binning[i][1] < dimRange ? binning[i][1] : dimRange;
      outBin = static_cast<int>(dimRange / stepSize);
      outExtents[i].second = dimMin + outBin * stepSize;
      outExtents[i].first = dimMin;

    } else {
      throw std::runtime_error("Cannot handle " + std::to_string(nArgs) + " bins.");
    }
    if (outBin < 0)
      throw std::runtime_error("output bin calculated to be less than 0");
    outBins.emplace_back(outBin);
  }
  return std::make_pair(outExtents, outBins);
}

std::vector<std::string> labelProjection(const DblMatrix &projection) {
  const std::string replacements[] = {"zeta", "eta", "xi"};
  std::vector<std::string> ret(3);
  std::vector<std::string> labels(3);

  for (size_t i = 0; i < 3; ++i) {
    for (size_t j = 0; j < 3; ++j) {
      const double in = projection[i][j];
      if (std::abs(in) == 1)
        if (in > 0)
          labels[j] = "'" + replacements[i] + "'";
        else
          labels[j] = "'-" + replacements[i] + "'";
      else if (in == 0)
        labels[j] = "0";
      else {
        labels[j] = "'" + numToStringWithPrecision(in) + replacements[i] + "'";
      }
    }
    ret[i] = "[" + boost::algorithm::join(labels, ", ") + "]";
  }
  return ret;
}

} // anonymous namespace

namespace Mantid::MDAlgorithms {

/**
Determine the original q units. Assumes first 3 dimensions by index are r,l,d
@param inws : Input workspace to extract dimension info from
@param logger : logging object
@return vector of markers
*/
std::vector<std::string> findOriginalQUnits(const IMDWorkspace_const_sptr &inws, Mantid::Kernel::Logger &logger) {
  std::vector<std::string> unitMarkers(3);
  for (size_t i = 0; i < inws->getNumDims() && i < 3; ++i) {
    auto units = inws->getDimension(i)->getUnits();
    const boost::regex re("(Angstrom\\^-1)|(A\\^-1)", boost::regex::icase);
    // Does the unit label look like it's in Angstroms?
    std::string unitMarker;
    if (boost::regex_match(units.ascii(), re)) {
      unitMarker = Mantid::MDAlgorithms::CutMD::InvAngstromSymbol;
    } else {
      unitMarker = Mantid::MDAlgorithms::CutMD::RLUSymbol;
    }
    unitMarkers[i] = unitMarker;
    logger.debug() << "In dimension with index " << i << " and units " << units.ascii() << " taken to be of type "
                   << unitMarker << '\n';
  }
  return unitMarkers;
}

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(CutMD)

const std::string CutMD::InvAngstromSymbol = "a";
const std::string CutMD::RLUSymbol = "r";
const std::string CutMD::AutoMethod = "Auto";
const std::string CutMD::RLUMethod = "RLU";
const std::string CutMD::InvAngstromMethod = "Q in A^-1";

//----------------------------------------------------------------------------------------------

void CutMD::init() {
  declareProperty(std::make_unique<WorkspaceProperty<IMDWorkspace>>("InputWorkspace", "", Direction::Input),
                  "MDWorkspace to slice");

  declareProperty(
      std::make_unique<WorkspaceProperty<ITableWorkspace>>("Projection", "", Direction::Input, PropertyMode::Optional),
      "Projection");

  declareProperty(std::make_unique<ArrayProperty<double>>("P1Bin"), "Projection 1 binning.");
  declareProperty(std::make_unique<ArrayProperty<double>>("P2Bin"), "Projection 2 binning.");
  declareProperty(std::make_unique<ArrayProperty<double>>("P3Bin"), "Projection 3 binning.");
  declareProperty(std::make_unique<ArrayProperty<double>>("P4Bin"), "Projection 4 binning.");
  declareProperty(std::make_unique<ArrayProperty<double>>("P5Bin"), "Projection 5 binning.");

  declareProperty(std::make_unique<WorkspaceProperty<IMDWorkspace>>("OutputWorkspace", "", Direction::Output),
                  "Output cut workspace");
  declareProperty("NoPix", false,
                  "If False creates a full MDEventWorkspaces "
                  "as output. True to create an "
                  "MDHistoWorkspace as output. This is DND "
                  "only in Horace terminology.");

  auto mustBePositiveInteger = std::make_shared<BoundedValidator<int>>();
  mustBePositiveInteger->setLower(0);

  declareProperty("MaxRecursionDepth", 1, mustBePositiveInteger,
                  "Sets the maximum recursion depth to use. Can be used to "
                  "constrain the workspaces internal structure");

  std::vector<std::string> propOptions{AutoMethod, RLUMethod, InvAngstromMethod};
  char buffer[1024];
  std::sprintf(buffer,
               "How will the Q units of the input workspace be interpreted? "
               "This property will disappear in future versions of Mantid\n"
               "%s : Figure it out based on the label units\n"
               "%s : Force them to be rlu\n"
               "%s : Force them to be inverse angstroms",
               AutoMethod.c_str(), RLUMethod.c_str(), InvAngstromMethod.c_str());

  std::string help(buffer);
  boost::algorithm::trim(help);
  declareProperty("InterpretQDimensionUnits", AutoMethod, std::make_shared<StringListValidator>(propOptions), help);
}

void CutMD::exec() {
  g_log.warning("CutMD is in the beta stage of development. Its properties and "
                "behaviour may change without warning.");

  // Collect input properties
  const IMDWorkspace_sptr inWS = getProperty("InputWorkspace");
  const size_t numDims = inWS->getNumDims();
  const ITableWorkspace_sptr projectionWS = getProperty("Projection");
  std::vector<std::vector<double>> pbins{getProperty("P1Bin"), getProperty("P2Bin"), getProperty("P3Bin"),
                                         getProperty("P4Bin"), getProperty("P5Bin")};

  Workspace_sptr sliceWS; // output workspace

  // Histogram workspaces can be sliced axis-aligned only.
  if (auto histInWS = std::dynamic_pointer_cast<IMDHistoWorkspace>(inWS)) {

    g_log.information("Integrating using binning parameters only.");
    auto integrateAlg = this->createChildAlgorithm("IntegrateMDHistoWorkspace", 0, 1);
    integrateAlg->setProperty("InputWorkspace", histInWS);
    integrateAlg->setProperty("P1Bin", pbins[0]);
    integrateAlg->setProperty("P2Bin", pbins[1]);
    integrateAlg->setProperty("P3Bin", pbins[2]);
    integrateAlg->setProperty("P4Bin", pbins[3]);
    integrateAlg->setProperty("P5Bin", pbins[4]);
    integrateAlg->execute();
    IMDHistoWorkspace_sptr temp = integrateAlg->getProperty("OutputWorkspace");
    sliceWS = temp;
  } else { // We are processing an MDEventWorkspace

    auto eventInWS = std::dynamic_pointer_cast<IMDEventWorkspace>(inWS);
    const bool noPix = getProperty("NoPix");

    // Check Projection format
    Projection projection;
    if (projectionWS)
      projection = Projection(*projectionWS);

    // Check PBin properties
    for (size_t i = 0; i < 5; ++i) {
      if (i < numDims && pbins[i].empty())
        throw std::runtime_error("P" + std::to_string(i + 1) + "Bin must be set when processing a workspace with " +
                                 std::to_string(numDims) + " dimensions.");
      if (i >= numDims && !pbins[i].empty())
        throw std::runtime_error("P" + std::to_string(i + 1) + "Bin must NOT be set when processing a workspace with " +
                                 std::to_string(numDims) + " dimensions.");
    }

    // Get extents in projection
    std::vector<MinMax> extentLimits{getDimensionExtents(eventInWS, 0), getDimensionExtents(eventInWS, 1),
                                     getDimensionExtents(eventInWS, 2)};

    // Scale projection
    DblMatrix projectionMatrix(3, 3);
    projectionMatrix.setRow(0, projection.U());
    projectionMatrix.setRow(1, projection.V());
    projectionMatrix.setRow(2, projection.W());

    std::vector<std::string> targetUnits(3);
    for (size_t i = 0; i < 3; ++i)
      targetUnits[i] = projection.getUnit(i) == RLU ? RLUSymbol : InvAngstromSymbol;

    const std::string determineUnitsMethod = this->getProperty("InterpretQDimensionUnits");
    std::vector<std::string> originUnits;
    if (determineUnitsMethod == AutoMethod) {
      originUnits = findOriginalQUnits(inWS, g_log);
    } else if (determineUnitsMethod == RLUMethod) {
      originUnits = std::vector<std::string>(3, RLUSymbol);
    } else {
      originUnits = std::vector<std::string>(3, InvAngstromSymbol);
    }

    DblMatrix scaledProjectionMatrix = scaleProjection(projectionMatrix, originUnits, targetUnits, eventInWS);

    // Calculate extents for the first 3 dimensions
    std::vector<MinMax> scaledExtents = calculateExtents(scaledProjectionMatrix, extentLimits);
    auto stepPair = calculateSteps(scaledExtents, pbins);
    std::vector<MinMax> steppedExtents = stepPair.first;
    std::vector<int> steppedBins = stepPair.second;

    // Calculate extents for additional dimensions
    for (size_t i = 3; i < numDims; ++i) {
      const size_t nArgs = pbins[i].size();
      const MinMax extentLimit = getDimensionExtents(eventInWS, i);
      const double extentRange = extentLimit.second - extentLimit.first;

      if (nArgs == 1) {
        steppedExtents.emplace_back(extentLimit);
        steppedBins.emplace_back(static_cast<int>(extentRange / pbins[i][0]));
      } else if (nArgs == 2) {
        steppedExtents.emplace_back(pbins[i][0], pbins[i][1]);
        steppedBins.emplace_back(1);
      } else if (nArgs == 3) {
        const double dimRange = pbins[i][2] - pbins[i][0];
        const double stepSize = pbins[i][1] < dimRange ? pbins[i][1] : dimRange;
        steppedExtents.emplace_back(pbins[i][0], pbins[i][2]);
        steppedBins.emplace_back(static_cast<int>(dimRange / stepSize));
      }

      // and double targetUnits' length by appending itself to itself
      const size_t preSize = targetUnits.size();
      targetUnits.resize(preSize * 2);
      auto halfEnd = targetUnits.begin() + preSize;
      std::copy(targetUnits.begin(), halfEnd, halfEnd);
    }

    // Make labels
    std::vector<std::string> labels = labelProjection(projectionMatrix);

    // Either run RebinMD or SliceMD
    const std::string cutAlgName = noPix ? "BinMD" : "SliceMD";
    auto cutAlg = createChildAlgorithm(cutAlgName, 0.0, 1.0);
    cutAlg->initialize();
    cutAlg->setProperty("InputWorkspace", inWS);
    cutAlg->setProperty("OutputWorkspace", "sliced");
    cutAlg->setProperty("NormalizeBasisVectors", false);
    cutAlg->setProperty("AxisAligned", false);
    if (!noPix) {
      int recursion_depth = getProperty("MaxRecursionDepth");
      cutAlg->setProperty("TakeMaxRecursionDepthFromInput", false);
      cutAlg->setProperty("MaxRecursionDepth", recursion_depth);
    }

    for (size_t i = 0; i < numDims; ++i) {
      std::string label;
      std::string unit;
      std::string vecStr;

      if (i < 3) {
        // Slicing algorithms accept name as [x, y, z]
        label = labels[i];
        unit = targetUnits[i];

        std::vector<std::string> vec(numDims, "0");
        for (size_t j = 0; j < 3; ++j)
          vec[j] = boost::lexical_cast<std::string>(scaledProjectionMatrix[i][j]);
        vecStr = boost::algorithm::join(vec, ", ");
      } else {
        // Always orthogonal
        auto dim = inWS->getDimension(i);
        label = dim->getName();
        unit = dim->getUnits();
        std::vector<std::string> vec(numDims, "0");
        vec[i] = "1";
        vecStr = boost::algorithm::join(vec, ", ");
      }

      const std::string value = std::string(label).append(", ").append(unit).append(", ").append(vecStr);
      cutAlg->setProperty("BasisVector" + std::to_string(i), value);
    }

    // Translate extents into a single vector
    std::vector<double> outExtents(steppedExtents.size() * 2);
    for (size_t i = 0; i < steppedExtents.size(); ++i) {
      outExtents[2 * i] = steppedExtents[i].first;
      outExtents[2 * i + 1] = steppedExtents[i].second;
    }

    cutAlg->setProperty("OutputExtents", outExtents);
    cutAlg->setProperty("OutputBins", steppedBins);

    cutAlg->execute();
    sliceWS = cutAlg->getProperty("OutputWorkspace");

    MultipleExperimentInfos_sptr sliceInfo = std::dynamic_pointer_cast<MultipleExperimentInfos>(sliceWS);

    if (!sliceInfo)
      throw std::runtime_error("Could not extract experiment info from child's OutputWorkspace");

    // Attach projection matrix to output
    if (sliceInfo->getNumExperimentInfo() > 0) {
      ExperimentInfo_sptr info = sliceInfo->getExperimentInfo(0);
      info->mutableRun().addProperty("W_MATRIX", projectionMatrix.getVector(), true);
    }
  }

  auto geometry = std::dynamic_pointer_cast<Mantid::API::MDGeometry>(sliceWS);

  /* Original workspace and transformation information does not make sense for
   * self-contained Horace-style
   * cuts, so clear it out. */
  geometry->clearTransforms();
  geometry->clearOriginalWorkspaces();

  setProperty("OutputWorkspace", sliceWS);
}

} // namespace Mantid::MDAlgorithms
