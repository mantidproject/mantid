// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidGeometry/Instrument.h"

namespace Mantid {
namespace DataObjects {

/** An SpecialWorkspace2D is a specialized Workspace2D where
 * the Y value at each pixel will be used for a special meaning.
 * Specifically, by GroupingWorkspace, MaskWorkspace and
 * OffsetsWorkspace.
 *
 * When created from an instrument, the workspace has a single pixel per detector, and this cannot
 * be changed.  When created from a matrix workspace, the workspace will have a single pixel
 * per source-workspace spectrum, and will share any instrument information with the source workspace.
 */

class BinaryOperator {
public:
  enum e { AND, OR, XOR, NOT };
};

class MANTID_DATAOBJECTS_DLL SpecialWorkspace2D : public Workspace2D {
public:
  SpecialWorkspace2D() = default;
  SpecialWorkspace2D(const Geometry::Instrument_const_sptr &inst, const bool includeMonitors = false);
  SpecialWorkspace2D(const API::MatrixWorkspace_const_sptr &parent);

  /// Returns a clone of the workspace
  std::unique_ptr<SpecialWorkspace2D> clone() const { return std::unique_ptr<SpecialWorkspace2D>(doClone()); }
  /// Returns a default-initialized clone of the workspace
  std::unique_ptr<SpecialWorkspace2D> cloneEmpty() const {
    auto workspace = std::unique_ptr<SpecialWorkspace2D>(doCloneEmpty());
    workspace->detID_to_WI = this->detID_to_WI;
    return workspace;
  }
  SpecialWorkspace2D &operator=(const SpecialWorkspace2D &) = delete;
  /** Gets the name of the workspace type
  @return Standard string name  */
  const std::string id() const override { return "SpecialWorkspace2D"; }

  bool isDetectorIDMappingEmpty() const { return detID_to_WI.empty(); }
  void buildDetectorIDMapping();
  double getValue(const detid_t detectorID) const;
  double getValue(const detid_t detectorID, const double defaultValue) const;

  void setValue(const detid_t detectorID, const double value, const double error = 0.);
  void setValue(const std::set<detid_t> &detectorIDs, const double value, const double error = 0.);

  std::set<detid_t> getDetectorIDs(const std::size_t workspaceIndex) const;

  void binaryOperation(const std::shared_ptr<const SpecialWorkspace2D> &ws, const unsigned int operatortype);
  void binaryOperation(const unsigned int operatortype);

  virtual void copyFrom(std::shared_ptr<const SpecialWorkspace2D> sourcews);

private:
  SpecialWorkspace2D *doClone() const override { return new SpecialWorkspace2D(*this); }
  SpecialWorkspace2D *doCloneEmpty() const override { return new SpecialWorkspace2D(); }
  bool isCompatible(const std::shared_ptr<const SpecialWorkspace2D> &ws);

protected:
  /// Protected copy constructor. May be used by childs for cloning.
  SpecialWorkspace2D(const SpecialWorkspace2D &) = default;

  void init(const size_t &NVectors, const size_t &XLength, const size_t &YLength) override;
  void init(const HistogramData::Histogram &histogram) override;

  /// Return human-readable string
  const std::string toString() const override;

  void binaryAND(const std::shared_ptr<const SpecialWorkspace2D> &ws);
  void binaryOR(const std::shared_ptr<const SpecialWorkspace2D> &ws);
  void binaryXOR(const std::shared_ptr<const SpecialWorkspace2D> &ws);
  void binaryNOT();

  /// Map with key = detector ID, and value = workspace index.
  std::map<detid_t, std::size_t> detID_to_WI;
};

/// shared pointer to the SpecialWorkspace2D class
using SpecialWorkspace2D_sptr = std::shared_ptr<SpecialWorkspace2D>;

/// shared pointer to a const SpecialWorkspace2D
using SpecialWorkspace2D_const_sptr = std::shared_ptr<const SpecialWorkspace2D>;

} // namespace DataObjects
} // namespace Mantid
