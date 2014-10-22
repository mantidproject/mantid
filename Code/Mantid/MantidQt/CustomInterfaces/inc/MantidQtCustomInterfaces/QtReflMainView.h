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
      static std::string name() { return "New ISIS Reflectometry (Prototype)"; }
      // This interface's categories.
      static QString categoryInfo() { return "Reflectometry"; }

      //Connect the model
      virtual void showTable(Mantid::API::ITableWorkspace_sptr model);

      //Dialog/Prompt methods
      virtual std::string askUserString(const std::string& prompt, const std::string& title, const std::string& defaultValue);
      virtual bool askUserYesNo(std::string prompt, std::string title);
      virtual void giveUserInfo(std::string prompt, std::string title);
      virtual void giveUserWarning(std::string prompt, std::string title);
      virtual void giveUserCritical(std::string prompt, std::string title);

      //Set the status of the progress bar
      virtual void setProgressRange(int min, int max);
      virtual void setProgress(int progress);

      //Settor methods
      virtual void setInstrumentList(const std::vector<std::string>& instruments, const std::string& defaultInstrument);
      virtual void setOptionsHintStrategy(HintStrategy* hintStrategy);

      //Accessor methods
      virtual std::vector<size_t> getSelectedRowIndexes() const;
      virtual std::string getSearchInstrument() const;
      virtual std::string getProcessInstrument() const;
      virtual std::string getWorkspaceToOpen() const;

    private:
      //initialise the interface
      virtual void initLayout();
      //the presenter
      boost::shared_ptr<IReflPresenter> m_presenter;
      //the interface
      Ui::reflMainWidget ui;
      //the workspace the user selected to open
      std::string m_toOpen;

    private slots:
      void setModel(QString name);
      void actionNewTable();
      void actionSave();
      void actionSaveAs();
      void actionAddRow();
      void actionDeleteRow();
      void actionProcess();
      void actionGroupRows();
    };


  } // namespace CustomInterfaces
} // namespace Mantid

#endif  /* MANTID_CUSTOMINTERFACES_QTREFLMAINVIEW_H_ */
