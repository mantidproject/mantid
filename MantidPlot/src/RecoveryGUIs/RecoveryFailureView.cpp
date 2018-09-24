#include "RecoveryFailure.h"

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

void RecoveryFailure::setupUi(QDialog *ProjectRecovery)
{
  if (ProjectRecovery->objectName().isEmpty())
    ProjectRecovery->setObjectName(QString::fromUtf8("ProjectRecovery"));
  ProjectRecovery->resize(545, 321);
  widget = new QWidget(ProjectRecovery);
  widget->setObjectName(QString::fromUtf8("widget"));
  widget->setGeometry(QRect(11, 18, 521, 291));
  verticalLayout = new QVBoxLayout(widget);
  verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
  verticalLayout->setContentsMargins(0, 0, 0, 0);
  label = new QLabel(widget);
  label->setObjectName(QString::fromUtf8("label"));
  label->setTextFormat(Qt::PlainText);
  label->setAlignment(Qt::AlignCenter);
  label->setWordWrap(true);

  verticalLayout->addWidget(label);

  verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

  verticalLayout->addItem(verticalSpacer);

  tableView = new QTableView(widget);
  tableView->setObjectName(QString::fromUtf8("tableView"));
  tableView->setEditTriggers(QAbstractItemView::SelectedClicked);
  tableView->setDragDropOverwriteMode(false);
  tableView->setAlternatingRowColors(false);
  tableView->setSelectionMode(QAbstractItemView::SingleSelection);
  tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
  tableView->setSortingEnabled(true);

  verticalLayout->addWidget(tableView);

  verticalSpacer_2 = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

  verticalLayout->addItem(verticalSpacer_2);

  horizontalLayout = new QHBoxLayout();
  horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
  yesButton = new QPushButton(widget);
  yesButton->setObjectName(QString::fromUtf8("yesButton"));
  QSizePolicy sizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
  sizePolicy.setHorizontalStretch(0);
  sizePolicy.setVerticalStretch(0);
  sizePolicy.setHeightForWidth(yesButton->sizePolicy().hasHeightForWidth());
  yesButton->setSizePolicy(sizePolicy);

  horizontalLayout->addWidget(yesButton);

  yesButton_2 = new QPushButton(widget);
  yesButton_2->setObjectName(QString::fromUtf8("yesButton_2"));
  sizePolicy.setHeightForWidth(yesButton_2->sizePolicy().hasHeightForWidth());
  yesButton_2->setSizePolicy(sizePolicy);

  horizontalLayout->addWidget(yesButton_2);

  scriptWindowButton = new QPushButton(widget);
  scriptWindowButton->setObjectName(QString::fromUtf8("scriptWindowButton"));

  horizontalLayout->addWidget(scriptWindowButton);

  startMantidButton = new QPushButton(widget);
  startMantidButton->setObjectName(QString::fromUtf8("startMantidButton"));

  horizontalLayout->addWidget(startMantidButton);

  verticalLayout->addLayout(horizontalLayout);

  retranslateUi(ProjectRecovery);

  QMetaObject::connectSlotsByName(ProjectRecovery);
} // setupUi

void RecoveryFailure::retranslateUi(QDialog *ProjectRecovery)
{
  ProjectRecovery->setWindowTitle(QApplication::translate("ProjectRecovery", "Dialog", 0, QApplication::UnicodeUTF8));
  label->setText(QApplication::translate("ProjectRecovery", "Project recovery has failed unexpectedly, you can choose to recover from one of the checkpoints below, open the selected checkpoint in the script window, or you can start Mantid normally.", 0, QApplication::UnicodeUTF8));
  yesButton->setText(QApplication::translate("ProjectRecovery", "Try last \n"
                                                                "checkpoint again",
                                             0, QApplication::UnicodeUTF8));
  yesButton_2->setText(QApplication::translate("ProjectRecovery", "Try selected\n"
                                                                  "checkpoint",
                                               0, QApplication::UnicodeUTF8));
  scriptWindowButton->setText(QApplication::translate("ProjectRecovery", "Open selected in\n"
                                                                         "script window",
                                                      0, QApplication::UnicodeUTF8));
  startMantidButton->setText(QApplication::translate("ProjectRecovery", "Start Mantid\n"
                                                                        "normally",
                                                     0, QApplication::UnicodeUTF8));
} // retranslateUi