// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDSAMPLEMATERIALDIALOG_H_
#define MANTIDSAMPLEMATERIALDIALOG_H_

//----------------------------------
// Includes
//----------------------------------
#include "ui_MantidSampleMaterialDialog.h"

#include "MantidAPI/AlgorithmObserver.h"

#include <QDialog>

//----------------------------------
// Forward declarations
//----------------------------------
class MantidUI;

/**
This class displays a information about the sample material for a workspace
and allows it to be modified.

@author Dan Nixon
@date 22/10/2014
*/

class MantidSampleMaterialDialog : public QDialog,
                                   Mantid::API::AlgorithmObserver {
  Q_OBJECT

public:
  MantidSampleMaterialDialog(const QString &wsName, MantidUI *mtdUI,
                             Qt::WFlags flags = nullptr);

public slots:
  void updateMaterial();
  void handleSetMaterial();
  void handleCopyMaterial();

private:
  /// Handle completion of algorithm started from UI
  void finishHandle(const Mantid::API::IAlgorithm *alg) override;

  /// Name of displayed workspace
  QString m_wsName;

  /// A pointer to the MantidUI object
  MantidUI *m_mantidUI;

  Ui::MantidSampleMaterialDialog m_uiForm;
};

#endif // MANTIDSAMPLEMATERIALDIALOG_H_
