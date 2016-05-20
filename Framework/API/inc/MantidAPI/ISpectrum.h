#ifndef MANTID_API_ISPECTRUM_H_
#define MANTID_API_ISPECTRUM_H_

#include "MantidKernel/System.h"
#include "MantidGeometry/IDTypes.h"
#include "MantidKernel/cow_ptr.h"
#include "MantidHistogramData/Histogram.h"

#include <set>

namespace Mantid {
namespace API {

/** A "spectrum" is an object that holds the data for a particular spectrum,
 * in particular:
 *  - The X/Y/E arrays
 *  - The spectrum number
 *  - A list of detector ids associated with it.
 *
 * This is an interface that can be used for both Workspace2D's Spectrum
 objects,
 * and EventWorkspace's EventList objects

  @author Janik Zikovsky
  @date 2011-07-01

  Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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

  File change history is stored at: <https://github.com/mantidproject/mantid>
  Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class DLLExport ISpectrum {
public:
  ISpectrum();
  ISpectrum(const specnum_t specNo);
  virtual ~ISpectrum() = default;

  void copyInfoFrom(const ISpectrum &other);

  virtual void setX(const Kernel::cow_ptr<HistogramData::HistogramX> &X) = 0;
  virtual MantidVec &dataX() = 0;
  virtual const MantidVec &dataX() const = 0;
  virtual const MantidVec &readX() const = 0;
  virtual Kernel::cow_ptr<HistogramData::HistogramX> ptrX() const = 0;

  virtual void setData(const MantidVec &Y) = 0;
  virtual void setData(const MantidVec &Y, const MantidVec &E) = 0;

  virtual void setData(const MantidVecPtr &Y) = 0;
  virtual void setData(const MantidVecPtr &Y, const MantidVecPtr &E) = 0;

  virtual void clearData() = 0;

  virtual MantidVec &dataY() = 0;
  virtual MantidVec &dataE() = 0;

  virtual const MantidVec &dataY() const = 0;
  virtual const MantidVec &dataE() const = 0;
  virtual const MantidVec &readY() const;
  virtual const MantidVec &readE() const;

  virtual size_t getMemorySize() const = 0;

  virtual std::pair<double, double> getXDataRange() const;
  // ---------------------------------------------------------
  void addDetectorID(const detid_t detID);
  void addDetectorIDs(const std::set<detid_t> &detIDs);
  void addDetectorIDs(const std::vector<detid_t> &detIDs);
  void setDetectorID(const detid_t detID);
  void setDetectorIDs(const std::set<detid_t> &detIDs);
  void setDetectorIDs(std::set<detid_t> &&detIDs);

  bool hasDetectorID(const detid_t detID) const;
  std::set<detid_t> &getDetectorIDs();
  const std::set<detid_t> &getDetectorIDs() const;

  void clearDetectorIDs();

  // ---------------------------------------------------------
  specnum_t getSpectrumNo() const;

  void setSpectrumNo(specnum_t num);

  // ---------------------------------------------------------
  virtual void lockData() const;
  virtual void unlockData() const;

  //-------------------------------------------------------
  bool hasDx() const;
  void resetHasDx();

  HistogramData::Histogram histogram() const { return histogramRef(); }

  HistogramData::BinEdges binEdges() const { return histogramRef().binEdges(); }
  HistogramData::BinEdgeStandardDeviations binEdgeStandardDeviations() const {
    return histogramRef().binEdgeStandardDeviations();
  }
  HistogramData::Points points() const { return histogramRef().binEdges(); }
  HistogramData::PointStandardDeviations pointStandardDeviations() const {
    return histogramRef().pointStandardDeviations();
  }
  template <typename... T> void setBinEdges(T &&... data) {
    mutableHistogramRef().setBinEdges(std::forward<T>(data)...);
  }
  template <typename... T> void setBinEdgeVariances(T &&... data) {
    mutableHistogramRef().setBinEdgeVariances(std::forward<T>(data)...);
  }
  template <typename... T> void setBinEdgeStandardDeviations(T &&... data) {
    mutableHistogramRef().setBinEdgeStandardDeviations(
        std::forward<T>(data)...);
  }
  template <typename... T> void setPoints(T &&... data) {
    mutableHistogramRef().setPoints(std::forward<T>(data)...);
  }
  template <typename... T> void setPointVariances(T &&... data) {
    mutableHistogramRef().setPointVariances(std::forward<T>(data)...);
  }
  template <typename... T> void setPointStandardDeviations(T &&... data) {
    mutableHistogramRef().setPointStandardDeviations(std::forward<T>(data)...);
  }
  const HistogramData::HistogramX &x() const { return histogramRef().x(); }
  const HistogramData::HistogramDx &dx() const { return histogramRef().dx(); }
  HistogramData::HistogramX &mutableX() {
    return mutableHistogramRef().mutableX();
  }
  HistogramData::HistogramDx &mutableDx() {
    return mutableHistogramRef().mutableDx();
  }
  Kernel::cow_ptr<HistogramData::HistogramX> sharedX() const {
    return histogramRef().sharedX();
  }
  Kernel::cow_ptr<HistogramData::HistogramDx> sharedDx() const {
    return histogramRef().sharedDx();
  }
  void setSharedX(const Kernel::cow_ptr<HistogramData::HistogramX> &x) {
    mutableHistogramRef().setSharedX(x);
  }
  void setSharedDx(const Kernel::cow_ptr<HistogramData::HistogramDx> &dx) {
    mutableHistogramRef().setSharedDx(dx);
  }

protected:
  /// The spectrum number of this spectrum
  specnum_t m_specNo;

  /// Set of the detector IDs associated with this spectrum
  std::set<detid_t> detectorIDs;

private:
  virtual const HistogramData::Histogram &histogramRef() const = 0;
  virtual HistogramData::Histogram &mutableHistogramRef() = 0;
};

} // namespace API
} // namespace Mantid

#endif /* MANTID_API_ISPECTRUM_H_ */
