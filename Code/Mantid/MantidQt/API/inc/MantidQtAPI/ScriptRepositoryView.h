#ifndef MANTID_API_SCRIPTREPOSITORYVIEW_H_
#define MANTID_API_SCRIPTREPOSITORYVIEW_H_

#include <QDialog>
#include "MantidKernel/System.h"
#include "ui_ScriptRepositoryView.h"
#include "DllOption.h"
namespace MantidQt
{
namespace API
{
  class RepoModel; 
  /** ScriptRepositoryView : TODO: DESCRIPTION
    
    Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
  class  EXPORT_OPT_MANTIDQT_API ScriptRepositoryView : public QDialog
  {
    Q_OBJECT
  public:
    ScriptRepositoryView(QWidget * parent=0);
    virtual ~ScriptRepositoryView();
    public slots:
    void filterValues(QString);
  signals:
    void loadScript(const QString);

    protected slots:
    void cell_activated(const QModelIndex & ); 
    void cell_clicked(const QModelIndex & );

  private:
    Ui::ScriptRepositoryView * ui; 
    RepoModel * model;
  };


} // namespace API
} // namespace Mantid

#endif  /* MANTID_API_SCRIPTREPOSITORYVIEW_H_ */
