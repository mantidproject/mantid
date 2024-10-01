// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Column.h"
#include "MantidDataObjects/LeanElasticPeak.h"
#include "MantidDataObjects/Peak.h"

#include <boost/variant.hpp>
#include <list>

namespace Mantid {
namespace DataObjects {

/** PeakColumn : a Column sub-class used to display
 * peak information as a TableWorkspace.
 *
 * The column holds a reference to a vector of Peak objects.
 * Values in the column are taken directly from those Peak objects.
 *
 * @author Janik Zikovsky
 * @date 2011-04-25 18:06:32.952258
 */
template <class T> class MANTID_DATAOBJECTS_DLL PeakColumn : public Mantid::API::Column {

public:
  /// Construct a column with a reference to the peaks list, a name & type
  PeakColumn(std::vector<T> &peaks, const std::string &name);

  /// Number of individual elements in the column.
  size_t size() const override { return m_peaks.size(); }

  /// Returns typeid for the data in the column
  const std::type_info &get_type_info() const override;

  /// Returns typeid for the pointer type to the data element in the column
  const std::type_info &get_pointer_type_info() const override;

  bool getReadOnly() const override;

  /// Prints
  void print(size_t index, std::ostream &s) const override;

  void read(size_t index, const std::string &text) override;

  /// Sets item from a stream
  void read(const size_t index, std::istringstream &in) override;

  /// Specialized type check
  bool isBool() const override;

  bool isNumber() const override;

  /// Must return overall memory size taken by the column.
  long int sizeOfData() const override;

  /// Clone.
  PeakColumn *clone() const override;

  /// Cast to double
  double toDouble(size_t i) const override;

  /// Assign from double
  void fromDouble(size_t i, double value) override;

  /// Reference to the data.
  const std::vector<T> &data() const { return m_peaks; }

  bool equals(const Column &, double, bool) const override {
    throw std::runtime_error("equals not implemented, to compare use CompareWorkspace");
  }

protected:
  /// Sets the new column size.
  void resize(size_t count) override;
  /// Inserts an item.
  void insert(size_t index) override;
  /// Removes an item.
  void remove(size_t index) override;
  /// Pointer to a data element
  void *void_pointer(size_t index) override;
  /// Pointer to a data element
  const void *void_pointer(size_t index) const override;

private:
  /// Reference to the peaks object saved in the PeaksWorkspace.
  std::vector<T> &m_peaks;
  /// Precision of hkl in table workspace
  int m_hklPrec;

  /// Type of the row cache value
  using CacheValueType = boost::variant<double, int, std::string, Kernel::V3D>;
  ///
  mutable std::list<CacheValueType> m_oldRows;
  /// Sets the correct value in the referenced peak.
  void setPeakHKLOrRunNumber(const size_t index, const double val);
};

} // namespace DataObjects
} // namespace Mantid
