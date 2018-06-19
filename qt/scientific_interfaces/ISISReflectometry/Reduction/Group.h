#ifndef MANTID_CUSTOMINTERFACES_GROUP_H_
#define MANTID_CUSTOMINTERFACES_GROUP_H_
#include <string>
#include <vector>
#include <boost/optional.hpp>
#include "Row.h"
#include "../DllConfig.h"
#include "WorkspaceNamesFactory.h"

namespace MantidQt {
namespace CustomInterfaces {

template <typename Row> class Group {
public:
  using RowType = Row;
  Group(std::string name);
  Group(std::string name, std::vector<boost::optional<Row>> rows);

  std::string const &name() const;
  void setName(std::string const &name);
  std::string postprocessedWorkspaceName(
      WorkspaceNamesFactory const &workspaceNamesFactory) const;

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

template <typename Row>
std::ostream &operator<<(std::ostream &os, Group<Row> const &group);

extern template class MANTIDQT_ISISREFLECTOMETRY_DLL Group<SlicedRow>;
using SlicedGroup = Group<SlicedRow>;
extern template MANTIDQT_ISISREFLECTOMETRY_DLL std::ostream &
operator<<(std::ostream &, SlicedGroup const &);

extern template class MANTIDQT_ISISREFLECTOMETRY_DLL Group<UnslicedRow>;
using UnslicedGroup = Group<UnslicedRow>;
extern template MANTIDQT_ISISREFLECTOMETRY_DLL std::ostream &
operator<<(std::ostream &, UnslicedGroup const &);

UnslicedGroup unslice(SlicedGroup const &slicedGroup,
                      WorkspaceNamesFactory const &workspaceNamesFactory);
SlicedGroup slice(UnslicedGroup const &unslicedGroup,
                  WorkspaceNamesFactory const &WorkspaceNamesFactory);
}
}
#endif // MANTID_CUSTOMINTERFACES_GROUP_H_
