// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/System.h"

#include "DllConfig.h"
#include "IALCBaselineModellingView.h"
#include "MantidQtWidgets/Plotting/RangeSelector.h"

#include "ui_ALCBaselineModellingView.h"

class QSignalMapper;

namespace MantidQt {
namespace CustomInterfaces {

class ALCBaselineModellingPresenter;

/** ALCBaselineModellingView : Widget-based implementation of the ALC Baseline
  Modelling step
                               interface.
*/
class MANTIDQT_MUONINTERFACE_DLL ALCBaselineModellingView : public IALCBaselineModellingView {
  Q_OBJECT

public:
  ALCBaselineModellingView(QWidget *widget);
  ~ALCBaselineModellingView() override;

  void subscribePresenter(ALCBaselineModellingPresenter *presenter) override { m_presenter = presenter; }
  std::string function() const override;
  SectionRow sectionRow(int row) const override;
  SectionSelector sectionSelector(int index) const override;
  int noOfSectionRows() const override;

  void removePlot(QString const &plotName) override;

public slots:
  void initialize() override;
  void setDataCurve(Mantid::API::MatrixWorkspace_sptr workspace, std::size_t const &workspaceIndex = 0) override;
  void setCorrectedCurve(Mantid::API::MatrixWorkspace_sptr workspace, std::size_t const &workspaceIndex = 0) override;
  void setBaselineCurve(Mantid::API::MatrixWorkspace_sptr workspace, std::size_t const &workspaceIndex = 0) override;
  void setFunction(Mantid::API::IFunction_const_sptr func) override;
  void setNoOfSectionRows(int rows) override;
  void setSectionRow(int row, SectionRow values) override;
  void addSectionSelector(int index, SectionSelector values) override;
  void deleteSectionSelector(int index) override;
  void updateSectionSelector(int index, SectionSelector values) override;
  void displayError(const QString &message) override;
  void help() override;
  void handleFitRequested() const override;
  void handleAddSectionRequested() const override;
  void handleRemoveSectionRequested(int row) const override;
  void handleSectionRowModified(int row) const override;
  void handleSectionSelectorModified(int index) const override;
  // -- End of IALCBaselineModellingView interface
  // -------------------------------------------------

private slots:
  /// Show context menu for sections table
  void sectionsContextMenu(const QPoint &widgetPoint);

private:
  ALCBaselineModellingPresenter *m_presenter;
  void initConnections() const override;

  /// Helper to set range selector values
  void setSelectorValues(MantidWidgets::RangeSelector *selector, SectionSelector values);
  QHash<QString, QVariant> getPlotKwargs(MantidWidgets::PreviewPlot *plot, const QString &curveName);

  /// The widget used
  QWidget *const m_widget;

  /// UI form
  Ui::ALCBaselineModellingView m_ui;

  /// Range selectors
  std::map<int, MantidWidgets::RangeSelector *> m_rangeSelectors;

  QSignalMapper *m_selectorModifiedMapper;
};

} // namespace CustomInterfaces
} // namespace MantidQt
