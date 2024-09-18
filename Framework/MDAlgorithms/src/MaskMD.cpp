// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidMDAlgorithms/MaskMD.h"
#include "MantidAPI/IMDWorkspace.h"
#include "MantidGeometry/MDGeometry/MDBoxImplicitFunction.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/MandatoryValidator.h"
#include <algorithm>
#include <boost/algorithm/string.hpp>
#include <boost/regex.hpp>

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Geometry;
using boost::regex;

namespace Mantid::MDAlgorithms {

/*
 * The list of dimension names often looks like "[H,0,0],[0,K,0]" with "[H,0,0]"
 * being the first dimension but getProperty returns a vector of
 * the string split on every comma
 * This function parses the string, and does not split on commas within brackets
 */
std::vector<std::string> parseDimensionNames(const std::string &names_string) {

  // This regex has two parts which are separated by the "|" (or)
  // The first part matches anything which is bounded by square brackets
  // unless they contain square brackets (so that it only matches inner pairs)
  // The second part matches anything that doesn't contain a comma
  // NB, the order of the two parts matters
  regex expression(R"(\[([^\[]*)\]|[^,]+)");

  boost::sregex_token_iterator iter(names_string.begin(), names_string.end(), expression, 0);
  boost::sregex_token_iterator end;

  std::vector<std::string> names_result(iter, end);

  return names_result;
}

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(MaskMD)

/// Local type to group min, max extents with a dimension index.
struct InputArgument {
  double min, max;
  size_t index;
};

/// Comparator to allow sorting by dimension index.
struct LessThanIndex {
  bool operator()(const InputArgument &a, const InputArgument &b) const { return a.index < b.index; }
};

//----------------------------------------------------------------------------------------------
/// Algorithm's name for identification. @see Algorithm::name
const std::string MaskMD::name() const { return "MaskMD"; }

/// Algorithm's version for identification. @see Algorithm::version
int MaskMD::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string MaskMD::category() const { return "MDAlgorithms\\Transforms"; }

//----------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void MaskMD::init() {
  declareProperty(std::make_unique<PropertyWithValue<bool>>("ClearExistingMasks", true, Direction::Input),
                  "Clears any existing masks before applying the provided masking.");
  declareProperty(std::make_unique<WorkspaceProperty<IMDWorkspace>>("Workspace", "", Direction::InOut),
                  "An input/output workspace.");
  declareProperty(std::make_unique<ArrayProperty<std::string>>(
                      "Dimensions", std::make_shared<MandatoryValidator<std::vector<std::string>>>(), Direction::Input),
                  "Dimension ids/names all comma separated.\n"
                  "According to the dimensionality of the workspace, these names will be "
                  "grouped,\n"
                  "so the number of entries must be n*(number of dimensions in the "
                  "workspace).");

  declareProperty(std::make_unique<ArrayProperty<double>>(
                      "Extents", std::make_shared<MandatoryValidator<std::vector<double>>>(), Direction::Input),
                  "Extents {min, max} corresponding to each of the dimensions specified, "
                  "according to the order those identifies have been specified.");
}

