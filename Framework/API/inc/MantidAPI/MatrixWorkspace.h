#ifndef MANTID_API_MATRIXWORKSPACE_H_
#define MANTID_API_MATRIXWORKSPACE_H_

#include "MantidAPI/DllConfig.h"
#include "MantidAPI/ExperimentInfo.h"
#include "MantidAPI/IMDWorkspace.h"
#include "MantidAPI/ISpectrum.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidKernel/EmptyValues.h"

#include <atomic>
#include <mutex>

namespace Mantid {

namespace Indexing {
class IndexInfo;
}

namespace Types {
namespace Core {
class DateAndTime;
}
} // namespace Types

namespace Geometry {
class ParameterMap;
}

namespace API {
class Axis;
class SpectrumDetectorMapping;

/// typedef for the image type
using MantidImage = std::vector<std::vector<double>>;
/// shared pointer to MantidImage
using MantidImage_sptr = boost::shared_ptr<MantidImage>;
/// shared pointer to const MantidImage
using MantidImage_const_sptr = boost::shared_ptr<const MantidImage>;

//----------------------------------------------------------------------
/** Base MatrixWorkspace Abstract Class.

@author Laurent C Chapon, ISIS, RAL
@date 26/09/2007

Copyright &copy; 2007-2010 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
National Laboratory & European Spallation Source

This file is part of Mantid.

Mantid is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

Mantid is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

File change history is stored at: <https://github.com/mantidproject/mantid>.
Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class MANTID_API_DLL MatrixWorkspace : public IMDWorkspace,
                                       public ExperimentInfo {

private:
  using ExperimentInfo::toString;

public:
  // The Workspace Factory create-from-parent method needs direct access to the
  // axes.
  friend class WorkspaceFactoryImpl;

  void initialize(const std::size_t &NVectors, const std::size_t &XLength,
                  const std::size_t &YLength);
  void initialize(const std::size_t &NVectors,
                  const HistogramData::Histogram &histogram);
  void initialize(const Indexing::IndexInfo &indexInfo,
                  const HistogramData::Histogram &histogram);

  MatrixWorkspace &operator=(const MatrixWorkspace &other) = delete;
  /// Delete
  ~MatrixWorkspace() override;

  /// Returns a clone of the workspace
  MatrixWorkspace_uptr clone() const { return MatrixWorkspace_uptr(doClone()); }

  /// Returns a default-initialized clone of the workspace
  MatrixWorkspace_uptr cloneEmpty() const {
    return MatrixWorkspace_uptr(doCloneEmpty());
  }

  const Indexing::IndexInfo &indexInfo() const;
  void setIndexInfo(const Indexing::IndexInfo &indexInfo);

  using IMDWorkspace::toString;
  /// String description of state
  const std::string toString() const override;

  /**@name Instrument queries */
  //@{
  boost::shared_ptr<const Geometry::IDetector>
  getDetector(const size_t workspaceIndex) const;
  double detectorTwoTheta(const Geometry::IDetector &det) const;
  double detectorSignedTwoTheta(const Geometry::IDetector &det) const;

  //@}

  virtual void updateSpectraUsing(const SpectrumDetectorMapping &map);
  /// Build the default spectra mapping, most likely wanted after an instrument
  /// update
  void rebuildSpectraMapping(const bool includeMonitors = true);

  // More mapping
  spec2index_map getSpectrumToWorkspaceIndexMap() const;
  detid2index_map
  getDetectorIDToWorkspaceIndexMap(bool throwIfMultipleDets = false) const;
  virtual std::vector<size_t>
  getDetectorIDToWorkspaceIndexVector(detid_t &offset,
                                      bool throwIfMultipleDets = false) const;

  virtual std::vector<size_t>
  getSpectrumToWorkspaceIndexVector(specnum_t &offset) const;
  std::vector<size_t>
  getIndicesFromSpectra(const std::vector<specnum_t> &spectraList) const;
  size_t getIndexFromSpectrumNumber(const specnum_t specNo) const;
  std::vector<size_t>
  getIndicesFromDetectorIDs(const std::vector<detid_t> &detIdList) const;
  std::vector<specnum_t>
  getSpectraFromDetectorIDs(const std::vector<detid_t> &detIdList) const;

