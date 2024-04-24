// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/LoadILLPolarizationFactors.h"

#include "MantidAPI/FileProperty.h"
#include "MantidAPI/IncreasingAxisValidator.h"
#include "MantidAPI/TextAxis.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidHistogramData/Histogram.h"
#include "MantidHistogramData/Interpolate.h"

#include <fstream>

namespace {
/// Constants for algorithm's property names.
namespace Prop {
const static std::string FILENAME{"Filename"};
const static std::string OUT_WS{"OutputWorkspace"};
const static std::string REF_WS{"WavelengthReference"};
} // namespace Prop

/// A struct holding the due of number arrays in the IDL files.
struct FactorDefinition {
  /// Wavelength points where the linear coeffs change.
  std::vector<double> limits;
  /// The linear coeffs to construct the efficiencies from.
  std::vector<double> fitFactors;
};

/// Tags for the polarization factors.
enum class Factor { F1, F2, Phi, P1, P2 };

/// Maps a string to a Factor tag.
Factor factor(const std::string &l) {
  const auto id = l.substr(0, 2);
  if (id == "F1")
    return Factor::F1;
  if (id == "F2")
    return Factor::F2;
  if (id == "Ph")
    return Factor::Phi;
  if (id == "P1")
    return Factor::P1;
  if (id == "P2")
    return Factor::P2;
  throw std::runtime_error("Syntax error.");
}

/// Returns a list of all available Factor tags.
const std::array<Factor, 5> factor_list() { return {{Factor::F1, Factor::F2, Factor::P1, Factor::P2, Factor::Phi}}; }

/// Returns the string presentation of tag f.
std::string to_string(const Factor f) {
  switch (f) {
  case Factor::F1:
    return "F1";
  case Factor::F2:
    return "F2";
  case Factor::P1:
    return "P1";
  case Factor::P2:
    return "P2";
  case Factor::Phi:
    return "Phi";
  }
  throw std::runtime_error("Unknown polarization correction factor tag.");
}

/// Returns `l` with comments erased.
std::string cleanse_comments(const std::string &l) {
  const auto commentBegin = l.find(';');
  // commentBegin == npos is OK.
  return l.substr(0, commentBegin);
}

/// Removes whitspace from 'l'.
void cleanse_whitespace(std::string &l) { l.erase(std::remove_if(l.begin(), l.end(), isspace), l.end()); }

/// Returns true if `l` contains the limits array.
bool contains_limits(const std::string &l) { return l.find("_limits") != std::string::npos; }

/// Converts the IDL array in `l` to std::vector.
std::vector<double> extract_values(const std::string &l) {
  const auto valBegin = l.find('[');
  const auto valEnd = l.find(']');
  if (valBegin == std::string::npos || valEnd == std::string::npos)
    throw std::runtime_error("Syntax error.");
  std::vector<double> values;
  std::string valStr;
  for (size_t i = valBegin + 1; i != valEnd; i++) {
    const auto c = l[i];
    if (c != ',') {
      valStr.push_back(c);
      if (i != valEnd - 1) {
        continue;
      }
    }
    double v;
    try {
      v = std::stod(valStr);
    } catch (std::exception &) {
      throw std::runtime_error("Syntax error");
    }
    values.emplace_back(v);
    valStr.clear();
  }
  return values;
}

/// Returns a histogram with X set to [0, ...points..., maxWavelength)].
Mantid::HistogramData::Histogram make_histogram(const std::vector<double> &points, const double maxWavelength) {
  Mantid::HistogramData::Points p(points.size() + 2);
  p.mutableRawData().front() = 0;
  p.mutableRawData().back() = maxWavelength > points.back() ? maxWavelength : 2 * points.back();
  for (size_t i = 1; i != p.size() - 1; ++i) {
    p.mutableData()[i] = points[i - 1];
  }
  const Mantid::HistogramData::Counts c(p.size());
  return Mantid::HistogramData::Histogram(p, c);
}

/// Fills `h` with the efficiency factors.
void calculate_factors_in_place(Mantid::HistogramData::Histogram &h, const std::vector<double> &piecewiseFactors) {
  const auto &xs = h.x();
  auto &ys = h.mutableY();
  ys[0] = piecewiseFactors.front();
  for (size_t i = 1; i != h.size(); ++i) {
    ys[i] = ys[i - 1] + piecewiseFactors[i] * (xs[i] - xs[i - 1]);
  }
}

/// Parses `in`, returns a map from factor tags to their numeric definitions.
std::map<Factor, FactorDefinition> parse(std::istream &in) {
  std::map<Factor, FactorDefinition> factors;
  std::string l;
  while (!in.eof()) {
    try {
      std::getline(in, l);
    } catch (std::exception &e) {
      throw std::runtime_error(std::string("Unknown exception: ") + e.what());
    }
    cleanse_whitespace(l);
    l = cleanse_comments(l);
    if (l.empty())
      continue;
    auto &fDef = factors[factor(l)];
    const auto values = extract_values(l);
    if (contains_limits(l)) {
      fDef.limits = values;
    } else {
      fDef.fitFactors = values;
    }
  }
  return factors;
}

/// Checks that all needed data has been gathered.
void definition_map_sanity_check(const std::map<Factor, FactorDefinition> &m) {
  const auto factors = factor_list();
  for (const auto f : factors) {
    const auto i = m.find(f);
    if (i == m.end()) {
      throw std::runtime_error("One of the factors is missing.");
    }
    const auto &fDef = (*i).second;
    if (fDef.limits.empty()) {
      throw std::runtime_error("No limits defined for a factor.");
    }
    if (fDef.fitFactors.empty()) {
      throw std::runtime_error("No fitting information defined for a factor.");
    }
    if (fDef.limits.size() + 2 != fDef.fitFactors.size()) {
      throw std::runtime_error("Size mismatch between limits and fitting information.");
    }
  }
}

/// Calculates error estimates in place.
void addErrors(Mantid::HistogramData::Histogram &h, const Factor tag) {
  // The error estimates are taken from the LAMP/COSMOS software.
  const auto f = [tag]() {
    switch (tag) {
    case Factor::F1:
    case Factor::F2:
      return 1. / 3000.;
    case Factor::P1:
    case Factor::P2:
    case Factor::Phi:
      return 1. / 500.;
    default:
      throw std::logic_error("Logic error: unknown efficiency factor tag.");
    }
  }();
  h.mutableE() = (h.y() * f).rawData();
}

/// Sets the Y and X units for ws.
void setUnits(Mantid::API::MatrixWorkspace &ws) {
  auto xAxis = ws.getAxis(0);
  xAxis->setUnit("Wavelength");
  ws.setYUnit("Polarization efficiency");
}
} // namespace

