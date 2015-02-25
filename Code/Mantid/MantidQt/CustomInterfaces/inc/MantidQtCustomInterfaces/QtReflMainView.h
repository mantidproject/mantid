#ifndef MANTID_CUSTOMINTERFACES_QTREFLMAINVIEW_H_
#define MANTID_CUSTOMINTERFACES_QTREFLMAINVIEW_H_

#include "MantidKernel/System.h"
#include "MantidQtAPI/UserSubWindow.h"
#include "MantidQtCustomInterfaces/ReflMainView.h"
#include "MantidQtCustomInterfaces/IReflPresenter.h"
#include "MantidQtCustomInterfaces/ReflSearchModel.h"
#include "MantidQtCustomInterfaces/QReflTableModel.h"
#include "MantidQtMantidWidgets/SlitCalculator.h"
#include <boost/scoped_ptr.hpp>
#include <QSignalMapper>
#include "ui_ReflMainWidget.h"

namespace MantidQt
{
  namespace CustomInterfaces
  {

    /** QtReflMainView : Provides an interface for processing reflectometry data.

    Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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
      static std::string name() { return "ISIS Reflectometry (Polref)"; }
      // This interface's categories.
      static QString categoryInfo() { return "Reflectometry"; }

      //Connect the model
      virtual void showTable(QReflTableModel_sptr model);
      virtual void showSearch(ReflSearchModel_sptr model);

      //Dialog/Prompt methods
      virtual std::string askUserString(const std::string& prompt, const std::string& title, const std::string& defaultValue);
      virtual bool askUserYesNo(std::string prompt, std::string title);
      virtual void giveUserInfo(std::string prompt, std::string title);
      virtual void giveUserWarning(std::string prompt, std::string title);
      virtual void giveUserCritical(std::string prompt, std::string title);
      virtual void showAlgorithmDialog(const std::string& algorithm);

      //Plotting
      virtual void plotWorkspaces(const std::set<std::string>& workspaces);

      //Set the status of the progress bar
      virtual void setProgressRange(int min, int max);
      virtual void setProgress(int progress);

      //Settor methods
      virtual void setSelection(const std::set<int>& rows);
      virtual void setTableList(const std::set<std::string>& tables);
      virtual void setInstrumentList(const std::vector<std::string>& instruments, const std::string& defaultInstrument);
      virtual void setOptionsHintStrategy(MantidQt::MantidWidgets::HintStrategy* hintStrategy);
      virtual void setClipboard(const std::string& text);

      //Accessor methods
      virtual std::set<int> getSelectedRows() const;
      virtual std::set<int> getSelectedSearchRows() const;
      virtual std::string getSearchInstrument() const;
      virtual std::string getProcessInstrument() const;
      virtual std::string getWorkspaceToOpen() const;
      virtual std::string getClipboard() const;
      virtual std::string getSearchString() const;

      virtual boost::shared_ptr<IReflPresenter> getPresenter() const;

    private:
      //initialise the interface
      virtual void initLayout();
      //the presenter
      boost::shared_ptr<IReflPresenter> m_presenter;
      //the models
      QReflTableModel_sptr m_model;
      ReflSearchModel_sptr m_searchModel;
      //the interface
      Ui::reflMainWidget ui;
      //the workspace the user selected to open
      std::string m_toOpen;
      QSignalMapper* m_openMap;
      MantidWidgets::SlitCalculator* m_calculator;

    private slots:
      void on_actionNewTable_triggered();
      void on_actionSaveTable_triggered();
      void on_actionSaveTableAs_triggered();
      void on_actionAppendRow_triggered();
      void on_actionPrependRow_triggered();
      void on_actionDeleteRow_triggered();
      void on_actionProcess_triggered();
      void on_actionGroupRows_triggered();
      void on_actionClearSelected_triggered();
      void on_actionCopySelected_triggered();
      void on_actionCutSelected_triggered();
      void on_actionPasteSelected_triggered();
      void on_actionExpandSelection_triggered();
      void on_actionOptionsDialog_triggered();
      void on_actionSearch_triggered();
      void on_actionTransfer_triggered();
      void on_actionImportTable_triggered();
      void on_actionExportTable_triggered();
      void on_actionHelp_triggered();
      void on_actionPlotRow_triggered();
      void on_actionPlotGroup_triggered();
      void on_actionSlitCalculator_triggered();

      void on_comboSearchInstrument_currentIndexChanged(int index);
      void on_comboProcessInstrument_currentIndexChanged(int index);

      void setModel(QString name);
      void tableUpdated(const QModelIndex& topLeft, const QModelIndex& bottomRight);
      void showContextMenu(const QPoint& pos);
      void showSearchContextMenu(const QPoint& pos);
    };


  } // namespace CustomInterfaces
} // namespace Mantid

#endif  /* MANTID_CUSTOMINTERFACES_QTREFLMAINVIEW_H_ */
