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

#ifndef PROJECTRECOVERYVIEW_H
#define PROJECTRECOVERYVIEW_H

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

class ProjectRecoveryView {
public:
QWidget *widget;
QVBoxLayout *verticalLayout;
QLabel *label;
QSpacerItem *verticalSpacer;
QTableView *tableView;
QSpacerItem *verticalSpacer_2;
QHBoxLayout *horizontalLayout;
QSpacerItem *horizontalSpacer;
QPushButton *yesButton;
QPushButton *scriptWindowButton;
QPushButton *startMantidButton;

void setupUi(QDialog *ProjectRecovery);

void retranslateUi(QDialog *ProjectRecovery);
};

QT_END_NAMESPACE

#endif // PROJECTRECOVERYVIEW_H
