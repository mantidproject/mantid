// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidMDAlgorithms/SmoothMD.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/IMDHistoWorkspace.h"
#include "MantidAPI/IMDIterator.h"
#include "MantidAPI/Progress.h"
#include "MantidDataObjects/MDHistoWorkspaceIterator.h"
#include "MantidKernel/ArrayBoundedValidator.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidKernel/MultiThreaded.h"
#include "MantidKernel/PropertyWithValue.h"
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/make_shared.hpp>
#include <boost/tuple/tuple.hpp>
#include <limits>
#include <map>
#include <numeric>
#include <sstream>
#include <stack>
#include <string>
#include <utility>
#include <vector>

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;

// Typedef for width vector
using WidthVector = std::vector<double>;

// Typedef for kernel vector
using KernelVector = std::vector<double>;

// Typedef for an optional md histo workspace
using OptionalIMDHistoWorkspace_const_sptr =
    boost::optional<IMDHistoWorkspace_const_sptr>;

// Typedef for a smoothing function
using SmoothFunction = boost::function<IMDHistoWorkspace_sptr(
    IMDHistoWorkspace_const_sptr, const WidthVector &,
    OptionalIMDHistoWorkspace_const_sptr)>;

// Typedef for a smoothing function map keyed by name.
using SmoothFunctionMap = std::map<std::string, SmoothFunction>;

namespace {

/**
 * Maps a function name to a smoothing function
 * @return function map
 */
SmoothFunctionMap makeFunctionMap(Mantid::MDAlgorithms::SmoothMD *instance) {
  return {
      {"Hat", boost::bind(&Mantid::MDAlgorithms::SmoothMD::hatSmooth, instance,
                          _1, _2, _3)},
      {"Gaussian", boost::bind(&Mantid::MDAlgorithms::SmoothMD::gaussianSmooth,
                               instance, _1, _2, _3)}};
}
} // namespace

