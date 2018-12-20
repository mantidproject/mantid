#ifndef SMOOTH_NEIGHBOURS_DIALOG_H_
#define SMOOTH_NEIGHBOURS_DIALOG_H_

#include "MantidQtWidgets/Common/AlgorithmDialog.h"
#include "MantidQtWidgets/Common/AlgorithmPropertiesWidget.h"

class SmoothNeighboursDialog : public MantidQt::API::AlgorithmDialog {
  Q_OBJECT

public:
  /// Constructor
  SmoothNeighboursDialog(QWidget *parent = nullptr);

protected:
  /// Overridden to enable validators
  void accept() override;

private slots:
  /// Called when input workspace get changed
  void inputWorkspaceChanged(const QString &pName);

private:
  /// Non rectangular detector group name
  static const QString NON_UNIFORM_GROUP;
  /// Rectangular detector group name
  static const QString RECTANGULAR_GROUP;
  /// Input workspace name
  static const QString INPUT_WORKSPACE;

  /// Initialize the layout
  void initLayout() override;

  /// Widget for all the PropertyWidgets
  MantidQt::API::AlgorithmPropertiesWidget *m_propertiesWidget;

  /// Main layout for the dialog
  QVBoxLayout *m_dialogLayout;
};

#endif