  bool hasGroupedDetectors() const;

  /// Get the footprint in memory in bytes.
  size_t getMemorySize() const override;
  virtual size_t getMemorySizeForXAxes() const;

  // Section required for iteration
  /// Returns the number of single indexable items in the workspace
  virtual std::size_t size() const = 0;
  /// Returns the size of each block of data returned by the dataY accessors.
  /// This throws an exception if the lengths are not identical across the
  /// spectra.
  virtual std::size_t blocksize() const = 0;
  /// Returns the number of histograms in the workspace
  virtual std::size_t getNumberHistograms() const = 0;

  /// Sets MatrixWorkspace title
  void setTitle(const std::string &) override;
  /// Gets MatrixWorkspace title (same as Run object run_title property)
  const std::string getTitle() const override;

  virtual Types::Core::DateAndTime getFirstPulseTime() const;
  Types::Core::DateAndTime getLastPulseTime() const;

  /// Returns the bin index for a given X value of a given workspace index
  size_t binIndexOf(const double xValue, const std::size_t = 0) const;

  //----------------------------------------------------------------------
  // DATA ACCESSORS
  //----------------------------------------------------------------------

  /// Return the underlying ISpectrum ptr at the given workspace index.
  virtual ISpectrum &getSpectrum(const size_t index) = 0;

  /// Return the underlying ISpectrum ptr (const version) at the given workspace
  /// index.
  virtual const ISpectrum &getSpectrum(const size_t index) const = 0;

