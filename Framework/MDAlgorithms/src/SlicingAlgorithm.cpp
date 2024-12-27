// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidMDAlgorithms/SlicingAlgorithm.h"
#include "MantidAPI/Run.h"
#include "MantidDataObjects/CoordTransformAffine.h"
#include "MantidDataObjects/CoordTransformAligned.h"
#include "MantidGeometry/MDGeometry/MDBoxImplicitFunction.h"
#include "MantidGeometry/MDGeometry/MDHistoDimension.h"
#include "MantidGeometry/MDGeometry/MDPlane.h"
#include "MantidKernel/ArrayLengthValidator.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/EnabledWhenProperty.h"
#include "MantidKernel/Strings.h"
#include "MantidKernel/VisibleWhenProperty.h"

#include <boost/regex.hpp>
#include <utility>

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Geometry;
using Mantid::Kernel::Strings::strip;

namespace Mantid::MDAlgorithms {

/** Constructor
 */
SlicingAlgorithm::SlicingAlgorithm()
    : m_transform(), m_transformFromOriginal(), m_transformToOriginal(), m_transformFromIntermediate(),
      m_transformToIntermediate(), m_axisAligned(true), m_outD(0), // unititialized and should be invalid
      m_NormalizeBasisVectors(false) {}

/** Initialize the algorithm's properties.
 */
void SlicingAlgorithm::initSlicingProps() {
  std::string dimChars = getDimensionChars();

  // --------------- Axis-aligned properties
  // ---------------------------------------
  declareProperty("AxisAligned", true, "Perform binning aligned with the axes of the input MDEventWorkspace?");
  setPropertyGroup("AxisAligned", "Axis-Aligned Binning");
  for (size_t i = 0; i < dimChars.size(); i++) {
    std::string dim(" ");
    dim[0] = dimChars[i];
    std::string propName = "AlignedDim" + dim;
    declareProperty(std::make_unique<PropertyWithValue<std::string>>(propName, "", Direction::Input),
                    "Binning parameters for the " + Strings::toString(i) +
                        "th dimension.\n"
                        "Enter it as a comma-separated list of values with the format: "
                        "'name,minimum,maximum,number_of_bins'. Leave blank for NONE.");
    setPropertySettings(propName, std::make_unique<VisibleWhenProperty>("AxisAligned", IS_EQUAL_TO, "1"));
    setPropertyGroup(propName, "Axis-Aligned Binning");
  }

  // --------------- NON-Axis-aligned properties
  // ---------------------------------------
  std::string grpName = "Non-Aligned Binning";

  auto ps = [] {
    std::unique_ptr<IPropertySettings> settings =
        std::make_unique<VisibleWhenProperty>("AxisAligned", IS_EQUAL_TO, "0");
    return settings;
  };
  for (size_t i = 0; i < dimChars.size(); i++) {
    std::string dim(" ");
    dim[0] = dimChars[i];
    std::string propName = "BasisVector" + dim;
    declareProperty(std::make_unique<PropertyWithValue<std::string>>(propName, "", Direction::Input),
                    "Description of the basis vector of the " + Strings::toString(i) +
                        "th output dimension."
                        "Format: 'name, units, x,y,z,..'.\n"
                        "  name : string for the name of the output dimension.\n"
                        "  units : string for the units of the output dimension.\n"
                        "  x,y,z,...: vector definining the basis in the input dimensions "
                        "space.\n"
                        "Leave blank for NONE.");
    setPropertySettings(propName, ps());
    setPropertyGroup(propName, grpName);
  }
  declareProperty(std::make_unique<ArrayProperty<double>>("Translation", Direction::Input),
                  "Coordinates in the INPUT workspace that corresponds to "
                  "(0,0,0) in the OUTPUT workspace.\n"
                  "Enter as a comma-separated string.\n"
                  "Default: 0 in all dimensions (no translation).");

  declareProperty(std::make_unique<ArrayProperty<double>>("OutputExtents", Direction::Input),
                  "The minimum, maximum edges of space of each dimension of "
                  "the OUTPUT workspace, as a comma-separated list");

  declareProperty(std::make_unique<ArrayProperty<int>>("OutputBins", Direction::Input),
                  "The number of bins for each dimension of the OUTPUT workspace.");

  declareProperty(std::make_unique<PropertyWithValue<bool>>("NormalizeBasisVectors", true, Direction::Input),
                  "Normalize the given basis vectors to unity. \n"
                  "If true, then a distance of 1 in the INPUT dimensions = 1 "
                  "in the OUTPUT dimensions.\n"
                  "If false, then a distance of norm(basis_vector) in the "
                  "INPUT dimension = 1 in the OUTPUT dimensions.");

  declareProperty(std::make_unique<PropertyWithValue<bool>>("ForceOrthogonal", false, Direction::Input),
                  "Force the input basis vectors to form an orthogonal coordinate system. "
                  "Only works in 3 dimension!");

  // For GUI niceness
  setPropertyGroup("Translation", grpName);
  setPropertyGroup("OutputExtents", grpName);
  setPropertyGroup("OutputBins", grpName);
  setPropertyGroup("NormalizeBasisVectors", grpName);
  setPropertyGroup("ForceOrthogonal", grpName);
  setPropertySettings("Translation", ps());
  setPropertySettings("OutputExtents", ps());
  setPropertySettings("OutputBins", ps());
  setPropertySettings("NormalizeBasisVectors", ps());
  setPropertySettings("ForceOrthogonal", ps());
}

//----------------------------------------------------------------------------------------------
/** Generate the MDHistoDimension and basis vector for a given string from
 *BasisVector0 etc.
 *
 * If the workspace being binned has an original workspace, then the vector
 * is transformed to THOSE coordinates.
 *
 *  "Format: 'name, units, x,y,z,..'.\n"
 * Adds values to m_bases, m_binDimensions,
 *    m_binningScaling and m_transformScaling
 *
 * @param str :: name,number_of_bins
 */
void SlicingAlgorithm::makeBasisVectorFromString(const std::string &str) {
  std::string input = Strings::strip(str);
  if (input.empty())
    return;
  if (input.size() < 3)
    throw std::invalid_argument("Dimension string is too short to be valid: " + str);

  // The current dimension index.
  size_t dim = m_binDimensions.size();

  size_t n_first_comma;

  // Special case: accept dimension names [x,y,z]
  if (input[0] == '[') {
    // Find the name at the closing []
    size_t n = input.find_first_of(']', 1);
    if (n == std::string::npos)
      throw std::invalid_argument("No closing ] character in the dimension name of : " + str);
    // Find the comma after the name
    n_first_comma = input.find_first_of(',', n);
    if (n_first_comma == std::string::npos)
      throw std::invalid_argument("No comma after the closing ] character in the dimension string: " + str);
  } else
    // Find the comma after the name
    n_first_comma = input.find_first_of(',');

  if (n_first_comma == std::string::npos)
    throw std::invalid_argument("No comma in the dimension string: " + str);
  if (n_first_comma == input.size() - 1)
    throw std::invalid_argument("Dimension string ends in a comma: " + str);

  // Get the entire name
  std::string name = Strings::strip(input.substr(0, n_first_comma));
  if (name.empty())
    throw std::invalid_argument("name should not be blank.");

  // Now remove the name and comma
  // And split the rest of it
  input = input.substr(n_first_comma + 1);
  std::vector<std::string> strs;
  boost::split(strs, input, boost::is_any_of(","));
  if (strs.size() != this->m_inWS->getNumDims() + 1)
    throw std::invalid_argument("Wrong number of values (expected 2 + # of "
                                "input dimensions) in the dimensions string: " +
                                str);

  // Get the number of bins from
  int numBins = m_numBins[dim];
  if (numBins < 1)
    throw std::invalid_argument("Number of bins for output dimension " + Strings::toString(dim) + " should be >= 1.");

  // Get the min/max extents in this OUTPUT dimension
  double min = m_minExtents[dim];
  double max = m_maxExtents[dim];
  double lengthInOutput = max - min;
  if (lengthInOutput <= 0)
    throw std::invalid_argument("The maximum extents for dimension " + Strings::toString(dim) + " should be > 0.");

  // Create the basis vector with the right # of dimensions
  VMD basis(this->m_inWS->getNumDims());
  for (size_t d = 0; d < this->m_inWS->getNumDims(); d++)
    if (!Strings::convert(strs[d + 1], basis[d]))
      throw std::invalid_argument("Error converting argument '" + strs[d + 1] + "' in the dimensions string '" + str +
                                  "' to a number.");

  // If B was binned from A (m_originalWS), and we are binning C from B,
  // convert the basis vector from B space -> A space
  if (m_originalWS) {
    // Turn basis vector into two points
    VMD basis0(this->m_inWS->getNumDims());
    VMD basis1 = basis;
    // Convert the points to the original coordinates (from inWS to originalWS)
    CoordTransform const *toOrig = m_inWS->getTransformToOriginal();
    VMD origBasis0 = toOrig->applyVMD(basis0);
    VMD origBasis1 = toOrig->applyVMD(basis1);
    // New basis vector, now in the original workspace
    basis = origBasis1 - origBasis0;
  }

  // Check on the length of the basis vector
  double basisLength = basis.norm();
  if (basisLength <= 0)
    throw std::invalid_argument("direction should not be 0-length.");

  // Normalize it to unity, if desized
  double transformScaling = 1.0;
  if (m_NormalizeBasisVectors) {
    // A distance of 1 in the INPUT space = a distance of 1.0 in the OUTPUT
    // space
    basis.normalize();
    transformScaling = 1.0;
  } else
    // A distance of |basisVector| in the INPUT space = a distance of 1.0 in the
    // OUTPUT space
    transformScaling = (1.0 / basisLength);

  // This is the length of this dimension as measured in the INPUT space
  double lengthInInput = lengthInOutput / transformScaling;

  // Scaling factor, to convert from units in the INPUT dimensions to the output
  // BIN number
  double binningScaling = double(numBins) / (lengthInInput);

  // Extract the arguments
  std::string units = Strings::strip(strs[0]);

  // Create the appropriate frame
  auto frame = createMDFrameForNonAxisAligned(units, basis);

  // Create the output dimension
  auto out = std::make_shared<MDHistoDimension>(name, name, *frame, static_cast<coord_t>(min),
                                                static_cast<coord_t>(max), numBins);

  // Put both in the algo for future use
  m_bases.emplace_back(basis);
  m_binDimensions.emplace_back(std::move(out));
  m_binningScaling.emplace_back(binningScaling);
  m_transformScaling.emplace_back(transformScaling);
}

//----------------------------------------------------------------------------------------------
/** Reads the various Properties for the general (non-aligned) case
 * and fills in members on the Algorithm for later use
 *
 * @throw if some of the inputs are invalid
 */
void SlicingAlgorithm::processGeneralTransformProperties() {
  // Count the number of output dimensions
  m_outD = 0;
  std::string dimChars = this->getDimensionChars();
  for (char dimChar : dimChars) {
    std::string propName = "BasisVector0";
    propName[11] = dimChar;
    if (!Strings::strip(this->getPropertyValue(propName)).empty())
      m_outD++;
  }

  std::vector<double> extents = this->getProperty("OutputExtents");
  if (extents.size() != m_outD * 2)
    throw std::invalid_argument("The OutputExtents parameter must have " + Strings::toString(m_outD * 2) +
                                " entries "
                                "(2 for each dimension in the OUTPUT workspace).");

  m_minExtents.clear();
  m_maxExtents.clear();
  for (size_t d = 0; d < m_outD; d++) {
    m_minExtents.emplace_back(extents[d * 2]);
    m_maxExtents.emplace_back(extents[d * 2 + 1]);
  }

  m_numBins = this->getProperty("OutputBins");
  if (m_numBins.size() != m_outD)
    throw std::invalid_argument("The OutputBins parameter must have 1 entry "
                                "for each dimension in the OUTPUT workspace.");

  m_NormalizeBasisVectors = this->getProperty("NormalizeBasisVectors");
  m_transformScaling.clear();

  // Create the dimensions based on the strings from the user
  for (char dimChar : dimChars) {
    std::string propName = "BasisVector0";
    propName[11] = dimChar;
    try {
      makeBasisVectorFromString(getPropertyValue(propName));
    } catch (std::exception &e) {
      throw std::invalid_argument("Error parsing the " + propName + " parameter: " + std::string(e.what()));
    }
  }

  // Number of output binning dimensions found
  m_outD = m_binDimensions.size();
  if (m_outD == 0)
    throw std::runtime_error("No output dimensions were found in the MDEventWorkspace. Cannot bin!");

  // Get the Translation parameter
  std::vector<double> translVector;
  try {
    translVector = getProperty("Translation");
  } catch (std::exception &e) {
    throw std::invalid_argument("Error parsing the Translation parameter: " + std::string(e.what()));
  }

  // Default to 0,0,0 when not specified
  if (translVector.empty())
    translVector.resize(m_inWS->getNumDims(), 0);
  m_translation = VMD(translVector);

  if (m_translation.getNumDims() != m_inWS->getNumDims())
    throw std::invalid_argument("The number of dimensions in the Translation parameter is "
                                "not consistent with the number of dimensions in the input workspace.");

  // Validate
  if (m_outD > m_inWS->getNumDims())
    throw std::runtime_error("More output dimensions were specified than input dimensions "
                             "exist in the MDEventWorkspace. Cannot bin!");
  if (m_binningScaling.size() != m_outD)
    throw std::runtime_error("Inconsistent number of entries in scaling vector.");
}

//----------------------------------------------------------------------------------------------
/** Loads the dimensions and create the coordinate transform, using the inputs.
 * This is for the general (i.e. non-aligned) case
 */
void SlicingAlgorithm::createGeneralTransform() {
  // Process all the input properties
  this->processGeneralTransformProperties();

  // Number of input dimensions
  size_t inD = m_inWS->getNumDims();

  // ----- Make the basis vectors orthogonal -------------------------
  bool ForceOrthogonal = getProperty("ForceOrthogonal");
  if (ForceOrthogonal && m_bases[0].getNumDims() == 3 && m_bases.size() >= 2) {
    std::vector<VMD> firstTwo = m_bases;
    firstTwo.resize(2, VMD(3));
    std::vector<VMD> ortho = VMD::makeVectorsOrthogonal(firstTwo);
    // Set the bases back
    ortho.resize(m_bases.size(), VMD(3));
    m_bases = ortho;
    g_log.information() << "Basis vectors forced to be orthogonal: ";
    for (auto &base : m_bases)
      g_log.information() << base.toString(",") << "; ";
    g_log.information() << '\n';
  }

  // Now, convert the original vector to the coordinates of the ORIGNAL ws, if
  // any
  if (m_originalWS) {
    CoordTransform const *toOrig = m_inWS->getTransformToOriginal();
    m_translation = toOrig->applyVMD(m_translation);
  }

  // OK now find the min/max coordinates of the edges in the INPUT workspace
  m_inputMinPoint = m_translation;
  for (size_t d = 0; d < m_outD; d++) {
    // Translate from the outCoords=(0,0,0) to outCoords=(min,min,min)
    m_inputMinPoint += (m_bases[d] * m_binDimensions[d]->getMinimum());
  }

  // Create the CoordTransformAffine for BINNING with these basis vectors
  auto ct = std::make_unique<DataObjects::CoordTransformAffine>(inD, m_outD);
  // Note: the scaling makes the coordinate correspond to a bin index
  ct->buildNonOrthogonal(m_inputMinPoint, this->m_bases, VMD(this->m_binningScaling) / VMD(m_transformScaling));
  this->m_transform = std::move(ct);

  // Transformation original->binned
  auto ctFrom = std::make_unique<DataObjects::CoordTransformAffine>(inD, m_outD);
  ctFrom->buildNonOrthogonal(m_translation, this->m_bases, VMD(m_transformScaling) / VMD(m_transformScaling));
  m_transformFromOriginal = std::move(ctFrom);

  // Validate
  if (m_transform->getInD() != inD)
    throw std::invalid_argument("The number of input dimensions in the CoordinateTransform "
                                "object is not consistent with the number of dimensions in the input "
                                "workspace.");
  if (m_transform->getOutD() != m_outD)
    throw std::invalid_argument("The number of output dimensions in the CoordinateTransform "
                                "object is not consistent with the number of dimensions specified in "
                                "the OutDimX, etc. properties.");

  // Now the reverse transformation
  m_transformToOriginal = nullptr;
  if (m_outD == inD) {
    // Can't reverse transform if you lost dimensions.
    auto ctTo = std::make_unique<DataObjects::CoordTransformAffine>(inD, m_outD);
    auto toMatrix = static_cast<DataObjects::CoordTransformAffine *>(m_transformFromOriginal.get())->getMatrix();
    // Invert the affine matrix to get the reverse transformation
    toMatrix.Invert();
    ctTo->setMatrix(toMatrix);
    m_transformToOriginal = std::move(ctTo);
  }
}

//----------------------------------------------------------------------------------------------
/** Generate a MDHistoDimension_sptr from a comma-sep string (for AlignedDim0,
 *etc.)
 * Must be called in order X,Y,Z,T.
 *
 * @param str :: name,minimum,maximum,number_of_bins
 */
void SlicingAlgorithm::makeAlignedDimensionFromString(const std::string &str) {
  if (str.empty()) {
    throw std::runtime_error("Empty string passed to one of the AlignedDim0 parameters.");
  } else {
    // Strip spaces
    std::string input = Strings::strip(str);
    if (input.size() < 4)
      throw std::invalid_argument("Dimensions string is too short to be valid: " + str);

    // Find the 3rd comma from the end
    size_t n = std::string::npos;
    for (size_t i = 0; i < 3; i++) {
      n = input.find_last_of(',', n);
      if (n == std::string::npos)
        throw std::invalid_argument("Wrong number of values (4 are expected) "
                                    "in the dimensions string: " +
                                    str);
      if (n == 0)
        throw std::invalid_argument("Dimension string starts with a comma: " + str);
      n--;
    }
    // The name is everything before the 3rd comma from the end
    std::string name = input.substr(0, n + 1);
    name = Strings::strip(name);

    // And split the rest of it
    input = input.substr(n + 2);
    std::vector<std::string> strs;
    boost::split(strs, input, boost::is_any_of(","));
    if (strs.size() != 3)
      throw std::invalid_argument("Wrong number of values (3 are expected) after the name "
                                  "in the dimensions string: " +
                                  str);

    // Extract the arguments
    coord_t min, max;
    int numBins = 0;
    Strings::convert(strs[0], min);
    Strings::convert(strs[1], max);
    Strings::convert(strs[2], numBins);
    if (name.empty())
      throw std::invalid_argument("Name should not be blank.");
    if (min >= max)
      throw std::invalid_argument("Min should be > max.");
    if (numBins < 1)
      throw std::invalid_argument("Number of bins should be >= 1.");

    // Find the named axis in the input workspace
    size_t dim_index = 0;
    try {
      dim_index = m_inWS->getDimensionIndexByName(name);
    } catch (std::runtime_error &) {
      // The dimension was not found by name. Try using the ID
      try {
        dim_index = m_inWS->getDimensionIndexById(name);
      } catch (std::runtime_error &) {
        throw std::runtime_error("Dimension " + name +
                                 " was not found in the "
                                 "MDEventWorkspace! Cannot continue.");
      }
    }

    // Copy the dimension name, ID and units
    IMDDimension_const_sptr inputDim = m_inWS->getDimension(dim_index);
    const auto &frame = inputDim->getMDFrame();
    m_binDimensions.emplace_back(MDHistoDimension_sptr(
        new MDHistoDimension(inputDim->getName(), inputDim->getDimensionId(), frame, min, max, numBins)));

    // Add the index from which we're binning to the vector
    this->m_dimensionToBinFrom.emplace_back(dim_index);
  }
}
//----------------------------------------------------------------------------------------------
/** Using the parameters, create a coordinate transformation
 * for aligned cuts
 */
void SlicingAlgorithm::createAlignedTransform() {
  std::string dimChars = this->getDimensionChars();

  // Validate inputs
  bool previousWasEmpty = false;
  size_t numDims = 0;
  for (char dimChar : dimChars) {
    std::string propName = "AlignedDim0";
    propName[10] = dimChar;
    std::string prop = Strings::strip(getPropertyValue(propName));
    if (!prop.empty())
      numDims++;
    if (!prop.empty() && previousWasEmpty)
      throw std::invalid_argument("Please enter the AlignedDim parameters in the order 0,1,2, etc.,"
                                  "without skipping any entries.");
    previousWasEmpty = prop.empty();
  }

  // Number of input dimension
  size_t inD = m_inWS->getNumDims();
  // Validate
  if (numDims == 0)
    throw std::runtime_error("No output dimensions specified.");
  if (numDims > inD)
    throw std::runtime_error("More output dimensions were specified than input dimensions "
                             "exist in the MDEventWorkspace.");

  // Create the dimensions based on the strings from the user
  for (size_t i = 0; i < numDims; i++) {
    std::string propName = "AlignedDim0";
    propName[10] = dimChars[i];
    makeAlignedDimensionFromString(getPropertyValue(propName));
  }

  // Number of output binning dimensions found
  m_outD = m_binDimensions.size();

  // Now we build the coordinate transformation object
  m_translation = VMD(inD);
  m_bases.clear();
  std::vector<coord_t> origin(m_outD), scaling(m_outD);
  for (size_t d = 0; d < m_outD; d++) {
    origin[d] = m_binDimensions[d]->getMinimum();
    scaling[d] = 1.0f / m_binDimensions[d]->getBinWidth();
    // Origin in the input
    m_translation[m_dimensionToBinFrom[d]] = origin[d];
    // Create a unit basis vector that corresponds to this
    VMD basis(inD);
    basis[m_dimensionToBinFrom[d]] = 1.0;
    m_bases.emplace_back(basis);
  }

  // Transform for binning
  m_transform = std::make_unique<DataObjects::CoordTransformAligned>(m_inWS->getNumDims(), m_outD, m_dimensionToBinFrom,
                                                                     origin, scaling);

  // Transformation original->binned. There is no offset or scaling!
  std::vector<coord_t> unitScaling(m_outD, 1.0);
  std::vector<coord_t> zeroOrigin(m_outD, 0.0);
  m_transformFromOriginal =
      std::make_unique<DataObjects::CoordTransformAligned>(inD, m_outD, m_dimensionToBinFrom, zeroOrigin, unitScaling);

  // Now the reverse transformation.
  if (m_outD == inD) {
    // Make the reverse map = if you're in the output dimension "od", what INPUT
    // dimension index is that?
    Matrix<coord_t> mat = m_transformFromOriginal->makeAffineMatrix();
    mat.Invert();
    auto tmp = std::make_unique<DataObjects::CoordTransformAffine>(inD, m_outD);
    tmp->setMatrix(mat);
    m_transformToOriginal = std::move(tmp);
  } else {
    // Changed # of dimensions - can't reverse the transform
    m_transformToOriginal = nullptr;
    g_log.warning("SlicingAlgorithm: Your slice will cause the output "
                  "workspace to have fewer dimensions than the input. This will "
                  "affect your ability to create subsequent slices.");
  }
}

//-----------------------------------------------------------------------------------------------
/** Read the algorithm properties and creates the appropriate transforms
 * for slicing the MDEventWorkspace.
 *
 * NOTE: The m_inWS member must be set first.
 * If the workspace is based on another, e.g. result from BinMD,
 * m_inWS will be modified to be the original workspace and the transformations
 * will be altered to match.
 *
 * The m_transform, m_transformFromOriginal and m_transformToOriginal transforms
 *will be set.
 */
void SlicingAlgorithm::createTransform() {
  if (!m_inWS)
    throw std::runtime_error("SlicingAlgorithm::createTransform(): input "
                             "MDWorkspace must be set first!");
  if (std::dynamic_pointer_cast<MatrixWorkspace>(m_inWS))
    throw std::runtime_error(this->name() + " cannot be run on a MatrixWorkspace!");

  // Is the transformation aligned with axes?
  m_axisAligned = getProperty("AxisAligned");

  // Refer to the original workspace. Make sure that is possible
  if (m_inWS->numOriginalWorkspaces() > 0)
    m_originalWS = std::dynamic_pointer_cast<IMDWorkspace>(m_inWS->getOriginalWorkspace());
  if (m_originalWS) {
    if (m_axisAligned)
      throw std::runtime_error("Cannot perform axis-aligned binning on a MDHistoWorkspace. "
                               "Please use non-axis aligned binning.");

    if (m_originalWS->getNumDims() != m_inWS->getNumDims())
      throw std::runtime_error("SlicingAlgorithm::createTransform(): Cannot propagate "
                               "a transformation if the number of dimensions has changed.");

    if (!m_inWS->getTransformToOriginal())
      throw std::runtime_error("SlicingAlgorithm::createTransform(): Cannot propagate "
                               "a transformation. There is no transformation saved from " +
                               m_inWS->getName() + " back to " + m_originalWS->getName() + ".");

    // Fail if the MDHistoWorkspace was modified by binary operation
    DataObjects::MDHistoWorkspace_sptr inHisto = std::dynamic_pointer_cast<DataObjects::MDHistoWorkspace>(m_inWS);
    if (inHisto) {
      if (inHisto->getNumExperimentInfo() > 0) {
        const Run &run = inHisto->getExperimentInfo(0)->run();
        if (run.hasProperty("mdhisto_was_modified")) {
          Property *prop = run.getProperty("mdhisto_was_modified");
          if (prop) {
            if (prop->value() == "1") {
              throw std::runtime_error("This MDHistoWorkspace was modified by a binary operation "
                                       "(e.g. Plus, Minus). "
                                       "It is not currently possible to rebin a modified "
                                       "MDHistoWorkspace because that requires returning to the "
                                       "original "
                                       "(unmodified) MDEventWorkspace, and so would give incorrect "
                                       "results. "
                                       "Instead, you can use SliceMD and perform operations on the "
                                       "resulting "
                                       "MDEventWorkspaces, which preserve all events. "
                                       "You can override this check by removing the "
                                       "'mdhisto_was_modified' sample log.");
            }
          }
        }
      }
    }

    g_log.notice() << "Performing " << this->name() << " on the original workspace, '" << m_originalWS->getName()
                   << "'\n";
  }

  // Create the coordinate transformation
  m_transform = nullptr;
  if (m_axisAligned)
    this->createAlignedTransform();
  else
    this->createGeneralTransform();

  // Finalize, for binnign MDHistoWorkspace
  if (m_originalWS) {
    // The intermediate workspace is the MDHistoWorkspace being BINNED
    m_intermediateWS = m_inWS;
    CoordTransform const *originalToIntermediate = m_intermediateWS->getTransformFromOriginal();
    if (originalToIntermediate && (m_originalWS->getNumDims() == m_intermediateWS->getNumDims())) {
      try {
        // The transform from the INPUT to the INTERMEDIATE ws
        // intermediate_coords =  [OriginalToIntermediate] * [thisToOriginal] *
        // these_coords
        Matrix<coord_t> matToOriginal = m_transformToOriginal->makeAffineMatrix();
        Matrix<coord_t> matOriginalToIntermediate = originalToIntermediate->makeAffineMatrix();
        Matrix<coord_t> matToIntermediate = matOriginalToIntermediate * matToOriginal;

        m_transformToIntermediate = std::make_unique<DataObjects::CoordTransformAffine>(m_originalWS->getNumDims(),
                                                                                        m_intermediateWS->getNumDims());
        m_transformToIntermediate->setMatrix(matToIntermediate);
        // And now the reverse
        matToIntermediate.Invert();
        m_transformFromIntermediate = std::make_unique<DataObjects::CoordTransformAffine>(
            m_intermediateWS->getNumDims(), m_originalWS->getNumDims());
        m_transformFromIntermediate->setMatrix(matToIntermediate);
      } catch (std::runtime_error &) {
        // Ignore error. Have no transform.
      }
    }

    // Replace the input workspace with the original MDEventWorkspace
    // for future binning
    m_inWS = m_originalWS;
  }
}

//----------------------------------------------------------------------------------------------
/** Create an implicit function for picking boxes, based on the indexes in the
 * output MDHistoWorkspace.
 * This needs to be in the space of the INPUT MDEventWorkspace.
 *
 * In the most general case, this function assumes ORTHOGONAL BASIS VECTORS!
 * However, in the case of N dimensions with N or N-1 bases then non-orthogonal
 * basis vectors are allowed.
 *
 * @param chunkMin :: the minimum index in each dimension to consider "valid"
 *(inclusive).
 *        NULL to use the entire range.
 * @param chunkMax :: the maximum index in each dimension to consider "valid"
 *(exclusive)
 *        NULL to use the entire range.
 * @return MDImplicitFunction created
 */
std::unique_ptr<MDImplicitFunction> SlicingAlgorithm::getGeneralImplicitFunction(const size_t *const chunkMin,
                                                                                 const size_t *const chunkMax) {
  size_t nd = m_inWS->getNumDims();

  // General implicit function
  auto func = std::make_unique<MDImplicitFunction>();

  // First origin = min of each basis vector
  VMD o1 = m_translation;
  // Second origin = max of each basis vector
  VMD o2 = m_translation;
  // And this the list of basis vectors. Each vertex is given by o1+bases[i].
  std::vector<VMD> bases;
  VMD x;

  for (size_t d = 0; d < m_bases.size(); d++) {
    double xMin = m_binDimensions[d]->getMinimum();
    double xMax = m_binDimensions[d]->getMaximum();
    // Move the position if you're using a chunk
    if (chunkMin != nullptr)
      xMin = m_binDimensions[d]->getX(chunkMin[d]);
    if (chunkMax != nullptr)
      xMax = m_binDimensions[d]->getX(chunkMax[d]);
    // Offset the origin by the position along the basis vector
    o1 += (m_bases[d] * xMin);
    o2 += (m_bases[d] * xMax);

    VMD thisBase = m_bases[d] * (xMax - xMin);
    bases.emplace_back(thisBase);
    if (d == 0)
      x = thisBase;
  }

  // Dimensionality of the box
  size_t boxDim = bases.size();

  // Point that is sure to be inside the volume of interest
  VMD insidePoint = (o1 + o2) / 2.0;

  if (boxDim == 1) {
    // 2 planes defined by 1 basis vector
    // Your normal = the x vector
    func->addPlane(MDPlane(x, o1));
    func->addPlane(MDPlane(x * -1.0, o2));
  } else if (boxDim == nd || boxDim == nd - 1) {
    // Create a pair of planes for each base supplied. This is general to non-
    // orthogonal bases. If we have bases (x y z t) then we create the planes
    //
    // y z t
    // x z t
    // x y t
    // x y z
    //
    // Note: the last plane may or may not be created depending on the number
    // of bases supplied to the slicing algorithm relative to the number
    // of dimensions. i.e. if 3 bases were supplied and we have 4 dimensions
    // then 6 planes are created instead of 8.
    std::vector<VMD> vectors;

    for (size_t ignoreIndex = 0; ignoreIndex < boxDim; ++ignoreIndex) {
      vectors.clear();
      // Create a list of vectors that excludes the "current" basis
      for (size_t baseIndex = 0; baseIndex < boxDim; ++baseIndex) {
        if (baseIndex != ignoreIndex)
          vectors.emplace_back(bases[baseIndex]);
      }

      // if we have fewer basis vectors than dimensions
      // create a normal for the final dimension
      if (boxDim == nd - 1)
        vectors.emplace_back(VMD::getNormalVector(bases));

      // Add two planes for each set of vectors
      func->addPlane(MDPlane(vectors, o1, insidePoint));
      func->addPlane(MDPlane(vectors, o2, insidePoint));
    }
  } else {
    // Last-resort, totally general case
    // 2*N planes defined by N basis vectors, in any dimensionality workspace.
    // Assumes orthogonality!
    g_log.warning("SlicingAlgorithm given " + std::to_string(boxDim) + " bases and " + std::to_string(nd) +
                  " dimensions and " + "therefore will assume orthogonality");
    for (auto &base : bases) {
      // For each basis vector, make two planes, perpendicular to it and facing
      // inwards
      func->addPlane(MDPlane(base, o1));
      func->addPlane(MDPlane(base * -1.0, o2));
    }
  }

  return func;
}

//----------------------------------------------------------------------------------------------
/** Create an implicit function for picking boxes, based on the indexes in the
 * output MDHistoWorkspace.
 * This needs to be in the space of the INPUT MDEventWorkspace
 *
 * @param chunkMin :: the minimum index in each dimension to consider "valid"
 *(inclusive).
 *        NULL to use the entire range.
 * @param chunkMax :: the maximum index in each dimension to consider "valid"
 *(exclusive)
 *        NULL to use the entire range.
 * @return MDImplicitFunction created
 */
std::unique_ptr<MDImplicitFunction> SlicingAlgorithm::getImplicitFunctionForChunk(const size_t *const chunkMin,
                                                                                  const size_t *const chunkMax) {
  size_t nd = m_inWS->getNumDims();
  if (m_axisAligned) {
    std::vector<coord_t> function_min(nd, -1e30f); // default to all space if the dimension is not specified
    std::vector<coord_t> function_max(nd, +1e30f); // default to all space if the dimension is not specified
    for (size_t bd = 0; bd < m_outD; bd++) {
      // Dimension in the MDEventWorkspace
      size_t d = m_dimensionToBinFrom[bd];
      if (chunkMin)
        function_min[d] = m_binDimensions[bd]->getX(chunkMin[bd]);
      else
        function_min[d] = m_binDimensions[bd]->getX(0);
      if (chunkMax)
        function_max[d] = m_binDimensions[bd]->getX(chunkMax[bd]);
      else
        function_max[d] = m_binDimensions[bd]->getX(m_binDimensions[bd]->getNBins());
    }
    return std::make_unique<MDBoxImplicitFunction>(function_min, function_max);
  } else {
    // General implicit function
    return getGeneralImplicitFunction(chunkMin, chunkMax);
  }
}

/**
 * Create an MDFrame for the Non-Axis-Aligned case. Make sure that
 * MDFrames onto which the basis vector projects are not mixed, e.g. no mixing
 * of HKL and GenerFrame
 * @param units: the units
 * @param basisVector: the basis vector
 * @returns the unique pointer
 */
Mantid::Geometry::MDFrame_uptr
SlicingAlgorithm::createMDFrameForNonAxisAligned(const std::string &units,
                                                 const Mantid::Kernel::VMD &basisVector) const {
  // Get set of basis vectors
  auto oldBasis = getOldBasis(m_inWS->getNumDims());

  // Get indices onto which the vector projects
  auto indicesWithProjection = getIndicesWithProjection(basisVector, oldBasis);

  // Extract MDFrame
  return extractMDFrameForNonAxisAligned(indicesWithProjection, units);
}

std::vector<Mantid::Kernel::VMD> SlicingAlgorithm::getOldBasis(size_t dimension) const {
  std::vector<Mantid::Kernel::VMD> oldBasis;
  for (size_t i = 0; i < dimension; ++i) {
    Mantid::Kernel::VMD basisVector(dimension);
    basisVector[i] = 1.0;
    oldBasis.emplace_back(basisVector);
  }
  return oldBasis;
}

/**
 * Check if the two vectors are orthogonal or not
 * @param oldVector: the old vector
 * @param basisVector: the vector under investigation
 * @returns true if there is a projection  else false
 */
bool SlicingAlgorithm::isProjectingOnFrame(const Mantid::Kernel::VMD &oldVector,
                                           const Mantid::Kernel::VMD &basisVector) const {
  return std::fabs(oldVector.scalar_prod(basisVector)) > 0.0;
}

/**
 * Get indices which have a projection contribution
 * @param basisVector: the vector under investigation
 * @param oldBasis: the old basis vectors
 * @returns the indices of vectors onto which the basisVector projects
 */
std::vector<size_t> SlicingAlgorithm::getIndicesWithProjection(const Mantid::Kernel::VMD &basisVector,
                                                               const std::vector<Mantid::Kernel::VMD> &oldBasis) const {
  std::vector<size_t> indexWithProjection;
  for (size_t index = 0; index < oldBasis.size(); ++index) {
    if (isProjectingOnFrame(oldBasis[index], basisVector)) {
      indexWithProjection.emplace_back(index);
    }
  }
  return indexWithProjection;
}

/**
 * Extract the MDFrame. Make sure that all MDFrames are compatible -- if not
 * throw
 * @param indicesWithProjection: list of indices of dimensions which have a
 * projection
 * @param units: the units
 */
Mantid::Geometry::MDFrame_uptr
SlicingAlgorithm::extractMDFrameForNonAxisAligned(std::vector<size_t> indicesWithProjection,
                                                  const std::string &units) const {
  if (indicesWithProjection.empty()) {
    g_log.warning() << "Slicing Algorithm: Chosen vector does not "
                       "project on any vector of the old basis.";
  }
  // Get a reference frame to perform pairwise comparison
  const auto &referenceMDFrame = m_inWS->getDimension(indicesWithProjection[0])->getMDFrame();

  for (auto &index : indicesWithProjection) {
    const auto &toCheckMDFrame = m_inWS->getDimension(index)->getMDFrame();
    if (!referenceMDFrame.isSameType(toCheckMDFrame)) {
      g_log.warning() << "Slicing Algorithm: New basis vector tries to "
                         "mix un-mixable MDFrame types.";
    }
  }

  Mantid::Geometry::MDFrame_uptr mdFrame(referenceMDFrame.clone());
  setTargetUnits(mdFrame, units);

  return mdFrame;
}

/*
 * Set units of the output workspace
 * @param mdFrame: MDFrame to be added to the output workspace
 * @param unit: the unit to use in mdFrame
 */
void SlicingAlgorithm::setTargetUnits(Mantid::Geometry::MDFrame_uptr &mdFrame, const std::string &unit) const {
  boost::regex pattern("in.*A.*\\^-1");

  if (boost::regex_match(unit, pattern)) {
    // RLU with special label
    auto md_unit = ReciprocalLatticeUnit(unit);
    mdFrame->setMDUnit(md_unit);
  } else if (unit == "r") {
    // RLU
    auto md_unit = ReciprocalLatticeUnit();
    mdFrame->setMDUnit(md_unit);
  } else if (unit == "a") {
    // Inverse angstroms
    auto md_unit = InverseAngstromsUnit();
    mdFrame->setMDUnit(md_unit);
  }
  // else leave the unit the same as the input workspace
}

} // namespace Mantid::MDAlgorithms
