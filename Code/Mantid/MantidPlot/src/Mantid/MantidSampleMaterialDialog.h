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

Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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
Code Documentation is available at: <http://doxygen.mantidproject.org>
*/

class MantidSampleMaterialDialog : public QDialog, Mantid::API::AlgorithmObserver
{
  Q_OBJECT

public:
  MantidSampleMaterialDialog(const QString & wsName, MantidUI* mtdUI, Qt::WFlags flags = 0);

public slots:
  void updateMaterial();
  void handleSetMaterial();
  void handleCopyMaterial();

private:
  /// Handle completion of algorithm started from UI
  virtual void finishHandle(const Mantid::API::IAlgorithm *alg);

  /// Name of displayed workspace
  QString m_wsName;

  /// A pointer to the MantidUI object
  MantidUI* m_mantidUI;

  Ui::MantidSampleMaterialDialog m_uiForm;

};

#endif //MANTIDSAMPLEMATERIALDIALOG_H_