  /// Returns the Histogram at the given workspace index.
  HistogramData::Histogram histogram(const size_t index) const {
    return getSpectrum(index).histogram();
  }
  template <typename... T>
  void setHistogram(const size_t index, T &&... data) & {
    getSpectrum(index).setHistogram(std::forward<T>(data)...);
  }
  void convertToCounts(const size_t index) {
    getSpectrum(index).convertToCounts();
  }
  void convertToFrequencies(const size_t index) {
    getSpectrum(index).convertToFrequencies();
  }
  HistogramData::BinEdges binEdges(const size_t index) const {
    return getSpectrum(index).binEdges();
  }
  HistogramData::Points points(const size_t index) const {
    return getSpectrum(index).points();
  }
  HistogramData::PointStandardDeviations
  pointStandardDeviations(const size_t index) const {
    return getSpectrum(index).pointStandardDeviations();
  }
  template <typename... T>
  void setBinEdges(const size_t index, T &&... data) & {
    getSpectrum(index).setBinEdges(std::forward<T>(data)...);
  }
  template <typename... T> void setPoints(const size_t index, T &&... data) & {
    getSpectrum(index).setPoints(std::forward<T>(data)...);
  }
  template <typename... T>
  void setPointVariances(const size_t index, T &&... data) & {
    getSpectrum(index).setPointVariances(std::forward<T>(data)...);
  }
  template <typename... T>
  void setPointStandardDeviations(const size_t index, T &&... data) & {
    getSpectrum(index).setPointStandardDeviations(std::forward<T>(data)...);
  }
  HistogramData::Counts counts(const size_t index) const {
    return getSpectrum(index).counts();
  }
  HistogramData::CountVariances countVariances(const size_t index) const {
    return getSpectrum(index).countVariances();
  }
  HistogramData::CountStandardDeviations
  countStandardDeviations(const size_t index) const {
    return getSpectrum(index).countStandardDeviations();
  }
  HistogramData::Frequencies frequencies(const size_t index) const {
    return getSpectrum(index).frequencies();
  }
  HistogramData::FrequencyVariances
  frequencyVariances(const size_t index) const {
    return getSpectrum(index).frequencyVariances();
  }
  HistogramData::FrequencyStandardDeviations
  frequencyStandardDeviations(const size_t index) const {
    return getSpectrum(index).frequencyStandardDeviations();
  }
  template <typename... T> void setCounts(const size_t index, T &&... data) & {
    getSpectrum(index).setCounts(std::forward<T>(data)...);
  }
  template <typename... T>
  void setCountVariances(const size_t index, T &&... data) & {
    getSpectrum(index).setCountVariances(std::forward<T>(data)...);
  }
  template <typename... T>
  void setCountStandardDeviations(const size_t index, T &&... data) & {
    getSpectrum(index).setCountStandardDeviations(std::forward<T>(data)...);
  }
  template <typename... T>
  void setFrequencies(const size_t index, T &&... data) & {
    getSpectrum(index).setFrequencies(std::forward<T>(data)...);
  }
  template <typename... T>
  void setFrequencyVariances(const size_t index, T &&... data) & {
    getSpectrum(index).setFrequencyVariances(std::forward<T>(data)...);
  }
  template <typename... T>
  void setFrequencyStandardDeviations(const size_t index, T &&... data) & {
    getSpectrum(index).setFrequencyStandardDeviations(std::forward<T>(data)...);
  }
  const HistogramData::HistogramX &x(const size_t index) const {
    return getSpectrum(index).x();
  }
  const HistogramData::HistogramY &y(const size_t index) const {
    return getSpectrum(index).y();
  }
  const HistogramData::HistogramE &e(const size_t index) const {
    return getSpectrum(index).e();
  }
  const HistogramData::HistogramDx &dx(const size_t index) const {
    return getSpectrum(index).dx();
  }
  HistogramData::HistogramX &mutableX(const size_t index) & {
    return getSpectrum(index).mutableX();
  }
  HistogramData::HistogramDx &mutableDx(const size_t index) & {
    return getSpectrum(index).mutableDx();
  }
  HistogramData::HistogramY &mutableY(const size_t index) & {
    return getSpectrum(index).mutableY();
  }
  HistogramData::HistogramE &mutableE(const size_t index) & {
    return getSpectrum(index).mutableE();
  }
  Kernel::cow_ptr<HistogramData::HistogramX> sharedX(const size_t index) const {
    return getSpectrum(index).sharedX();
  }
  Kernel::cow_ptr<HistogramData::HistogramY> sharedY(const size_t index) const {
    return getSpectrum(index).sharedY();
  }
  Kernel::cow_ptr<HistogramData::HistogramE> sharedE(const size_t index) const {
    return getSpectrum(index).sharedE();
  }
  Kernel::cow_ptr<HistogramData::HistogramDx>
  sharedDx(const size_t index) const {
    return getSpectrum(index).sharedDx();
  }
  void setSharedX(const size_t index,
                  const Kernel::cow_ptr<HistogramData::HistogramX> &x) & {
    getSpectrum(index).setSharedX(x);
  }
  void setSharedDx(const size_t index,
                   const Kernel::cow_ptr<HistogramData::HistogramDx> &dx) & {
    getSpectrum(index).setSharedDx(dx);
  }
  void setSharedY(const size_t index,
                  const Kernel::cow_ptr<HistogramData::HistogramY> &y) & {
    getSpectrum(index).setSharedY(y);
  }
  void setSharedE(const size_t index,
                  const Kernel::cow_ptr<HistogramData::HistogramE> &e) & {
    getSpectrum(index).setSharedE(e);
  }
  // Methods for getting read-only access to the data.
  // Just passes through to the virtual dataX/Y/E function (const version)
  /// Deprecated, use x() instead. Returns a read-only (i.e. const) reference to
  /// the specified X array
  /// @param index :: workspace index to retrieve.
  const MantidVec &readX(std::size_t const index) const {
    return getSpectrum(index).dataX();
  }
  /// Deprecated, use y() instead. Returns a read-only (i.e. const) reference to
  /// the specified Y array
  /// @param index :: workspace index to retrieve.
  const MantidVec &readY(std::size_t const index) const {
    return getSpectrum(index).dataY();
  }
  /// Deprecated, use e() instead. Returns a read-only (i.e. const) reference to
  /// the specified E array
  /// @param index :: workspace index to retrieve.
  const MantidVec &readE(std::size_t const index) const {
    return getSpectrum(index).dataE();
  }
  /// Deprecated, use dx() instead. Returns a read-only (i.e. const) reference
  /// to the specified X error array
  /// @param index :: workspace index to retrieve.
  const MantidVec &readDx(size_t const index) const {
    return getSpectrum(index).dataDx();
  }

