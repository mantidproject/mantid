/********************************************************************************
** Form generated from reading UI file 'ProjectRecovery Failure GuiYN6861.ui'
**
** Created by: Qt User Interface Compiler version 4.8.7
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef RECOVERYFAILUREVIEW_H
#define RECOVERYFAILUREVIEW_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QDialog>
#include <QtGui/QHBoxLayout>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QPushButton>
#include <QtGui/QSpacerItem>
#include <QtGui/QTableView>
#include <QtGui/QVBoxLayout>
#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE

class RecoveryFailureView
{
public:
  QWidget *widget;
  QVBoxLayout *verticalLayout;
  QLabel *label;
  QSpacerItem *verticalSpacer;
  QTableView *tableView;
  QSpacerItem *verticalSpacer_2;
  QHBoxLayout *horizontalLayout;
  QPushButton *yesButton;
  QPushButton *yesButton_2;
  QPushButton *scriptWindowButton;
  QPushButton *startMantidButton;

  void setupUi(QDialog *ProjectRecovery);

  void retranslateUi(QDialog *ProjectRecovery);
};

namespace Ui
{
class RecoveryFailureView : public RecoveryFailureView
{
};
} // namespace Ui

QT_END_NAMESPACE

#endif // RECOVERYFAILUREVIEW_H
