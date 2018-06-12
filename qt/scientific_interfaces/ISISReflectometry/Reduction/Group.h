#ifndef MANTID_CUSTOMINTERFACES_GROUP_H_
#define MANTID_CUSTOMINTERFACES_GROUP_H_
#include <string>
#include <vector>
#include <boost/optional.hpp>
#include "Row.h"
#include "../DllConfig.h"

namespace MantidQt {
namespace CustomInterfaces {

template <typename Row> class Group {
public:
  using RowType = Row;
  Group(std::string name);
  Group(std::string name, std::vector<boost::optional<Row>> rows,
        std::string postprocessedWorkspaceName);

  std::string const &name() const;
  void setName(std::string const &name);
  std::string const &postprocessedWorkspaceName() const;

  void appendEmptyRow();
  void appendRow(boost::optional<Row> const &row);
  void insertRow(boost::optional<Row> const &row, int beforeRowAtIndex);
  void removeRow(int rowIndex);
  void updateRow(int rowIndex, boost::optional<Row> const &row);

  Row const *findRowWithTheta(double angle, double tolerance) const;

  boost::optional<Row> const &operator[](int rowIndex) const;
  std::vector<boost::optional<Row>> const &rows() const;

private:
  std::string m_name;
  // TODO Don't have this as an optional.
  std::vector<boost::optional<Row>> m_rows;
  std::string m_postprocessedWorkspaceName;
};

template class MANTIDQT_ISISREFLECTOMETRY_DLL Group<SlicedRow>;
using SlicedGroup = Group<SlicedRow>;

template class MANTIDQT_ISISREFLECTOMETRY_DLL Group<UnslicedRow>;
using UnslicedGroup = Group<UnslicedRow>;

UnslicedGroup unslice(SlicedGroup const &slicedGroup);
SlicedGroup slice(UnslicedGroup const &unslicedGroup);
}
}
#endif // MANTID_CUSTOMINTERFACES_GROUP_H_
