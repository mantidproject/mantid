/** Adapter class which handles saving or restoring project windows

@author Samuel Jones, ISIS, RAL
@date 21/09/2018

Copyright &copy; 2007-2018 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
National Laboratory & European Spallation Source

This file is part of Mantid.

Mantid is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

Mantid is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

File change history is stored at: <https://github.com/mantidproject/mantid>
*/

#include "ProjectRecoveryView.h"

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

void ProjectRecoveryView::setupUi(QDialog *ProjectRecovery)
{
  if (ProjectRecovery->objectName().isEmpty())
    ProjectRecovery->setObjectName(QString::fromUtf8("ProjectRecovery"));
  ProjectRecovery->resize(562, 322);
  widget = new QWidget(ProjectRecovery);
  widget->setObjectName(QString::fromUtf8("widget"));
  widget->setGeometry(QRect(11, 18, 541, 291));
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
  tableView->setObjectName(QString::fromUtf8("theTable"));
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
  horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

  horizontalLayout->addItem(horizontalSpacer);

  yesButton = new QPushButton(widget);
  yesButton->setObjectName(QString::fromUtf8("yesButton"));

  horizontalLayout->addWidget(yesButton);

  scriptWindowButton = new QPushButton(widget);
  scriptWindowButton->setObjectName(QString::fromUtf8("scriptWindowButton"));

  horizontalLayout->addWidget(scriptWindowButton);

  startMantidButton = new QPushButton(widget);
  startMantidButton->setObjectName(QString::fromUtf8("startMantidButton"));

  horizontalLayout->addWidget(startMantidButton);

  verticalLayout->addLayout(horizontalLayout);

  retranslateUi(ProjectRecovery);

  QMetaObject::connectSlotsByName(ProjectRecovery);
}

void ProjectRecoveryView::retranslateUi(QDialog *ProjectRecovery)
{
  ProjectRecovery->setWindowTitle(QApplication::translate("ProjectRecovery", "Dialog", 0, QApplication::UnicodeUTF8));
  label->setText(QApplication::translate("ProjectRecovery", "It looks like Mantid crashed recently. There is a recovery checkpoint available would you like to try it?", 0, QApplication::UnicodeUTF8));
  yesButton->setText(QApplication::translate("ProjectRecovery", "Yes", 0, QApplication::UnicodeUTF8));
  scriptWindowButton->setText(QApplication::translate("ProjectRecovery", "Just open in script window", 0, QApplication::UnicodeUTF8));
  startMantidButton->setText(QApplication::translate("ProjectRecovery", "Start Mantid normally", 0, QApplication::UnicodeUTF8));
}