// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_DATAOBJECTS_WORKSPACE2D_H_
#define MANTID_DATAOBJECTS_WORKSPACE2D_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/HistoWorkspace.h"
#include "MantidDataObjects/Histogram1D.h"

namespace Mantid {

namespace DataObjects {
/** \class Workspace2D

    Concrete workspace implementation. Data is a vector of Histogram1D.
    Since Histogram1D have share ownership of X, Y or E arrays,
    duplication is avoided for workspaces for example with identical time bins.

    \author Laurent C Chapon, ISIS, RAL
    \date 26/09/2007
*/
class DLLExport Workspace2D : public API::HistoWorkspace {
public:
  /**
  Gets the name of the workspace type
  @return Standard string name
   */
  const std::string id() const override { return "Workspace2D"; }

  Workspace2D(
      const Parallel::StorageMode storageMode = Parallel::StorageMode::Cloned);
  Workspace2D &operator=(const Workspace2D &other) = delete;
  ~Workspace2D() override;

  /// Returns a clone of the workspace
  std::unique_ptr<Workspace2D> clone() const {
    return std::unique_ptr<Workspace2D>(doClone());
  }

  /// Returns a default-initialized clone of the workspace
  std::unique_ptr<Workspace2D> cloneEmpty() const {
    return std::unique_ptr<Workspace2D>(doCloneEmpty());
  }

  /// Returns the histogram number
  std::size_t getNumberHistograms() const override;

  // section required for iteration
  std::size_t size() const override;
  std::size_t blocksize() const override;

  Histogram1D &getSpectrum(const size_t index) override {
    invalidateCommonBinsFlag();
    return getSpectrumWithoutInvalidation(index);
  }
  const Histogram1D &getSpectrum(const size_t index) const override;

  /// Generate a new histogram by rebinning the existing histogram.
  void generateHistogram(const std::size_t index, const MantidVec &X,
                         MantidVec &Y, MantidVec &E,
                         bool skipError = false) const override;

  /** sets the monitorWorkspace indexlist
    @param mList :: a vector holding the monitor workspace indexes
  */
  void setMonitorList(std::vector<specnum_t> &mList) { m_monitorList = mList; }

  /// Copy the data (Y's) from an image to this workspace.
  void setImageY(const API::MantidImage &image, size_t start = 0,
                 bool parallelExecution = true) override;
  /// Copy the data from an image to this workspace's errors.
  void setImageE(const API::MantidImage &image, size_t start = 0,
                 bool parallelExecution = true) override;
  /// Copy the data from an image to this workspace's (Y's) and errors.
  void setImageYAndE(const API::MantidImage &imageY,
                     const API::MantidImage &imageE, size_t start = 0,
                     bool loadAsRectImg = false, double scale_1 = 1.0,
                     bool parallelExecution = true);

protected:
  /// Protected copy constructor. May be used by childs for cloning.
  Workspace2D(const Workspace2D &other);

  /// Called by initialize()
  void init(const std::size_t &NVectors, const std::size_t &XLength,
            const std::size_t &YLength) override;
  void init(const HistogramData::Histogram &histogram) override;

  /// a vector holding workspace index of monitors in the workspace
  std::vector<specnum_t> m_monitorList;

  /// A vector that holds the 1D histograms
  std::vector<Histogram1D *> data;

private:
  Workspace2D *doClone() const override;
  Workspace2D *doCloneEmpty() const override;

  Histogram1D &getSpectrumWithoutInvalidation(const size_t index) override;
  virtual std::size_t getHistogramNumberHelper() const;
};

/// shared pointer to the Workspace2D class
using Workspace2D_sptr = boost::shared_ptr<Workspace2D>;
/// shared pointer to a const Workspace2D
using Workspace2D_const_sptr = boost::shared_ptr<const Workspace2D>;

} // namespace DataObjects
} // Namespace Mantid
#endif /*MANTID_DATAOBJECTS_WORKSPACE2D_H_*/