namespace Mantid::DataHandling {

using Mantid::API::WorkspaceProperty;
using Mantid::Kernel::Direction;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(LoadILLPolarizationFactors)

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string LoadILLPolarizationFactors::name() const { return "LoadILLPolarizationFactors"; }

/// Algorithm's version for identification. @see Algorithm::version
int LoadILLPolarizationFactors::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string LoadILLPolarizationFactors::category() const { return "DataHandling\\Text;ILL\\Reflectometry"; }

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string LoadILLPolarizationFactors::summary() const {
  return "Loads ILL formatted reflectometry polarization efficiency factors.";
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void LoadILLPolarizationFactors::init() {
  declareProperty(std::make_unique<API::FileProperty>(Prop::FILENAME, "", API::FileProperty::Load),
                  "Path to the polarization efficiency file.");
  const auto refWSValidator = std::make_shared<API::IncreasingAxisValidator>();
  declareProperty(
      std::make_unique<WorkspaceProperty<API::MatrixWorkspace>>(Prop::OUT_WS, "", Direction::Output, refWSValidator),
      "An output workspace containing the efficiencies at the "
      "reference workspace's wavelength points.");
  declareProperty(std::make_unique<WorkspaceProperty<API::MatrixWorkspace>>(Prop::REF_WS, "", Direction::Input),
                  "A reference workspace to get the wavelength axis from.");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void LoadILLPolarizationFactors::exec() {
  API::MatrixWorkspace_const_sptr refWS = getProperty(Prop::REF_WS);
  HistogramData::Histogram tmplHist{refWS->histogram(0).points()};
  API::MatrixWorkspace_sptr outWS = DataObjects::create<DataObjects::Workspace2D>(5, tmplHist);
  auto outVertAxis = std::make_unique<API::TextAxis>(5);
  const auto maxWavelength = tmplHist.x().back();

  const std::string filename = getProperty(Prop::FILENAME);
  std::ifstream in(filename);
  if (in.bad()) {
    throw std::runtime_error("Couldn't open file " + filename);
  }
  const auto fittingData = [&in, &filename]() {
    try {
      const auto data = parse(in);
      definition_map_sanity_check(data);
      return data;
    } catch (std::exception &e) {
      throw std::runtime_error("Error while reading " + filename + ": " + e.what());
    }
  }();
  const auto factorTags = factor_list();
  PARALLEL_FOR_IF(Kernel::threadSafe(*refWS))
  for (int i = 0; i < static_cast<int>(factorTags.size()); ++i) {
    PARALLEL_START_INTERRUPT_REGION
    const auto tag = factorTags[i];
    const auto &fDef = fittingData.at(tag);
    auto source = make_histogram(fDef.limits, maxWavelength);
    calculate_factors_in_place(source, fDef.fitFactors);
    auto target = outWS->histogram(i);
    HistogramData::interpolateLinearInplace(source, target);
    addErrors(target, tag);
    HistogramData::Histogram outH{refWS->histogram(0)};
    outH.setSharedY(target.sharedY());
    outH.setSharedE(target.sharedE());
    outWS->setHistogram(i, outH);
    outVertAxis->setLabel(i, to_string(tag));
    PARALLEL_END_INTERRUPT_REGION
  }
  PARALLEL_CHECK_INTERRUPT_REGION
  outWS->replaceAxis(1, std::move(outVertAxis));
  setUnits(*outWS);
  outWS->setTitle("Polarization efficiency factors");
  setProperty(Prop::OUT_WS, outWS);
}

/** Validates the algorithm's inputs.
 * @return a map from property names to discovered issues.
 */
std::map<std::string, std::string> LoadILLPolarizationFactors::validateInputs() {
  std::map<std::string, std::string> issues;
  API::MatrixWorkspace_const_sptr refWS = getProperty(Prop::REF_WS);
  if (refWS->getNumberHistograms() == 0) {
    issues[Prop::REF_WS] = "The reference workspace does not contain any histograms.";
    return issues;
  }
  const auto &xs = refWS->x(0);
  // A validator should have checked that xs is ordered.
  if (xs.front() < 0) {
    issues[Prop::REF_WS] = "The reference workspace contains negative X values.";
  }
  return issues;
}

} // namespace Mantid::DataHandling
