// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDPLOT_FIRST_RUN_SETUP_WINDOW
#define MANTIDPLOT_FIRST_RUN_SETUP_WINDOW

#include "ui_FirstTimeSetup.h"
#include <QDialog>

/**
 * FirstTimeSetup dialog for MantidPlot.
 *
 */

class FirstTimeSetup : public QDialog {
  Q_OBJECT

public:
  explicit FirstTimeSetup(QWidget *parent = nullptr);
  ~FirstTimeSetup() override;

private:
  void initLayout();

public slots:
  void openExternalLink(const QString &);

private slots:
  void confirm();
  void cancel();
  void allowUsageDataStateChanged(int);

  void openReleaseNotes();
  void openSampleDatasets();
  void openMantidIntroduction();
  void openPythonIntroduction();
  void openPythonInMantid();
  void openExtendingMantid();

  void facilitySelected(const QString &facility);
  void openManageUserDirectories();

private:
  Ui::FirstTimeSetup m_uiForm;
};

#endif