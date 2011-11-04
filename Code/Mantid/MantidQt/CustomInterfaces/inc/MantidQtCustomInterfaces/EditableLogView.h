#ifndef MANTID_CUSTOMINTERFACES_EDITABLELOGVIEW_H_
#define MANTID_CUSTOMINTERFACES_EDITABLELOGVIEW_H_

#include "MantidKernel/System.h"
#include "MantidQtCustomInterfaces/LogView.h"
#include <qwidget.h>
#include <boost/shared_ptr.hpp>

class QTableWidget;
namespace MantidQt
{
namespace CustomInterfaces
{
  class LogPresenter;
  /** EditableLogView : Concrete view for viewing and editing log items.
    @author Owen Arnold
    @date 2011-11-04

    Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
  */
  class EditableLogView: public QWidget, public LogView
  {
      Q_OBJECT

    private:

      /// MVP presenter
      boost::shared_ptr<LogPresenter>  m_presenter;

      /// Default/Cached palette.
      QPalette m_pal;

      /// Flag indicating that close has been requested.
      bool m_request_close;

      /// table widget;
      QTableWidget* m_tableWidget;

    private slots:

      void close();

    public:

      /// Constructor
      EditableLogView(boost::shared_ptr<LogPresenter> presenter);

      /// Destructor
      ~EditableLogView();

      /// Indicate that the view has been modified.
      void indicateModified();

      /// Indicate that the view is unmodified.
      void indicateDefault();

      /// Initalization method.
      void initalize(std::vector<AbstractMementoItem_sptr>);

      /// Getter for the log data.
      virtual LogDataMap getLogData() const;

      /// Getter indicating that a mode swap has been requested.
      virtual bool swapMode() const;

      /// Hides the view.
      void show();

      /// Shows the view.
      void hide();

  };


} // namespace CustomInterfaces
} // namespace MantidQt

#endif  /* MANTID_CUSTOMINTERFACES_EDITABLELOGVIEW_H_ */