  /// Deprecated, use mutableX() instead. Returns the x data
  virtual MantidVec &dataX(const std::size_t index) {
    invalidateCommonBinsFlag();
    return getSpectrum(index).dataX();
  }
  /// Deprecated, use mutableY() instead. Returns the y data
  virtual MantidVec &dataY(const std::size_t index) {
    return getSpectrum(index).dataY();
  }
  /// Deprecated, use mutableE() instead. Returns the error data
  virtual MantidVec &dataE(const std::size_t index) {
    return getSpectrum(index).dataE();
  }
  /// Deprecated, use mutableDx() instead. Returns the x error data
  virtual MantidVec &dataDx(const std::size_t index) {
    return getSpectrum(index).dataDx();
  }

  /// Deprecated, use x() instead. Returns the x data const
  virtual const MantidVec &dataX(const std::size_t index) const {
    return getSpectrum(index).dataX();
  }
  /// Deprecated, use y() instead. Returns the y data const
  virtual const MantidVec &dataY(const std::size_t index) const {
    return getSpectrum(index).dataY();
  }
  /// Deprecated, use e() instead. Returns the error const
  virtual const MantidVec &dataE(const std::size_t index) const {
    return getSpectrum(index).dataE();
  }
  /// Deprecated, use dx() instead. Returns the error const
  virtual const MantidVec &dataDx(const std::size_t index) const {
    return getSpectrum(index).dataDx();
  }

  virtual double getXMin() const;
  virtual double getXMax() const;
  virtual void getXMinMax(double &xmin, double &xmax) const;

  /// Deprecated, use sharedX() instead. Returns a pointer to the x data
  virtual Kernel::cow_ptr<HistogramData::HistogramX>
  refX(const std::size_t index) const {
    return getSpectrum(index).ptrX();
  }

  /// Deprecated, use setSharedX() instead. Set the specified X array to point
  /// to the given existing array
  virtual void setX(const std::size_t index,
                    const Kernel::cow_ptr<HistogramData::HistogramX> &X) {
    getSpectrum(index).setX(X);
    invalidateCommonBinsFlag();
  }

  /// Deprecated, use setSharedX() instead. Set the specified X array to point
  /// to the given existing array
  virtual void setX(const std::size_t index,
                    const boost::shared_ptr<HistogramData::HistogramX> &X) {
    getSpectrum(index).setX(X);
    invalidateCommonBinsFlag();
  }

  /**
   * Probes if DX (X Error) values were set on a particular spectrum
   * @param index: the workspace index
   */
  virtual bool hasDx(const std::size_t index) const {
    return getSpectrum(index).hasDx();
  }

  /// Generate the histogram or rebin the existing histogram.
  virtual void generateHistogram(const std::size_t index, const MantidVec &X,
                                 MantidVec &Y, MantidVec &E,
                                 bool skipError = false) const = 0;

  /// Return a vector with the integrated counts for all spectra withing the
  /// given range
  virtual void getIntegratedSpectra(std::vector<double> &out, const double minX,
                                    const double maxX,
                                    const bool entireRange) const;

  /// Return an index in the X vector for an x-value close to a given value
  std::pair<size_t, double> getXIndex(size_t i, double x, bool isLeft = true,
                                      size_t start = 0) const;

  //----------------------------------------------------------------------

  int axes() const;
  virtual Axis *getAxis(const std::size_t &axisIndex) const;
  void replaceAxis(const std::size_t &axisIndex, Axis *const newAxis);

  /// Returns true if the workspace contains data in histogram form (as opposed
  /// to point-like)
  virtual bool isHistogramData() const;

  /// Returns true if the workspace contains has common X bins
  virtual bool isCommonBins() const;

