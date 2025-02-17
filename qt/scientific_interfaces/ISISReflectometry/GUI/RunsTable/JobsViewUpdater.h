// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include "Common/Map.h"
#include "MantidQtWidgets/Common/Batch/IJobTreeView.h"
#include "MantidQtWidgets/Common/ParseKeyValueString.h"
#include "Reduction/Group.h"

namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {

class JobsViewUpdater {
public:
  explicit JobsViewUpdater(MantidQt::MantidWidgets::Batch::IJobTreeView &view) : m_view(view) {}

  void groupAppended(int groupIndex, Group const &group);

  void groupRemoved(int groupIndex);

  void rowInserted(int groupIndex, int rowIndex, Row const &row);

  void rowModified(int groupIndex, int rowIndex, Row const &row);

  void setPrecision(const int &precision);

  void resetPrecision();

private:
  MantidQt::MantidWidgets::Batch::IJobTreeView &m_view;
  std::optional<int> m_precision;
};
} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt
