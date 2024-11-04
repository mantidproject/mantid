// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
/**
See the developer documentation for Batch Widget at
developer.mantidproject.org/BatchWidget/index.html
*/
#pragma once
#include "MantidQtWidgets/Common/Batch/JobTreeView.h"
#include "MantidQtWidgets/Common/DllOption.h"
namespace MantidQt {
namespace MantidWidgets {
namespace Batch {
class EXPORT_OPT_MANTIDQT_COMMON JobTreeViewSignalAdapter : public QObject, public JobTreeViewSubscriber {
  Q_OBJECT
public:
  JobTreeViewSignalAdapter(JobTreeView &view, QObject *parent = nullptr);

  void notifyCellTextChanged(RowLocation const &itemIndex, int column, std::string const &oldValue,
                             std::string const &newValue) override;
  void notifySelectionChanged() override;
  void notifyRowInserted(RowLocation const &newRowLocation) override;
  void notifyAppendAndEditAtChildRowRequested() override;
  void notifyAppendAndEditAtRowBelowRequested() override;
  void notifyEditAtRowAboveRequested() override;
  void notifyRemoveRowsRequested(std::vector<RowLocation> const &locationsOfRowsToRemove) override;
  void notifyCopyRowsRequested() override;
  void notifyCutRowsRequested() override;
  void notifyPasteRowsRequested() override;
  void notifyFilterReset() override;
signals:
  void cellTextChanged(MantidQt::MantidWidgets::Batch::RowLocation const &itemIndex, int column,
                       std::string const &oldValue, std::string const &newValue);
  void selectionChanged();
  void rowInserted(MantidQt::MantidWidgets::Batch::RowLocation const &newRowLocation);
  void filterReset();
  void removeRowsRequested(std::vector<MantidQt::MantidWidgets::Batch::RowLocation> const &locationsOfRowsToRemove);
  void appendAndEditAtChildRowRequested();
  void appendAndEditAtRowBelowRequested();
  void editAtRowAboveRequested();
  void copyRowsRequested();
  void pasteRowsRequested();
  void cutRowsRequested();
};
} // namespace Batch
} // namespace MantidWidgets
} // namespace MantidQt