  std::string YUnit() const;
  void setYUnit(const std::string &newUnit);
  std::string YUnitLabel() const;
  void setYUnitLabel(const std::string &newLabel);

  /// Are the Y-values dimensioned?
  bool isDistribution() const;
  void setDistribution(bool newValue);

  // Methods to set and access masked bins
  void maskBin(const size_t &workspaceIndex, const size_t &binIndex,
               const double &weight = 1.0);
  void flagMasked(const size_t &index, const size_t &binIndex,
                  const double &weight = 1.0);
  bool hasMaskedBins(const size_t &workspaceIndex) const;
  /// Masked bins for each spectrum are stored as a set of pairs containing <bin
  /// index, weight>
  using MaskList = std::map<size_t, double>;
  const MaskList &maskedBins(const size_t &workspaceIndex) const;
  void setMaskedBins(const size_t workspaceIndex, const MaskList &maskedBins);

  // Methods handling the internal monitor workspace
  virtual void
  setMonitorWorkspace(const boost::shared_ptr<MatrixWorkspace> &monitorWS);
  boost::shared_ptr<MatrixWorkspace> monitorWorkspace() const;

  void saveInstrumentNexus(::NeXus::File *file) const;
  void loadInstrumentNexus(::NeXus::File *file);

  //=====================================================================================
  // MD Geometry methods
  //=====================================================================================
  size_t getNumDims() const override;
  boost::shared_ptr<const Mantid::Geometry::IMDDimension>
  getDimension(size_t index) const override;
  boost::shared_ptr<const Mantid::Geometry::IMDDimension>
  getDimensionWithId(std::string id) const override;
  //=====================================================================================
  // End MD Geometry methods
  //=====================================================================================

  //=====================================================================================
  // IMDWorkspace methods
  //=====================================================================================

  /// Gets the number of points available on the workspace.
  uint64_t getNPoints() const override;
  /// Get the number of points available on the workspace.
  uint64_t getNEvents() const override { return this->getNPoints(); }
  /// Dimension id for x-dimension.
  static const std::string xDimensionId;
  /// Dimensin id for y-dimension.
  static const std::string yDimensionId;
  /// Generate a line plot through the matrix workspace.
  LinePlot getLinePlot(const Mantid::Kernel::VMD &start,
                       const Mantid::Kernel::VMD &end,
                       Mantid::API::MDNormalization normalize) const override;
  /// Get the signal at a coordinate in the workspace.
  signal_t getSignalAtCoord(
      const coord_t *coords,
      const Mantid::API::MDNormalization &normalization) const override;
  /// Get the signal at a coordinate in the workspace
  signal_t getSignalWithMaskAtCoord(
      const coord_t *coords,
      const Mantid::API::MDNormalization &normalization) const override;
  /// Create iterators. Partitions the iterators according to the number of
  /// cores.
  std::vector<std::unique_ptr<IMDIterator>> createIterators(
      size_t suggestedNumCores = 1,
      Mantid::Geometry::MDImplicitFunction *function = nullptr) const override;

  /// Apply masking.
  void
  setMDMasking(Mantid::Geometry::MDImplicitFunction *maskingRegion) override;
  /// Clear exsting masking.
  void clearMDMasking() override;

  /// @return the special coordinate system used if any.
  Mantid::Kernel::SpecialCoordinateSystem
  getSpecialCoordinateSystem() const override;

  // Check if this class has an oriented lattice on a sample object
  virtual bool hasOrientedLattice() const override;

  //=====================================================================================
  // End IMDWorkspace methods
  //=====================================================================================

  //=====================================================================================
  // Image methods
  //=====================================================================================

