// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef LINEPLOTOPTIONS_H
#define LINEPLOTOPTIONS_H

#include "DllOption.h"
#include "MantidAPI/IMDWorkspace.h"
#include "MantidQtWidgets/Plotting/Qwt/MantidQwtIMDWorkspaceData.h"
#include "ui_LinePlotOptions.h"
#include <QWidget>

class EXPORT_OPT_MANTIDQT_SLICEVIEWER LinePlotOptions : public QWidget {
  Q_OBJECT

public:
  LinePlotOptions(QWidget *parent = nullptr, bool logScaleOption = false);
  ~LinePlotOptions() override;

  void setOriginalWorkspace(Mantid::API::IMDWorkspace_sptr ws);

  int getPlotAxis() const;
  void setPlotAxis(int choice);

  Mantid::API::MDNormalization getNormalization() const;
  void setNormalization(Mantid::API::MDNormalization method);
  bool isLogScaledY() const;
  /// Load the state of the line options from a Mantid project file
  void loadFromProject(const std::string &lines);
  /// Save the state of the line options to a Mantid project file
  std::string saveToProject() const;

public slots:
  void radPlot_changed();
  void radNormalization_changed();
  void onYScalingChanged();

signals:
  /// Signal emitted when the PlotAxisChoice changes
  void changedPlotAxis();
  /// Signal emitted when the Normalization method changes
  void changedNormalization();
  /// Signal emitted when the Y log scaling changes
  void changedYLogScaling();

private:
  void addPlotRadioButton(const std::string &text, const std::string &tooltip,
                          const bool bIntegrated = false);

  Ui::LinePlotOptionsClass ui;

  /// Vector of the various plot axis radio buttons
  QVector<QRadioButton *> m_radPlots;

  /// Chosen plot X-axis
  int m_plotAxis;

  /// Chosen normalization method
  Mantid::API::MDNormalization m_normalize;
};

#endif // LINEPLOTOPTIONS_H
