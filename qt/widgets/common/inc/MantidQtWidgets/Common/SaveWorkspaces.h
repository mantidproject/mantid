// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllOption.h"
#include "MantidQtWidgets/Common/MantidDialog.h"
#include <QCheckBox>
#include <QGridLayout>
#include <QHash>
#include <QLineEdit>
#include <QListWidget>
#include <QSignalMapper>
#include <QString>

namespace MantidQt {
namespace MantidWidgets {
/**
Implements a dialog box that allows users to save multiple Mantid workspaces

@author Steve Williams
*/
class EXPORT_OPT_MANTIDQT_COMMON SaveWorkspaces : public API::MantidDialog {
  Q_OBJECT

public:
  SaveWorkspaces(QWidget *parent, const QString &suggFname, QHash<const QCheckBox *const, QString> &defSavs,
                 bool saveAsZeroErrorFree);
  void initLayout();
  /// Returns the save extension expected the name algorithm
  static QString getSaveAlgExt(const QString &algName);
public slots:
  void onSaveAsZeroErrorFreeChanged(int state);
  void onUpdateGeomtryInformation(QString &geometryID, QString &sampleHeight, QString &sampleWidth,
                                  QString &sampleThickness);

signals:
  void closing();
  void createZeroErrorFreeWorkspace(QString &originalWorkspace, QString &zeroFreeWorkspace);
  void deleteZeroErrorFreeWorkspace(QString &zeroFreeWorkspace);
  void updateGeometryInformation();

private:
  QLineEdit *m_fNameEdit;
  QListWidget *m_workspaces;
  QCheckBox *m_append;
  QString m_lastName;
  bool m_saveAsZeroErrorFree;
  QString m_geometryID;
  QString m_sampleHeight;
  QString m_sampleWidth;
  QString m_sampleThickness;

  QHash<QCheckBox *const, QString> m_savFormats;
  using SavFormatsConstIt = QHash<QCheckBox *const, QString>::const_iterator;

  void setupLine1(QHBoxLayout *const lineOne);
  void setupLine2(QHBoxLayout *const lineTwo, const QHash<const QCheckBox *const, QString> &defSavs);
  void readSettings();
  void setFileName(const QString &newName);
  void setupFormatTicks(const QHash<const QCheckBox *const, QString> &defSavs);
  void saveSettings() const;

  void addButtonsDisab(int row);
  void closeEvent(QCloseEvent *event) override;
  QString saveList(const QList<QListWidgetItem *> &list, const QString &algorithm, QString fileBase, bool toAppend,
                   QHash<QString, QString> workspaceMap);
  QHash<QString, QString> provideZeroFreeWorkspaces(const QListWidget *workspaces);
  void removeZeroFreeWorkspaces(const QHash<QString, QString> &workspaces);
  bool isValid();
private slots:
  void saveSel();
  void setFileName(int row);
  void saveFileBrowse();
};
} // namespace MantidWidgets
} // namespace MantidQt
