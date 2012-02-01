#ifndef LINEPLOTOPTIONS_H
#define LINEPLOTOPTIONS_H

#include <QtGui/QWidget>
#include "ui_LinePlotOptions.h"
#include "MantidQtAPI/MantidQwtIMDWorkspaceData.h"
#include "MantidAPI/IMDWorkspace.h"

class LinePlotOptions : public QWidget
{
  Q_OBJECT

public:
  LinePlotOptions(QWidget *parent = 0);
  ~LinePlotOptions();

  void setXYNames(const std::string & xName, const std::string & yName);

  MantidQwtIMDWorkspaceData::PlotAxisChoice getPlotAxis() const;
  void setPlotAxis(MantidQwtIMDWorkspaceData::PlotAxisChoice choice);

  Mantid::API::MDNormalization getNormalization() const;
  void setNormalization(Mantid::API::MDNormalization method);

public slots:
  void radPlot_changed();
  void radNormalization_changed();

signals:
  /// Signal emitted when the PlotAxisChoice changes
  void changedPlotAxis();
  /// Signal emitted when the Normalization method changes
  void changedNormalization();

private:
  Ui::LinePlotOptionsClass ui;

  /// Workspace being sliced
  Mantid::API::IMDWorkspace_sptr m_ws;

  /// Chosen plot X-axis
  MantidQwtIMDWorkspaceData::PlotAxisChoice m_plotAxis;

  /// Chosen normalization method
  Mantid::API::MDNormalization m_normalize;
};

#endif // LINEPLOTOPTIONS_H
