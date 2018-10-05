// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

#ifndef MANTID_CUSTOMINTERFACES_GROUP_H_
#define MANTID_CUSTOMINTERFACES_GROUP_H_
#include "../DllConfig.h"
#include "Row.h"
#include <boost/optional.hpp>
#include <string>
#include <vector>

namespace MantidQt {
namespace CustomInterfaces {

class MANTIDQT_ISISREFLECTOMETRY_DLL Group {
public:
  Group(std::string name);
  Group(std::string name, std::vector<boost::optional<Row>> rows);

  std::string const &name() const;
  void setName(std::string const &name);
  std::string postprocessedWorkspaceName() const;

  void appendEmptyRow();
  void appendRow(boost::optional<Row> const &row);
  void insertRow(boost::optional<Row> const &row, int beforeRowAtIndex);
  void removeRow(int rowIndex);
  void updateRow(int rowIndex, boost::optional<Row> const &row);
  bool allRowsAreValid() const;

  boost::optional<int> indexOfRowWithTheta(double angle,
                                           double tolerance) const;

  boost::optional<Row> const &operator[](int rowIndex) const;
  std::vector<boost::optional<Row>> const &rows() const;

private:
  std::string m_name;
  std::vector<boost::optional<Row>> m_rows;
};

template <typename ModificationListener>
void mergeRowsInto(Group &intoHere, Group const &fromHere, int groupIndex,
                   double thetaTolerance, ModificationListener &listener) {
  for (auto const &maybeRow : fromHere.rows()) {
    if (maybeRow.is_initialized()) {
      auto const &fromRow = maybeRow.get();
      auto index =
          intoHere.indexOfRowWithTheta(fromRow.theta(), thetaTolerance);
      if (index.is_initialized()) {
        auto const updateAtIndex = index.get();
        auto const &intoRow = intoHere[updateAtIndex].get();
        auto updatedRow = mergedRow(intoRow, fromRow);
        intoHere.updateRow(updateAtIndex, updatedRow);
        listener.rowModified(groupIndex, updateAtIndex, updatedRow);
      } else {
        intoHere.appendRow(maybeRow.get());
        listener.rowAppended(groupIndex,
                             static_cast<int>(intoHere.rows().size() - 1),
                             maybeRow.get());
      }
    }
  }
}

// std::ostream &operator<<(std::ostream &os, Group const &group) {
//  os << "  Group (name: " << group.name() << ")\n";
//  for (auto &&row : group.rows()) {
//    if (row.is_initialized())
//      os << "    " << row.get() << '\n';
//    else
//      os << "    Row (invalid)\n";
//  }
//  return os;
//}
} // namespace CustomInterfaces
} // namespace MantidQt
#endif // MANTID_CUSTOMINTERFACES_GROUP_H_
