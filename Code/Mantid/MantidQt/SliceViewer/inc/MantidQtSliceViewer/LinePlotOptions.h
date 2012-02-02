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

  void setOriginalWorkspace(Mantid::API::IMDWorkspace_sptr ws);

  int getPlotAxis() const;
  void setPlotAxis(int choice);

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

  void addPlotRadioButton(const std::string & text, const std::string & tooltip);

  Ui::LinePlotOptionsClass ui;

  /// Vector of the various plot axis radio buttons
  QVector<QRadioButton *> m_radPlots;

  /// Original workspace (gives the dimensions to plot);
  Mantid::API::IMDWorkspace_sptr m_originalWs;

  /// Chosen plot X-axis
  int m_plotAxis;

  /// Chosen normalization method
  Mantid::API::MDNormalization m_normalize;
};

#endif // LINEPLOTOPTIONS_H