/**
Free helper function.
try to fetch the workspace index.
@param ws : The workspace to find the dimensions in
@param candidateNameOrId: Either the name or the id of a dimension in the
workspace.
@return the index of the dimension in the workspace.
@throws runtime_error if the requested dimension is unknown either by id, or by
name in the workspace.
*/
size_t tryFetchDimensionIndex(const Mantid::API::IMDWorkspace_sptr &ws, const std::string &candidateNameOrId) {
  size_t dimWorkspaceIndex;
  try {
    dimWorkspaceIndex = ws->getDimensionIndexById(candidateNameOrId);
  } catch (const std::runtime_error &) {
    // this will throw if the name is unknown.
    dimWorkspaceIndex = ws->getDimensionIndexByName(candidateNameOrId);
  }
  return dimWorkspaceIndex;
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void MaskMD::exec() {
  IMDWorkspace_sptr ws = getProperty("Workspace");
  std::string dimensions_string = getPropertyValue("Dimensions");
  std::vector<double> extents = getProperty("Extents");

  // Dimension names may contain brackets with commas (i.e. [H,0,0])
  // so getProperty would return an incorrect vector of names;
  // instead get the string and parse it here
  std::vector<std::string> dimensions = parseDimensionNames(dimensions_string);
  // Report what dimension names were found
  g_log.debug() << "Dimension names parsed as: \n";
  for (const auto &dimension : dimensions) {
    g_log.debug() << dimension << '\n';
  }

  size_t nDims = ws->getNumDims();
  size_t nDimensionIds = dimensions.size();

  size_t nGroups = nDimensionIds / nDims;

  bool bClearExistingMasks = getProperty("ClearExistingMasks");
  if (bClearExistingMasks) {
    ws->clearMDMasking();
  }
  this->interruption_point();
  this->progress(0.0);

  // Explicitly cast nGroups and group to double to avoid compiler warnings
  // loss of precision does not matter as we are only using the result
  // for reporting algorithm progress
  const auto nGroups_double = static_cast<double>(nGroups);
  // Loop over all groups
  for (size_t group = 0; group < nGroups; ++group) {
    std::vector<InputArgument> arguments(nDims);

    // Loop over all arguments within the group. and construct InputArguments
    // for sorting.
    for (size_t i = 0; i < nDims; ++i) {
      size_t index = i + (group * nDims);
      InputArgument &arg = arguments[i];

      // Try to get the index of the dimension in the workspace.
      arg.index = tryFetchDimensionIndex(ws, dimensions[index]);

      arg.min = extents[index * 2];
      arg.max = extents[(index * 2) + 1];
    }

    // Sort all the inputs by the dimension index. Without this it will not be
    // possible to construct the MDImplicit function property.
    LessThanIndex comparator;
    std::sort(arguments.begin(), arguments.end(), comparator);

    // Create inputs for a box implicit function
    VMD mins(nDims);
    VMD maxs(nDims);
    for (size_t i = 0; i < nDims; ++i) {
      mins[i] = float(arguments[i].min);
      maxs[i] = float(arguments[i].max);
    }

    // Add new masking.
    ws->setMDMasking(std::make_unique<MDBoxImplicitFunction>(mins, maxs));
    this->interruption_point();
    auto group_double = static_cast<double>(group);
    this->progress(group_double / nGroups_double);
  }
  this->progress(1.0); // Ensure algorithm progress is reported as complete
}

std::map<std::string, std::string> MaskMD::validateInputs() {
  // Create the map
  std::map<std::string, std::string> validation_output;

  // Get properties to validate
  IMDWorkspace_sptr ws = getProperty("Workspace");
  std::string dimensions_string = getPropertyValue("Dimensions");
  std::vector<double> extents = getProperty("Extents");

  std::vector<std::string> dimensions = parseDimensionNames(dimensions_string);

  std::stringstream messageStream;

  // Check named dimensions can be found in workspace
  for (const auto &dimension_name : dimensions) {
    try {
      tryFetchDimensionIndex(ws, dimension_name);
    } catch (const std::runtime_error &) {
      messageStream << "Dimension '" << dimension_name << "' not found. ";
    }
  }
  if (messageStream.rdbuf()->in_avail() != 0) {
    validation_output["Dimensions"] = messageStream.str();
    messageStream.str(std::string());
  }

  size_t nDims = ws->getNumDims();
  size_t nDimensionIds = dimensions.size();

  // Check cardinality on names/ids
  if (nDimensionIds % nDims != 0) {
    messageStream << "Number of dimension ids/names must be n * " << nDims << ". The following names were given: ";
    for (const auto &dimension : dimensions) {
      messageStream << dimension << ", ";
    }

    validation_output["Dimensions"] = messageStream.str();
    messageStream.str(std::string());
  }

  // Check cardinality on extents
  if (extents.size() != (2 * dimensions.size())) {
    messageStream << "Number of extents must be " << 2 * dimensions.size() << ". ";
    validation_output["Extents"] = messageStream.str();
  }
  // Check extent value provided.
  for (size_t i = 0; (i < nDimensionIds) && ((i * 2 + 1) < extents.size()); ++i) {
    double min = extents[i * 2];
    double max = extents[(i * 2) + 1];
    if (min > max) {
      messageStream << "Cannot have minimum extents " << min << " larger than maximum extents " << max << ". ";
      validation_output["Extents"] = messageStream.str();
    }
  }

  return validation_output;
}

} // namespace Mantid::MDAlgorithms