  /// Get start and end x indices for images
  std::pair<size_t, size_t> getImageStartEndXIndices(size_t i, double startX,
                                                     double endX) const;
  /// Create an image of Ys.
  MantidImage_sptr getImageY(size_t start = 0, size_t stop = 0,
                             size_t width = 0, double startX = EMPTY_DBL(),
                             double endX = EMPTY_DBL()) const;
  /// Create an image of Es.
  MantidImage_sptr getImageE(size_t start = 0, size_t stop = 0,
                             size_t width = 0, double startX = EMPTY_DBL(),
                             double endX = EMPTY_DBL()) const;
  /// Copy the data (Y's) from an image to this workspace.
  virtual void setImageY(const MantidImage &image, size_t start = 0,
                         bool parallelExecution = true);
  /// Copy the data from an image to this workspace's errors.
  virtual void setImageE(const MantidImage &image, size_t start = 0,
                         bool parallelExecution = true);

  //=====================================================================================
  // End image methods
  //=====================================================================================

  void invalidateCachedSpectrumNumbers();

  void cacheDetectorGroupings(const det2group_map &mapping) override;
  size_t groupOfDetectorID(const detid_t detID) const override;

protected:
  /// Protected copy constructor. May be used by childs for cloning.
  MatrixWorkspace(const MatrixWorkspace &other);

  MatrixWorkspace(
      const Parallel::StorageMode storageMode = Parallel::StorageMode::Cloned);

  /// Initialises the workspace. Sets the size and lengths of the arrays. Must
  /// be overloaded.
  virtual void init(const std::size_t &NVectors, const std::size_t &XLength,
                    const std::size_t &YLength) = 0;
  virtual void init(const HistogramData::Histogram &histogram) = 0;

  /// Invalidates the commons bins flag.  This is generally called when a method
  /// could allow the X values to be changed.
  void invalidateCommonBinsFlag() { m_isCommonBinsFlagSet = false; }

  void updateCachedDetectorGrouping(const size_t index) const override;

  /// A vector of pointers to the axes for this workspace
  std::vector<Axis *> m_axes;

private:
  MatrixWorkspace *doClone() const override = 0;
  MatrixWorkspace *doCloneEmpty() const override = 0;

  /// Create an MantidImage instance.
  MantidImage_sptr
  getImage(const MantidVec &(MatrixWorkspace::*read)(std::size_t const) const,
           size_t start, size_t stop, size_t width, size_t indexStart,
           size_t indexEnd) const;
  /// Copy data from an image.
  void setImage(MantidVec &(MatrixWorkspace::*dataVec)(const std::size_t),
                const MantidImage &image, size_t start, bool parallelExecution);

  void setIndexInfoWithoutISpectrumUpdate(const Indexing::IndexInfo &indexInfo);
  void buildDefaultSpectrumDefinitions();
  void rebuildDetectorIDGroupings();

  std::unique_ptr<Indexing::IndexInfo> m_indexInfo;

  /// Has this workspace been initialised?
  bool m_isInitialized{false};

  /// The unit for the data values (e.g. Counts)
  std::string m_YUnit;
  /// A text label for use when plotting spectra
  std::string m_YUnitLabel;

  /// Flag indicating whether the m_isCommonBinsFlag has been set. False by
  /// default
  mutable bool m_isCommonBinsFlagSet{false};
  /// Flag indicating whether the data has common bins. False by default
  mutable bool m_isCommonBinsFlag{false};

  /// The set of masked bins in a map keyed on workspace index
  std::map<int64_t, MaskList> m_masks;

  /// A workspace holding monitor data relating to the main data in the
  /// containing workspace (null if none).
  boost::shared_ptr<MatrixWorkspace> m_monitorWorkspace;

  mutable std::atomic<bool> m_indexInfoNeedsUpdate{true};
  mutable std::mutex m_indexInfoMutex;

protected:
  /// Getter for the dimension id based on the axis.
  std::string getDimensionIdFromAxis(const int &axisIndex) const;
};

/// shared pointer to the matrix workspace base class
using MatrixWorkspace_sptr = boost::shared_ptr<MatrixWorkspace>;
/// shared pointer to the matrix workspace base class (const version)
using MatrixWorkspace_const_sptr = boost::shared_ptr<const MatrixWorkspace>;

} // namespace API
} // namespace Mantid

#endif /*MANTID_API_MATRIXWORKSPACE_H_*/
