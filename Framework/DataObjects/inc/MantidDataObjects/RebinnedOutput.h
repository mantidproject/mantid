// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_DATAOBJECTS_REBINNEDOUTPUT_H_
#define MANTID_DATAOBJECTS_REBINNEDOUTPUT_H_

#include "MantidAPI/ISpectrum.h"
#include "MantidDataObjects/Workspace2D.h"

namespace Mantid {
namespace DataObjects {

/** RebinnedOutput

  This class will handle the needs for 2D fractional overlap rebinning.
  The rebinning method requires the separate tracking of counts and
  fractional area. The workspace will always present the correct data to a
  2D display. Integration and rebinning will be handled via the fundamental
  algorithms.

  @date 2012-04-05
*/
class DLLExport RebinnedOutput : public Workspace2D {
public:
  RebinnedOutput() : m_finalized(false), m_hasSqrdErrs(true) {}
  RebinnedOutput(bool finalized, bool hasSqrdErrs)
      : m_finalized(finalized), m_hasSqrdErrs(hasSqrdErrs) {}
  /// Returns a clone of the workspace
  std::unique_ptr<RebinnedOutput> clone() const {
    return std::unique_ptr<RebinnedOutput>(doClone());
  }
  /// Returns a default-initialized clone of the workspace
  std::unique_ptr<RebinnedOutput> cloneEmpty() const {
    return std::unique_ptr<RebinnedOutput>(doCloneEmpty());
  }
  RebinnedOutput &operator=(const RebinnedOutput &) = delete;

  /// Get the workspace ID.
  const std::string id() const override;

  /// Returns the fractional area
  virtual MantidVec &dataF(const std::size_t index);

  /// Returns the fractional area
  virtual const MantidVec &dataF(const std::size_t index) const;

  /// Finalize to fractional area representation
  void finalize(bool hasSqrdErrs = true);
  /// Undo fractional area representation
  void unfinalize();

  /// Returns if finalize has been called
  bool isFinalized() const { return m_finalized; }
  /// Override the finalized flag
  void setFinalized(bool value) { m_finalized = value; }

  /// Returns if using squared errors
  bool hasSqrdErrors() const { return m_hasSqrdErrs; }
  /// Override the squared errors flag
  void setSqrdErrors(bool value) { m_hasSqrdErrs = value; }

  /// Returns a read-only (i.e. const) reference to the specified F array
  const MantidVec &readF(std::size_t const index) const;

  /// Set the fractional area array for a given index.
  void setF(const std::size_t index, const MantidVecPtr &F);
  /// Multiply the fractional area arrays by a scale factor
  void scaleF(const double scale);
  /// Returns if the fractional area is non zero
  bool nonZeroF() const;

protected:
  /// Protected copy constructor. May be used by childs for cloning.
  RebinnedOutput(const RebinnedOutput &) = default;

  /// Called by initialize() in MatrixWorkspace
  void init(const std::size_t &NVectors, const std::size_t &XLength,
            const std::size_t &YLength) override;
  void init(const HistogramData::Histogram &histogram) override;

  /// A vector that holds the 1D vectors for the fractional area.
  std::vector<MantidVec> fracArea;

  /// Flag to indicate if finalize has been called, and if errors/variance used
  bool m_finalized;

  /// Flag to indiciate if the finalized data used squared errors
  bool m_hasSqrdErrs;

private:
  RebinnedOutput *doClone() const override { return new RebinnedOutput(*this); }
  RebinnedOutput *doCloneEmpty() const override {
    return new RebinnedOutput(m_finalized, m_hasSqrdErrs);
  }
};

/// shared pointer to the RebinnedOutput class
using RebinnedOutput_sptr = boost::shared_ptr<RebinnedOutput>;
/// shared pointer to a const RebinnedOutput
using RebinnedOutput_const_sptr = boost::shared_ptr<const RebinnedOutput>;

} // namespace DataObjects
} // namespace Mantid

#endif /* MANTID_DATAOBJECTS_REBINNEDOUTPUT_H_ */
