#ifndef MANTIDQTMANTIDWIDGETS_DATASELECTOR_H_
#define MANTIDQTMANTIDWIDGETS_DATASELECTOR_H_

#include "WidgetDllOption.h"
#include "ui_DataSelector.h"

#include "MantidQtAPI/AlgorithmRunner.h"
#include "MantidQtAPI/MantidWidget.h"

#include <QWidget>

namespace MantidQt
{
  namespace MantidWidgets
  {
    /**
    This class defines a widget for selecting a workspace of file path by using a combination
    of two child MantidWidgets: MWRunFiles and WorkspaceSelector. This widget combines the two to
    produce a single composite widget that emits signals when the user has chosen appropriate input.

    @author Samuel Jackson
    @date 07/08/2013

    Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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

    class EXPORT_OPT_MANTIDQT_MANTIDWIDGETS DataSelector : public API::MantidWidget
    {
      Q_OBJECT

      Q_PROPERTY(bool autoLoad READ willAutoLoad WRITE setAutoLoad)
      Q_PROPERTY(QString loadLabelText READ getLoadBtnText WRITE setLoadBtnText)
      Q_PROPERTY(QStringList workspaceSuffixes READ getWSSuffixes WRITE setWSSuffixes)
      Q_PROPERTY(QStringList fileBrowserSuffixes READ getFBSuffixes WRITE setFBSuffixes)
      Q_PROPERTY(bool showLoad READ willShowLoad WRITE setShowLoad)

    public:
      DataSelector(QWidget *parent = 0);
      virtual ~DataSelector();

      /// Get the current file path in the MWRunFiles widget
      QString getFullFilePath() const;
      /// Get the currently available file or workspace name
      QString getCurrentDataName() const;
      /// Get whether the file selector is currently being shown
      bool isFileSelectorVisible() const;
      /// Get whether the workspace selector is currently being shown
      bool isWorkspaceSelectorVisible() const;
      /// Checks if widget is in a valid state
      bool isValid();
      /// Get file problem, empty string means no error.
      QString getProblem() const;
      /// Check if the widget is set to automatically attempt to load files
      bool willAutoLoad();
      /// Set the widget to automatically attempt to load files
      void setAutoLoad(bool load);
      /// Get the text of the load files button
      QString getLoadBtnText();
      /// Set the text of the load files button
      void setLoadBtnText(const QString &  text);
      /// Get file suffixes to filter for in the workspace selector
      QStringList getWSSuffixes();
      /// Set file suffixes to filter for in the workspace selector
      void setWSSuffixes(const QStringList & suffixes);
      /// Get file suffixes to filter for in the file browser
      QStringList getFBSuffixes();
      /// Set file suffixes to filter for in the file browser
      void setFBSuffixes(const QStringList & suffixes);
      /// Read settings from the given group
      void readSettings(const QString & group);
      /// Save settings in the given group
      void saveSettings(const QString & group);
      /// Check if the widget will show the load button
      bool willShowLoad();
      /// Set if the load button should be shown
      void setShowLoad(bool load);

    signals:
      /// Signal emitted when files were found but widget isn't autoloading
      void filesFound();
      /// Signal emitted when file input is visible
      void fileViewVisible();
      /// Signal emitted when workspace selector is visible
      void workspaceViewVisible();
      /// Signal emitted when data is ready from a workspace selector or file browser
      void dataReady(const QString& wsname);
      /// Signal emitted when the load button is clicked
      void loadClicked();

    protected:
      //Method for handling drop events
      void dropEvent(QDropEvent *);
      //called when a drag event enters the class
      void dragEnterEvent(QDragEnterEvent *);

    private slots:
      /// Slot called when the current view is changed
      void handleViewChanged(int index);
      /// Slot called when file input is available
      void handleFileInput();
      /// Slot called when workspace input is available
      void handleWorkspaceInput();
      /// Slot called if the widget fails to auto load the file.
      void handleAutoLoadComplete(bool error);

    private:
      /// Attempt to automatically load a file
      void autoLoadFile(const QString& filenames);
      /// Member containing the widgets child widgets.
      Ui::DataSelector m_uiForm;
      /// Algorithm Runner used to run the load algorithm
      MantidQt::API::AlgorithmRunner m_algRunner;
      /// Flag to enable auto loading. By default this is set to true.
      bool m_autoLoad;
      /// Flag to show or hide the load button. By default this is set to true.
      bool m_showLoad;

    };

  } /* namespace MantidWidgets */
} /* namespace MantidQt */
#endif /* DATASELECTOR_H_ */
