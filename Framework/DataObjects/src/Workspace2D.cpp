#include "MantidDataObjects/Workspace2D.h"
#include "MantidAPI/ISpectrum.h"
#include "MantidAPI/RefAxis.h"
#include "MantidAPI/SpectraAxis.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidHistogramData/LinearGenerator.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/IPropertyManager.h"
#include "MantidKernel/VectorHelper.h"

#include <algorithm>
#include <sstream>

using Mantid::API::MantidImage;

namespace Mantid {
namespace DataObjects {
using std::size_t;

DECLARE_WORKSPACE(Workspace2D)

/// Constructor
Workspace2D::Workspace2D(const Parallel::StorageMode storageMode)
    : HistoWorkspace(storageMode) {}

Workspace2D::Workspace2D(const Workspace2D &other)
    : HistoWorkspace(other), m_monitorList(other.m_monitorList) {
  data.resize(other.data.size());
  for (size_t i = 0; i < data.size(); ++i) {
    data[i] = new Histogram1D(*(other.data[i]));
  }
}

/// Destructor
Workspace2D::~Workspace2D() {
// On MSVC when you allocate memory in a multithreaded loop, like our cow_ptrs
// will do, the
// deallocation time increases by a huge amount if the memory is just
// naively deallocated in a serial order. This is because when it was
// allocated in the omp loop then the actual memory ends up being
// interleaved and then trying to deallocate this serially leads to
// lots of swapping in and out of memory. See
// http://social.msdn.microsoft.com/Forums/en-US/2fe4cfc7-ca5c-4665-8026-42e0ba634214/visual-studio-$

#ifdef _MSC_VER
  PARALLEL_FOR_IF(Kernel::threadSafe(*this))
  for (int64_t i = 0; i < static_cast<int64_t>(data.size()); i++) {
#else
  for (size_t i = 0; i < data.size(); ++i) {
#endif
    // Clear out the memory
    delete data[i];
  }
}

/**
 * Sets the size of the workspace and initializes arrays to zero
 *
 * @param NVectors :: The number of vectors/histograms/detectors in the
 * workspace
 *
 * @param XLength :: The number of X data points/bin boundaries in each vector
 * (must all be the same)
 *
 * @param YLength :: The number of data/error points in each vector
 * (must all be the same)
 */
void Workspace2D::init(const std::size_t &NVectors, const std::size_t &XLength,
                       const std::size_t &YLength) {
  data.resize(NVectors);

  auto x = Kernel::make_cow<HistogramData::HistogramX>(
      XLength, HistogramData::LinearGenerator(1.0, 1.0));
  HistogramData::Counts y(YLength);
  HistogramData::CountStandardDeviations e(YLength);
  Histogram1D spec(HistogramData::getHistogramXMode(XLength, YLength),
                   HistogramData::Histogram::YMode::Counts);
  spec.setX(x);
  spec.setCounts(y);
  spec.setCountStandardDeviations(e);
  for (size_t i = 0; i < data.size(); i++) {
    data[i] = new Histogram1D(spec);
    // Default spectrum number = starts at 1, for workspace index 0.
    data[i]->setSpectrumNo(specnum_t(i + 1));
  }

  // Add axes that reference the data
  m_axes.resize(2);
  m_axes[0] = new API::RefAxis(this);
  m_axes[1] = new API::SpectraAxis(this);
}

void Workspace2D::init(const HistogramData::Histogram &histogram) {
  data.resize(numberOfDetectorGroups());

  HistogramData::Histogram initializedHistogram(histogram);
  if (!histogram.sharedY()) {
    if (histogram.yMode() == HistogramData::Histogram::YMode::Frequencies) {
      initializedHistogram.setFrequencies(histogram.size(), 0.0);
      initializedHistogram.setFrequencyStandardDeviations(histogram.size(),
                                                          0.0);
    } else { // YMode::Counts or YMode::Uninitialized -> default to Counts
      initializedHistogram.setCounts(histogram.size(), 0.0);
      initializedHistogram.setCountStandardDeviations(histogram.size(), 0.0);
    }
  }

  Histogram1D spec(initializedHistogram.xMode(), initializedHistogram.yMode());
  spec.setHistogram(initializedHistogram);
  for (auto &i : data) {
    i = new Histogram1D(spec);
  }

  // Add axes that reference the data
  m_axes.resize(2);
  m_axes[0] = new API::RefAxis(this);
  m_axes[1] = new API::SpectraAxis(this);
}

/** Gets the number of histograms
@return Integer
*/
size_t Workspace2D::getNumberHistograms() const {
  return getHistogramNumberHelper();
}

/// get pseudo size
size_t Workspace2D::size() const {
  return std::accumulate(data.begin(), data.end(), static_cast<size_t>(0),
                         [](const size_t value, const Histogram1D *histo) {
                           return value + histo->size();
                         });
}

/// get the size of each vector
size_t Workspace2D::blocksize() const {
  if (data.empty()) {
    return 0;
  } else {
    size_t numBins = data[0]->size();
    for (const auto *iter : data)
      if (numBins != iter->size())
        throw std::length_error(
            "blocksize undefined because size of histograms is not equal");
    return numBins;
  }
}

/**
 * Copy the data (Y's) from an image to this workspace.
 * @param image :: An image to copy the data from.
 * @param start :: Startinf workspace indx to copy data to.
 * @param parallelExecution :: Should inner loop run as parallel operation
 */
void Workspace2D::setImageY(const MantidImage &image, size_t start,
                            bool parallelExecution) {
  MantidImage m;
  setImageYAndE(image, m, start, parallelExecution);
}

/**
 * Copy the data from an image to this workspace's errors.
 * @param image :: An image to copy the data from.
 * @param start :: Startinf workspace indx to copy data to.
 * @param parallelExecution :: Should inner loop run as parallel operation
 */
void Workspace2D::setImageE(const MantidImage &image, size_t start,
                            bool parallelExecution) {
  MantidImage m;
  setImageYAndE(m, image, start, parallelExecution);
}

/**
 * Copy the data from an image to the (Y's) and the errors for this
 * workspace.
 *
 * @param imageY :: An image to copy the data from.
 * @param imageE :: An image to copy the errors from.
 * @param start :: Startinf workspace indx to copy data to.
 *
 * @param loadAsRectImg :: load using one histogram per row and one
 * bin per column, instead of the default one histogram per pixel
 *
 * @param scale_1 :: scale factor for the X axis (norammly
 * representing the inverse of the pixel width or similar.
 *
 * @param parallelExecution :: Should inner loop run as parallel operation
 */
void Workspace2D::setImageYAndE(const API::MantidImage &imageY,
                                const API::MantidImage &imageE, size_t start,
                                bool loadAsRectImg, double scale_1,
                                bool parallelExecution) {
  UNUSED_ARG(parallelExecution) // for parallel for

  if (imageY.empty() && imageE.empty())
    return;
  if (imageY.empty() && imageE[0].empty())
    return;
  if (imageE.empty() && imageY[0].empty())
    return;

  const size_t numBins = blocksize();
  if (!loadAsRectImg && numBins != 1) {
    throw std::runtime_error(
        "Cannot set image in workspace: a single bin workspace is "
        "required when initializing a workspace from an "
        "image using a histogram per pixel.");
  }

  size_t height;
  size_t width;
  if (!imageY.empty()) {
    height = imageY.size();
    width = imageY.front().size();
  } else {
    height = imageE.size();
    width = imageE.front().size();
  }
  size_t dataSize = width * height;

  if (start + dataSize > getNumberHistograms() * numBins) {
    throw std::runtime_error(
        "Cannot set image: image is bigger than workspace.");
  }

  if (!loadAsRectImg) {
    // 1 pixel - one spectrum
    PARALLEL_FOR_IF(parallelExecution)
    for (int i = 0; i < static_cast<int>(height); ++i) {

      const auto &rowY = imageY[i];
      const auto &rowE = imageE[i];
      size_t spec = start + static_cast<size_t>(i) * width;
      auto pE = rowE.begin();
      for (auto pY = rowY.begin(); pY != rowY.end() && pE != rowE.end();
           ++pY, ++pE, ++spec) {
        data[spec]->dataY()[0] = *pY;
        data[spec]->dataE()[0] = *pE;
      }
    }
  } else {

    if (height != (getNumberHistograms()))
      throw std::runtime_error(
          std::string("To load an image into a workspace with one spectrum per "
                      "row, then number of spectra (") +
          std::to_string(getNumberHistograms()) +
          ") needs to be equal to the height (rows) of the image (" +
          std::to_string(height) + ")");

    if (width != numBins)
      throw std::runtime_error(
          std::string("To load an image into a workspace with one spectrum per "
                      "row, then number of bins (") +
          std::to_string(numBins) +
          ") needs to be equal to the width (columns) of the image (" +
          std::to_string(width) + ")");

    // one spectrum - one row
    PARALLEL_FOR_IF(parallelExecution)
    for (int i = 0; i < static_cast<int>(height); ++i) {

      const auto &rowY = imageY[i];
      const auto &rowE = imageE[i];
      data[i]->dataY() = rowY;
      data[i]->dataE() = rowE;
    }
    // X values. Set first spectrum and copy/propagate that one to all the other
    // spectra
    PARALLEL_FOR_IF(parallelExecution)
    for (int i = 0; i < static_cast<int>(width) + 1; ++i) {
      data[0]->dataX()[i] = i * scale_1;
    }
    PARALLEL_FOR_IF(parallelExecution)
    for (int i = 1; i < static_cast<int>(height); ++i) {
      data[i]->setX(data[0]->ptrX());
    }
  }
}

/// Return reference to Histogram1D at the given workspace index.
Histogram1D &Workspace2D::getSpectrum(const size_t index) {
  invalidateCommonBinsFlag();
  auto &spec = const_cast<Histogram1D &>(
      static_cast<const Workspace2D &>(*this).getSpectrum(index));
  spec.setMatrixWorkspace(this, index);
  return spec;
}

/// Return const reference to Histogram1D at the given workspace index.
const Histogram1D &Workspace2D::getSpectrum(const size_t index) const {
  if (index >= data.size()) {
    std::ostringstream ss;
    ss << "Workspace2D::getSpectrum, histogram number " << index
       << " out of range " << data.size();
    throw std::range_error(ss.str());
  }
  return *data[index];
}

//--------------------------------------------------------------------------------------------
/** Returns the number of histograms.
 *  For some reason Visual Studio couldn't deal with the main
 * getHistogramNumber() method
 *  being virtual so it now just calls this private (and virtual) method which
 * does the work.
 *  @return the number of histograms associated with the workspace
 */
size_t Workspace2D::getHistogramNumberHelper() const { return data.size(); }

//---------------------------------------------------------------------------
/** Rebin a particular spectrum to a new histogram bin boundaries.
 *
 * @param index :: workspace index to generate
 * @param X :: input X vector of the bin boundaries.
 * @param Y :: output vector to be filled with the Y data.
 * @param E :: output vector to be filled with the Error data (optionally)
 * @param skipError :: if true, the error vector is NOT calculated.
 *        CURRENTLY IGNORED, the Error is always calculated.
 */
void Workspace2D::generateHistogram(const std::size_t index, const MantidVec &X,
                                    MantidVec &Y, MantidVec &E,
                                    bool skipError) const {
  UNUSED_ARG(skipError);
  if (index >= data.size())
    throw std::range_error(
        "Workspace2D::generateHistogram, histogram number out of range");
  // output data arrays are implicitly filled by function
  const auto &spec = this->getSpectrum(index);
  const MantidVec &currentX = spec.readX();
  const MantidVec &currentY = spec.readY();
  const MantidVec &currentE = spec.readE();
  if (X.size() <= 1)
    throw std::runtime_error(
        "Workspace2D::generateHistogram(): X vector must be at least length 2");
  Y.resize(X.size() - 1, 0);
  E.resize(X.size() - 1, 0);

  // Perform the rebin from the current bins to the new ones
  if (currentX.size() ==
      currentY.size()) // First, convert to bin boundaries if needed.  The
  {                    // VectorHelper::rebin, assumes bin boundaries, even if
    std::vector<double> histX; // it is a distribution!
    histX.resize(currentX.size() + 1);
    Mantid::Kernel::VectorHelper::convertToBinBoundary(currentX, histX);
    Mantid::Kernel::VectorHelper::rebin(histX, currentY, currentE, X, Y, E,
                                        true);
  } else // assume x_size = y_size + 1
  {
    Mantid::Kernel::VectorHelper::rebin(currentX, currentY, currentE, X, Y, E,
                                        this->isDistribution());
  }
}

Workspace2D *Workspace2D::doClone() const { return new Workspace2D(*this); }

Workspace2D *Workspace2D::doCloneEmpty() const {
  return new Workspace2D(storageMode());
}

} // namespace DataObjects
} // namespace Mantid

namespace Mantid {
namespace Kernel {
template <>
DLLExport Mantid::DataObjects::Workspace2D_sptr
IPropertyManager::getValue<Mantid::DataObjects::Workspace2D_sptr>(
    const std::string &name) const {
  PropertyWithValue<Mantid::DataObjects::Workspace2D_sptr> *prop =
      dynamic_cast<PropertyWithValue<Mantid::DataObjects::Workspace2D_sptr> *>(
          getPointerToProperty(name));
  if (prop) {
    return *prop;
  } else {
    std::string message =
        "Attempt to assign property " + name +
        " to incorrect type. Expected shared_ptr<Workspace2D>.";
    throw std::runtime_error(message);
  }
}

template <>
DLLExport Mantid::DataObjects::Workspace2D_const_sptr
IPropertyManager::getValue<Mantid::DataObjects::Workspace2D_const_sptr>(
    const std::string &name) const {
  PropertyWithValue<Mantid::DataObjects::Workspace2D_sptr> *prop =
      dynamic_cast<PropertyWithValue<Mantid::DataObjects::Workspace2D_sptr> *>(
          getPointerToProperty(name));
  if (prop) {
    return prop->operator()();
  } else {
    std::string message =
        "Attempt to assign property " + name +
        " to incorrect type. Expected const shared_ptr<Workspace2D>.";
    throw std::runtime_error(message);
  }
}
} // namespace Kernel
} // namespace Mantid

///\endcond TEMPLATE
