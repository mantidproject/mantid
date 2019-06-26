// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_DATAOBJECTS_SPECIALWORKSPACE2D_H_
#define MANTID_DATAOBJECTS_SPECIALWORKSPACE2D_H_

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
 * The workspace has a single pixel per detector, and this cannot
 * be changed.
 *
 */

class BinaryOperator {
public:
  enum e { AND, OR, XOR, NOT };
};

class DLLExport SpecialWorkspace2D : public Workspace2D {
public:
  SpecialWorkspace2D() = default;
  SpecialWorkspace2D(Geometry::Instrument_const_sptr inst,
                     const bool includeMonitors = false);
  SpecialWorkspace2D(API::MatrixWorkspace_const_sptr parent);

  /// Returns a clone of the workspace
  std::unique_ptr<SpecialWorkspace2D> clone() const {
    return std::unique_ptr<SpecialWorkspace2D>(doClone());
  }
  /// Returns a default-initialized clone of the workspace
  std::unique_ptr<SpecialWorkspace2D> cloneEmpty() const {
    auto workspace = std::unique_ptr<SpecialWorkspace2D>(doCloneEmpty());
    workspace->setDetMap(this->detID_to_WI);
    return workspace;
  }
  SpecialWorkspace2D &operator=(const SpecialWorkspace2D &) = delete;
  /** Gets the name of the workspace type
  @return Standard string name  */
  const std::string id() const override { return "SpecialWorkspace2D"; }

  double getValue(const detid_t detectorID) const;
  double getValue(const detid_t detectorID, const double defaultValue) const;

  void setValue(const detid_t detectorID, const double value,
                const double error = 0.);
  void setValue(const std::set<detid_t> &detectorIDs, const double value,
                const double error = 0.);

  std::set<detid_t> getDetectorIDs(const std::size_t workspaceIndex) const;

  void binaryOperation(boost::shared_ptr<const SpecialWorkspace2D> &ws,
                       const unsigned int operatortype);
  void binaryOperation(const unsigned int operatortype);

  virtual void copyFrom(boost::shared_ptr<const SpecialWorkspace2D> sourcews);

  /** set a new detector map, for use in clone empty
   * @param map the map to set.
   */
  void setDetMap(std::map<detid_t, std::size_t> map){ detID_to_WI=map;}

private:
  SpecialWorkspace2D *doClone() const override {
    return new SpecialWorkspace2D(*this);
  }
  SpecialWorkspace2D *doCloneEmpty() const override {
    return new SpecialWorkspace2D();
  }
  bool isCompatible(boost::shared_ptr<const SpecialWorkspace2D> ws);

protected:
  /// Protected copy constructor. May be used by childs for cloning.
  SpecialWorkspace2D(const SpecialWorkspace2D &) = default;

  void init(const size_t &NVectors, const size_t &XLength,
            const size_t &YLength) override;
  void init(const HistogramData::Histogram &histogram) override;

  /// Return human-readable string
  const std::string toString() const override;

  void binaryAND(boost::shared_ptr<const SpecialWorkspace2D> ws);
  void binaryOR(boost::shared_ptr<const SpecialWorkspace2D> ws);
  void binaryXOR(boost::shared_ptr<const SpecialWorkspace2D> ws);
  void binaryNOT();

  /// Map with key = detector ID, and value = workspace index.
  std::map<detid_t, std::size_t> detID_to_WI;
};

/// shared pointer to the SpecialWorkspace2D class
using SpecialWorkspace2D_sptr = boost::shared_ptr<SpecialWorkspace2D>;

/// shared pointer to a const SpecialWorkspace2D
using SpecialWorkspace2D_const_sptr =
    boost::shared_ptr<const SpecialWorkspace2D>;

} // namespace DataObjects
} // namespace Mantid

#endif /* MANTID_DATAOBJECTS_SPECIALWORKSPACE2D_H_ */
