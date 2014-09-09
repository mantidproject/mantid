#ifndef MANTID_CUSTOMINTERFACES_QTREFLMAINVIEW_H_
#define MANTID_CUSTOMINTERFACES_QTREFLMAINVIEW_H_

#include "MantidKernel/System.h"
#include "MantidQtAPI/UserSubWindow.h"
#include "MantidQtCustomInterfaces/ReflMainView.h"
#include "MantidQtCustomInterfaces/IReflPresenter.h"
#include <boost/scoped_ptr.hpp>
#include "ui_ReflMainWidget.h"

namespace MantidQt
{
  namespace CustomInterfaces
  {

    /** QtReflMainView : Provides an interface for processing reflectometry data.

    Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
    class DLLExport QtReflMainView : public MantidQt::API::UserSubWindow, public ReflMainView
    {
      Q_OBJECT
    public:
      QtReflMainView(QWidget *parent = 0);
      virtual ~QtReflMainView();

      /// Name of the interface
      static std::string name() { return "New ISIS Reflectometry"; }
      // This interface's categories.
      static QString categoryInfo() { return "Reflectometry"; }

      //Connect the model
      virtual void showTable(Mantid::API::ITableWorkspace_sptr model);

      //dialog box methods
      virtual bool askUserString(const std::string& prompt, const std::string& title, const std::string& defaultValue);
      virtual std::string getUserString() const {return m_UserString;}
      virtual void giveUserInfo(std::string prompt, std::string title);
      virtual void giveUserWarning(std::string prompt, std::string title);
      virtual void giveUserCritical(std::string prompt, std::string title);
      virtual bool askUserYesNo(std::string prompt, std::string title);

      //flag methods
      virtual std::vector<size_t> getSelectedRowIndexes() const;
      virtual Flag getFlag();
      virtual bool flagSet() const;

    protected:
      //notify flags
      std::vector<Flag> m_flags;

    private:
      //initialise the interface
      virtual void initLayout();
      //the string provided by the user in askUserString()
      std::string m_UserString;
      //the presenter
      boost::scoped_ptr<IReflPresenter> m_presenter;
      //the interface
      Ui::reflMainWidget ui;

    private slots:
      void setModel(QString name);
      void setNew();
      void saveButton();
      void saveAsButton();
      void addRowButton();
      void deleteRowButton();
      void processButton();
    };


  } // namespace CustomInterfaces
} // namespace Mantid

#endif  /* MANTID_CUSTOMINTERFACES_QTREFLMAINVIEW_H_ */
