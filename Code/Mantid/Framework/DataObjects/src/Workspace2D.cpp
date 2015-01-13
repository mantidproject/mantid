#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/Exception.h"
#include "MantidAPI/RefAxis.h"
#include "MantidAPI/SpectraAxis.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/ISpectrum.h"
#include "MantidKernel/VectorHelper.h"

using Mantid::API::ISpectrum;
using Mantid::API::MantidImage;

namespace Mantid {
namespace DataObjects {
using std::size_t;

DECLARE_WORKSPACE(Workspace2D)

/// Constructor
Workspace2D::Workspace2D() {}

/// Destructor
Workspace2D::~Workspace2D() {
// Clear out the memory

// The omp loop is here primarily MSVC. On MSVC 2012
// when you allocate memory in a multithreaded loop, like our cow_ptrs will do,
// the deallocation time increases by a huge amount if the memory is just
// naively deallocated in a serial order. This is because when it was allocated
// in the omp loop then the actual memory ends up being interleaved and
// then trying to deallocate this serially leads to lots of swapping in and out
// of
// memory.
// See
// http://social.msdn.microsoft.com/Forums/en-US/2fe4cfc7-ca5c-4665-8026-42e0ba634214/visual-studio-2012-slow-deallocation-when-new-called-within-openmp-loop?forum=vcgeneral

#ifdef _MSC_VER
  PARALLEL_FOR1(this)
#endif
  for (int64_t i = 0; i < static_cast<int64_t>(data.size()); i++) {
    delete data[i];
  }
}

/** Sets the size of the workspace and initializes arrays to zero
*  @param NVectors :: The number of vectors/histograms/detectors in the
* workspace
*  @param XLength :: The number of X data points/bin boundaries in each vector
* (must all be the same)
*  @param YLength :: The number of data/error points in each vector (must all be
* the same)
*/
void Workspace2D::init(const std::size_t &NVectors, const std::size_t &XLength,
                       const std::size_t &YLength) {
  m_noVectors = NVectors;
  data.resize(m_noVectors);

  MantidVecPtr t1, t2;
  t1.access().resize(XLength); // this call initializes array to zero
  t2.access().resize(YLength);
  for (size_t i = 0; i < m_noVectors; i++) {
    // Create the spectrum upon init
    Histogram1D *spec = new Histogram1D();
    data[i] = spec;
    // Set the data and X
    spec->setX(t1);
    spec->setDx(t1);
    // Y,E arrays populated
    spec->setData(t2, t2);
    // Default spectrum number = starts at 1, for workspace index 0.
    spec->setSpectrumNo(specid_t(i + 1));
    spec->setDetectorID(detid_t(i + 1));
  }

  // Add axes that reference the data
  m_axes.resize(2);
  m_axes[0] = new API::RefAxis(XLength, this);
  m_axes[1] = new API::SpectraAxis(this);
}

/** Gets the number of histograms
@return Integer
*/
size_t Workspace2D::getNumberHistograms() const {
  return getHistogramNumberHelper();
}

/// get pseudo size
size_t Workspace2D::size() const { return data.size() * blocksize(); }

/// get the size of each vector
size_t Workspace2D::blocksize() const {
  return (data.size() > 0) ? data[0]->dataY().size() : 0;
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
  * Copy the data from an image to the (Y's) and the errors for this workspace.
  * @param imageY :: An image to copy the data from.
  * @param imageE :: An image to copy the errors from.
  * @param start :: Startinf workspace indx to copy data to.
  * @param parallelExecution :: Should inner loop run as parallel operation
  */

void Workspace2D::setImageYAndE(const API::MantidImage &imageY,
                                const API::MantidImage &imageE, size_t start,
                                bool parallelExecution) {
  if (imageY.empty() && imageE.empty())
    return;
  if (imageY.empty() && imageE[0].empty())
    return;
  if (imageE.empty() && imageY[0].empty())
    return;

  if (blocksize() != 1) {
    throw std::runtime_error(
        "Cannot set image: a single bin workspace is expected.");
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

  if (start + dataSize > getNumberHistograms()) {
    throw std::runtime_error(
        "Cannot set image: image is bigger than workspace.");
  }

  PARALLEL_FOR_IF(parallelExecution)
  for (int i = 0; i < static_cast<int>(height); ++i) {
    const auto &rowY = imageY[i];
    const auto &rowE = imageE[i];

    size_t spec = start + static_cast<size_t>(i) * width;
    auto rowYEnd = rowY.end();
    auto rowEEnd = rowE.end();

    auto pixelE = rowE.begin();
    for (auto pixelY = rowY.begin(); pixelY != rowYEnd && pixelE != rowEEnd;
         ++pixelY, ++pixelE, ++spec) {
      if (rowY.begin() != rowY.end())
        (*data[spec]).dataY()[0] = *pixelY;
      if (rowE.begin() != rowE.end())
        (*data[spec]).dataE()[0] = *pixelE;
    }
  }
}

//--------------------------------------------------------------------------------------------
/// Return the underlying ISpectrum ptr at the given workspace index.
ISpectrum *Workspace2D::getSpectrum(const size_t index) {
  if (index >= m_noVectors) {
    std::stringstream ss;
    ss << "Workspace2D::getSpectrum, histogram number " << index
       << " out of range " << m_noVectors;
    throw std::range_error(ss.str());
  }
  invalidateCommonBinsFlag();
  return data[index];
}

const ISpectrum *Workspace2D::getSpectrum(const size_t index) const {
  if (index >= m_noVectors) {
    std::stringstream ss;
    ss << "Workspace2D::getSpectrum, histogram number " << index
       << " out of range " << m_noVectors;
    throw std::range_error(ss.str());
  }
  return data[index];
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
  if (index >= this->m_noVectors)
    throw std::range_error(
        "Workspace2D::generateHistogram, histogram number out of range");
  // output data arrays are implicitly filled by function
  const ISpectrum *spec = this->getSpectrum(index);
  const MantidVec &currentX = spec->readX();
  const MantidVec &currentY = spec->readY();
  const MantidVec &currentE = spec->readE();
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
                                        this->isDistribution());
  } else // assume x_size = y_size + 1
  {
    Mantid::Kernel::VectorHelper::rebin(currentX, currentY, currentE, X, Y, E,
                                        this->isDistribution());
  }
}

} // namespace DataObjects
} // NamespaceMantid

///\cond TEMPLATE
template DLLExport class Mantid::API::WorkspaceProperty<
    Mantid::DataObjects::Workspace2D>;

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
    std::string message = "Attempt to assign property " + name +
                          " to incorrect type. Expected Workspace2D.";
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
    std::string message = "Attempt to assign property " + name +
                          " to incorrect type. Expected const Workspace2D.";
    throw std::runtime_error(message);
  }
}
} // namespace Kernel
} // namespace Mantid

///\endcond TEMPLATE
