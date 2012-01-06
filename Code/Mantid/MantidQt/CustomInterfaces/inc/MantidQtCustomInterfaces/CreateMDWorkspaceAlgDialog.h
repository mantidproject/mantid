#ifndef MANTIDQTCUSTOMINTERFACES_CREATEMDWORKSPACE_ALG_DIALOG_H_
#define MANTIDQTCUSTOMINTERFACES_CREATEMDWORKSPACE_ALG_DIALOG_H_

#include <QDialog>
#include "ui_CreateMDWorkspaceAlgDialog.h"

/// Code-behind for CreateMDWorkspaceAlgDialog.
class CreateMDWorkspaceAlgDialog : public QDialog
{
  Q_OBJECT

public:

  CreateMDWorkspaceAlgDialog();
  virtual ~CreateMDWorkspaceAlgDialog();

  QString getQDimension() const;
  QString getAnalysisMode() const;
  QString getOtherDimensions() const;
  QString getMaxExtents() const;
  QString getMinExtents() const;
  bool getPreprocessedEvents() const;



private:

  Ui::CreateMDWorkspaceAlgDialog m_uiForm;

};


#endif