namespace Mantid {
namespace MDAlgorithms {

/*
 * Create a Gaussian kernel. The returned kernel is a 1D vector,
 * the order of which matches the linear indices returned by
 * the findNeighbourIndexesByWidth1D method.
 * @param fwhm : Full Width Half Maximum of the Gaussian (in units of pixels)
 * @return The Gaussian kernel
 */
KernelVector gaussianKernel(const double fwhm) {

  // Calculate sigma from FWHM
  // FWHM = 2 sqrt(2 * ln(2)) * sigma
  const double sigma = (fwhm * 0.42463) / 2.0;
  const double sigma_factor = M_SQRT1_2 / (fwhm * 0.42463);

  KernelVector kernel_one_side;
  // Start from centre and calculate values going outwards until value < 0.02
  // We have to truncate the function at some point and 0.02 is chosen
  // for consistency with Horace
  // Use erf to get the value of the Gaussian integrated over the width of the
  // pixel, more accurate than just using centre value of pixel and erf is fast
  double pixel_value = std::erf(0.5 * sigma_factor) * sigma;
  int pixel_count = 0;
  while (pixel_value > 0.02) {
    kernel_one_side.push_back(pixel_value);
    pixel_count++;
    pixel_value = (std::erf((pixel_count + 0.5) * sigma_factor) -
                   std::erf((pixel_count - 0.5) * sigma_factor)) *
                  0.5 * sigma;
  }

  // Create the symmetric kernel vector
  KernelVector kernel;
  kernel.resize(kernel_one_side.size() * 2 - 1);
  std::reverse_copy(kernel_one_side.cbegin(), kernel_one_side.cend(),
                    kernel.begin());
  std::copy(kernel_one_side.cbegin() + 1, kernel_one_side.cend(),
            kernel.begin() + kernel_one_side.size());

  return normaliseKernel(kernel);
}

/*
 * Normalise the kernel
 * It is necessary to renormlise where the kernel overlaps edges of the
 * workspace
 * The contributing elements should sum to unity
 */
KernelVector renormaliseKernel(KernelVector kernel,
                               const std::vector<bool> &validity) {

  if (validity.size() == kernel.size() &&
      std::find(validity.cbegin(), validity.cend(), false) != validity.cend()) {
    // Use validity as a mask
    for (size_t i = 0; i < kernel.size(); ++i) {
      kernel[i] *= validity[i];
    }

    kernel = normaliseKernel(kernel);
  }

  return kernel;
}

/*
 * Normalise the kernel so that sum of valid elements is unity
 */
KernelVector normaliseKernel(KernelVector kernel) {
  double sum_kernel_recip =
      1.0 / std::accumulate(kernel.cbegin(), kernel.cend(), 0.0);
  for (auto &pixel : kernel) {
    pixel *= sum_kernel_recip;
  }

  return kernel;
}

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(SmoothMD)

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string SmoothMD::name() const { return "SmoothMD"; }

/// Algorithm's version for identification. @see Algorithm::version
int SmoothMD::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string SmoothMD::category() const {
  return "MDAlgorithms\\Transforms";
}

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string SmoothMD::summary() const {
  return "Smooth an MDHistoWorkspace according to a weight function";
}

/**
 * Hat function smoothing. All weights even. Hat function boundaries beyond
 * width.
 * @param toSmooth : Workspace to smooth
 * @param widthVector : Width vector
 * @param weightingWS : Weighting workspace (optional)
 * @return Smoothed MDHistoWorkspace
 */
IMDHistoWorkspace_sptr
SmoothMD::hatSmooth(IMDHistoWorkspace_const_sptr toSmooth,
                    const WidthVector &widthVector,
                    OptionalIMDHistoWorkspace_const_sptr weightingWS) {

  const bool useWeights = weightingWS.is_initialized();
  uint64_t nPoints = toSmooth->getNPoints();
  Progress progress(this, 0.0, 1.0, size_t(double(nPoints) * 1.1));
  // Create the output workspace.
  IMDHistoWorkspace_sptr outWS(toSmooth->clone());
  progress.reportIncrement(
      size_t(double(nPoints) * 0.1)); // Report ~10% progress

  const int nThreads = Mantid::API::FrameworkManager::Instance()
                           .getNumOMPThreads(); // NThreads to Request

  auto iterators = toSmooth->createIterators(nThreads, nullptr);

  PARALLEL_FOR_NO_WSP_CHECK()
  for (int it = 0; it < int(iterators.size()); ++it) { // NOLINT

    PARALLEL_START_INTERUPT_REGION
    auto iterator =
        dynamic_cast<MDHistoWorkspaceIterator *>(iterators[it].get());

    if (!iterator) {
      throw std::logic_error(
          "Failed to cast IMDIterator to MDHistoWorkspaceIterator");
    }

    do {
      // Gets all vertex-touching neighbours
      size_t iteratorIndex = iterator->getLinearIndex();

      if (useWeights) {

        // Check that we could measure here.
        if ((*weightingWS)->getSignalAt(iteratorIndex) == 0) {

          outWS->setSignalAt(iteratorIndex,
                             std::numeric_limits<double>::quiet_NaN());
          outWS->setErrorSquaredAt(iteratorIndex,
                                   std::numeric_limits<double>::quiet_NaN());

          continue; // Skip we couldn't measure here.
        }
      }

      // Explicitly cast the doubles to int
      // We've already checked in the validator that the doubles we have are odd
      // integer values and well below max int
      std::vector<int> widthVectorInt;
      widthVectorInt.reserve(widthVector.size());
      std::copy(widthVector.cbegin(), widthVector.cend(),
                std::back_inserter(widthVectorInt));

      std::vector<size_t> neighbourIndexes =
          iterator->findNeighbourIndexesByWidth(widthVectorInt);

      size_t nNeighbours = neighbourIndexes.size();
      double sumSignal = iterator->getSignal();
      double sumSqError = iterator->getError();
      for (auto neighbourIndex : neighbourIndexes) {
        if (useWeights) {
          if ((*weightingWS)->getSignalAt(neighbourIndex) == 0) {
            // Nothing measured here. We cannot use that neighbouring point.
            nNeighbours -= 1;
            continue;
          }
        }
        sumSignal += toSmooth->getSignalAt(neighbourIndex);
        double error = toSmooth->getErrorAt(neighbourIndex);
        sumSqError += (error * error);
      }

      // Calculate the mean
      outWS->setSignalAt(iteratorIndex, sumSignal / double(nNeighbours + 1));
      // Calculate the sample variance
      outWS->setErrorSquaredAt(iteratorIndex,
                               sumSqError / double(nNeighbours + 1));

      progress.report();

    } while (iterator->next());
    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION

  return outWS;
}

/**
 * Gaussian function smoothing.
 * The Gaussian function is linearly separable, allowing convolution
 * of a multidimensional Gaussian kernel with the workspace to be carried out by
 * a convolution with a 1D Gaussian kernel in each dimension. This
 * reduces the number of calculations overall.
 * @param toSmooth : Workspace to smooth
 * @param widthVector : Width vector
 * @param weightingWS : Weighting workspace (optional)
 * @return Smoothed MDHistoWorkspace
 */
IMDHistoWorkspace_sptr
SmoothMD::gaussianSmooth(IMDHistoWorkspace_const_sptr toSmooth,
                         const WidthVector &widthVector,
                         OptionalIMDHistoWorkspace_const_sptr weightingWS) {

  const bool useWeights = weightingWS.is_initialized();
  uint64_t nPoints = toSmooth->getNPoints();
  Progress progress(this, 0.0, 1.0, size_t(double(nPoints) * 1.1));
  // Create the output workspace
  IMDHistoWorkspace_sptr outWS(toSmooth->clone().release());
  // Create a temporary workspace
  IMDHistoWorkspace_sptr tempWS(toSmooth->clone().release());
  // Report ~10% progress
  progress.reportIncrement(size_t(double(nPoints) * 0.1));

  // Create a kernel for each dimension and
  std::vector<KernelVector> gaussian_kernels;
  gaussian_kernels.reserve(widthVector.size());
  for (const auto width : widthVector) {
    gaussian_kernels.emplace_back(gaussianKernel(width));
  }

  const int nThreads = Mantid::API::FrameworkManager::Instance()
                           .getNumOMPThreads(); // NThreads to Request

  auto write_ws = tempWS;
  for (size_t dimension_number = 0; dimension_number < widthVector.size();
       ++dimension_number) {

    auto iterators = toSmooth->createIterators(nThreads, nullptr);

    // Alternately write to each workspace
    IMDHistoWorkspace_sptr read_ws;
    if (dimension_number % 2 == 0) {
      read_ws = outWS;
      write_ws = tempWS;
    } else {
      read_ws = tempWS;
      write_ws = outWS;
    }

    PARALLEL_FOR_NO_WSP_CHECK()
    for (int it = 0; it < int(iterators.size()); ++it) { // NOLINT

      PARALLEL_START_INTERUPT_REGION
      auto iterator =
          dynamic_cast<MDHistoWorkspaceIterator *>(iterators[it].get());
      if (!iterator) {
        throw std::logic_error(
            "Failed to cast IMDIterator to MDHistoWorkspaceIterator");
      }

      do {

        // Gets linear index at current position
        size_t iteratorIndex = iterator->getLinearIndex();

        if (useWeights) {

          // Check that we could measure here.
          if ((*weightingWS)->getSignalAt(iteratorIndex) == 0) {

            write_ws->setSignalAt(iteratorIndex,
                                  std::numeric_limits<double>::quiet_NaN());
            write_ws->setErrorSquaredAt(
                iteratorIndex, std::numeric_limits<double>::quiet_NaN());

            continue; // Skip we couldn't measure here.
          }
        }

        std::pair<std::vector<size_t>, std::vector<bool>> indexesAndValidity =
            iterator->findNeighbourIndexesByWidth1D(
                static_cast<int>(gaussian_kernels[dimension_number].size()),
                static_cast<int>(dimension_number));
        std::vector<size_t> neighbourIndexes = std::get<0>(indexesAndValidity);
        std::vector<bool> indexValidity = std::get<1>(indexesAndValidity);
        auto normalised_kernel = renormaliseKernel(
            gaussian_kernels[dimension_number], indexValidity);

        // Convolve signal with kernel
        double sumSignal = 0;
        double sumSquareError = 0;
        double error = 0;
        for (size_t i = 0; i < neighbourIndexes.size(); ++i) {
          if (indexValidity[i]) {
            sumSignal += read_ws->getSignalAt(neighbourIndexes[i]) *
                         normalised_kernel[i];
            error =
                read_ws->getErrorAt(neighbourIndexes[i]) * normalised_kernel[i];
            sumSquareError += error * error;
          }

          write_ws->setSignalAt(iteratorIndex, sumSignal);
          write_ws->setErrorSquaredAt(iteratorIndex, sumSquareError);
        }
        progress.report();

      } while (iterator->next());
      PARALLEL_END_INTERUPT_REGION
    }
    PARALLEL_CHECK_INTERUPT_REGION
  }

  return write_ws;
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void SmoothMD::init() {
  declareProperty(make_unique<WorkspaceProperty<API::IMDHistoWorkspace>>(
                      "InputWorkspace", "", Direction::Input),
                  "An input MDHistoWorkspace to smooth.");

  auto widthVectorValidator = boost::make_shared<CompositeValidator>();
  auto boundedValidator =
      boost::make_shared<ArrayBoundedValidator<double>>(1, 1000);
  widthVectorValidator->add(boundedValidator);
  widthVectorValidator->add(
      boost::make_shared<MandatoryValidator<WidthVector>>());

  declareProperty(
      make_unique<ArrayProperty<double>>("WidthVector", widthVectorValidator,
                                         Direction::Input),
      "Width vector. Either specify the width in n-pixels for each "
      "dimension, or provide a single entry (n-pixels) for all "
      "dimensions. Must be odd integers if Hat function is chosen.");

  const std::array<std::string, 2> allFunctionTypes = {{"Hat", "Gaussian"}};
  const std::string first = allFunctionTypes.front();

  std::stringstream docBuffer;
  docBuffer << "Smoothing function. Defaults to " << first;
  declareProperty(
      Kernel::make_unique<PropertyWithValue<std::string>>(
          "Function", first,
          boost::make_shared<ListValidator<std::string>>(allFunctionTypes),
          Direction::Input),
      docBuffer.str());

  std::array<std::string, 1> unitOptions = {{"pixels"}};

  std::stringstream docUnits;
  docUnits << "The units that WidthVector has been specified in. Allowed "
              "values are: ";
  for (auto const &unitOption : unitOptions) {
    docUnits << unitOption << ", ";
  }
  declareProperty(
      Kernel::make_unique<PropertyWithValue<std::string>>(
          "Units", "pixels",
          boost::make_shared<ListValidator<std::string>>(unitOptions),
          Direction::Input),
      docUnits.str());

  declareProperty(make_unique<WorkspaceProperty<API::IMDHistoWorkspace>>(
                      "InputNormalizationWorkspace", "", Direction::Input,
                      PropertyMode::Optional),
                  "Multidimensional weighting workspace. Optional.");

  declareProperty(make_unique<WorkspaceProperty<API::IMDHistoWorkspace>>(
                      "OutputWorkspace", "", Direction::Output),
                  "An output smoothed MDHistoWorkspace.");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void SmoothMD::exec() {

  // Get the input workspace to smooth
  IMDHistoWorkspace_sptr toSmooth = this->getProperty("InputWorkspace");

  // Get the input weighting workspace
  IMDHistoWorkspace_sptr weightingWS =
      this->getProperty("InputNormalizationWorkspace");
  OptionalIMDHistoWorkspace_const_sptr optionalWeightingWS;
  if (weightingWS) {
    optionalWeightingWS = weightingWS;
  }

  // Get the width vector
  std::vector<double> widthVector = this->getProperty("WidthVector");
  if (widthVector.size() == 1) {
    // Pad the width vector out to the right size if only one entry has been
    // provided.
    widthVector =
        std::vector<double>(toSmooth->getNumDims(), widthVector.front());
  }

  // Find the chosen smooth operation
  const std::string smoothFunctionName = this->getProperty("Function");
  SmoothFunctionMap functionMap = makeFunctionMap(this);
  SmoothFunction smoothFunction = functionMap[smoothFunctionName];
  // invoke the smoothing operation
  auto smoothed = smoothFunction(toSmooth, widthVector, optionalWeightingWS);

  setProperty("OutputWorkspace", smoothed);
}

/**
 * validateInputs
 * @return map of property names to errors.
 */
std::map<std::string, std::string> SmoothMD::validateInputs() {

  std::map<std::string, std::string> product;

  IMDHistoWorkspace_sptr toSmoothWs = this->getProperty("InputWorkspace");

  // Function type
  std::string function_type = this->getProperty("Function");

  // Check the width vector
  const std::string widthVectorPropertyName = "WidthVector";
  std::vector<double> widthVector = this->getProperty(widthVectorPropertyName);

  if (widthVector.size() != 1 &&
      widthVector.size() != toSmoothWs->getNumDims()) {
    product.emplace(widthVectorPropertyName,
                    widthVectorPropertyName +
                        " can either have one entry or needs to "
                        "have entries for each dimension of the "
                        "InputWorkspace.");
  } else if (function_type == "Hat") {
    // If Hat function is used then widthVector must contain odd integers only
    for (auto const widthEntry : widthVector) {
      double intpart;
      if (modf(widthEntry, &intpart) != 0.0) {
        std::stringstream message;
        message << widthVectorPropertyName
                << " entries must be (odd) integers "
                   "when Hat function is chosen. "
                   "Bad entry is "
                << widthEntry;
        product.emplace(widthVectorPropertyName, message.str());
      } else if (static_cast<unsigned long>(widthEntry) % 2 == 0) {
        std::stringstream message;
        message << widthVectorPropertyName
                << " entries must be odd integers "
                   "when Hat function is chosen. "
                   "Bad entry is "
                << widthEntry;
      }
    }
  }

  // Check the dimensionality of the normalization workspace
  const std::string normalisationWorkspacePropertyName =
      "InputNormalizationWorkspace";

  IMDHistoWorkspace_sptr normWs =
      this->getProperty(normalisationWorkspacePropertyName);
  if (normWs) {
    const size_t nDimsNorm = normWs->getNumDims();
    const size_t nDimsSmooth = toSmoothWs->getNumDims();
    if (nDimsNorm != nDimsSmooth) {
      std::stringstream message;
      message << normalisationWorkspacePropertyName
              << " has a different number of dimensions than InputWorkspace. "
                 "Shapes of inputs must be the same. Cannot continue "
                 "smoothing.";
      product.insert(
          std::make_pair(normalisationWorkspacePropertyName, message.str()));
    } else {
      // Loop over dimensions and check nbins.
      for (size_t i = 0; i < nDimsNorm; ++i) {
        const size_t nBinsNorm = normWs->getDimension(i)->getNBins();
        const size_t nBinsSmooth = toSmoothWs->getDimension(i)->getNBins();
        if (nBinsNorm != nBinsSmooth) {
          std::stringstream message;
          message << normalisationWorkspacePropertyName
                  << ". Number of bins from dimension with index " << i
                  << " do not match. " << nBinsSmooth << " expected. Got "
                  << nBinsNorm
                  << ". Shapes of inputs must be the same. Cannot "
                     "continue smoothing.";
          product.emplace(normalisationWorkspacePropertyName, message.str());
          break;
        }
      }
    }
  }

  return product;
}

} // namespace MDAlgorithms
} // namespace Mantid
