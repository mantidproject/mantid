#ifndef MANTIDQTCUSTOMINTERFACES_HOMER_H_
#define MANTIDQTCUSTOMINTERFACES_HOMER_H_

#include "MantidQtCustomInterfaces/ui_ConvertToEnergy.h"

#include "MantidQtCustomInterfaces/deltaECalc.h"
#include "MantidQtCustomInterfaces/Background.h"
#include "MantidQtMantidWidgets/pythonCalc.h"
#include "MantidQtMantidWidgets/MWRunFiles.h"
#include "MantidQtMantidWidgets/MWDiag.h"
#include "MantidKernel/PropertyWithValue.h"
#include "MantidQtAPI/UserSubWindow.h"
#include "MantidAPI/IAlgorithm.h"
#include <QString>
#include <QHash>
#include <QSettings>

//-----------------------------------------------------
// Forward declarations
//-----------------------------------------------------
class QButtonGroup;
class QAbstractButton;
class QHideEvent;
class QCloseEvent;
class QShowEvent;

namespace MantidQt
{
  namespace CustomInterfaces
  {
    //-----------------------------------------------------
    // Forward declarations
    //-----------------------------------------------------
    class Background;

    /** 
    This class implements the ConvertToEnergy interface for the direct instruments

    @author Steve Williams, ISIS Computing Group, RAL
    @author Martyn Gigg, Tessella Support Services plc
    @date 24/02/2009

    Copyright &copy; 2009 STFC Rutherford Appleton Laboratories

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
    class Homer : public MantidQt::API::UserSubWindow
    {
      Q_OBJECT

    public:
      Homer(QWidget *parent, Ui::ConvertToEnergy & uiForm);

      // Initialize the layout
      virtual void initLayout();
      virtual void initLocalPython();

    private:
      void showEvent(QShowEvent *event);
      void hideEvent(QHideEvent *event);
      void closeEvent(QCloseEvent *event);
      /// enable the run button if the results dialog has been closed and the python has stopped
      void pythonIsRunning(bool running = true);
      QString defaultName();
      std::string insertNumber(const std::string &filename, const int number);

      // set up the default values, signals and slots, tooltips, etc.
      QString setUpInstru();
      void setUpPage1();
      void page1FileWidgs();
      void page1Validators();
      void setUpPage2();
      void setUpPage3();

      bool isInputValid() const;
      bool isFileInputValid() const;
      bool isParamInputValid() const;
      bool isRebinStringValid() const;
      bool checkEi(const QString & text) const;

      void setSettingsGroup(const QString &instrument);
      bool runScripts();
      void checkNoErrors(const deltaECalc &unitsConv);
      void saveSettings();
      QString openFileDia(const bool save, const QStringList &exts);
      void syncBackgroundSettings();

    signals:
      // these signals send data to the find bad detectors (diag) widget
      void MWDiag_updateWBV(const QString &);
      void MWDiag_updateTOFs(const double &, const double &);
      void MWDiag_sendRuns(const std::vector<std::string> &);

    private slots:
        void validateAbsEi(const QString &);
        void validateRunEi(const QString &);
        void validateRebinBox(const QString &);
        void validateMapFile();
        ///run the algorithms that can be run with the data that users supplied
        void runClicked();
        //get rid of this one, no?
        void browseClicked(const QString buttonDis);
        /// open the wiki page for this interface in their browser
        void helpClicked();
        void runFilesChanged();
        void updateSaveName();
        void saveNameUpd();
        void updateWBV();
        void bgRemoveClick();
        void bgRemoveReadSets();
        void instrSelectionChanged(const QString& prefix);
        void setIDFValues(const QString & prefix);  
        void saveFormatOptionClicked(QAbstractButton*);
        void updateAbsEi(const QString & text);
        void markAbsEiDirty(bool dirty = true);

    private:
      Ui::ConvertToEnergy m_uiForm;

      MantidWidgets::MWRunFiles *m_runFilesWid;
      MantidWidgets::MWRunFile *m_WBVWid;
      MantidWidgets::MWRunFiles *m_absRunFilesWid;
      MantidWidgets::MWRunFile *m_absWhiteWid;

      Background *m_backgroundDialog;
      /// A pointer to the widget with the user controls for finding bad detectors
      MantidWidgets::MWDiag *m_diagPage;
      /// Saves if the user specified their own name for the SPE output file
      bool m_saveChanged;
      bool m_isPyInitialized;
      bool m_backgroundWasVisible;
      bool m_absEiDirty;
      QHash<const QWidget * const, QLabel *> m_validators;
      QSettings m_prev;
      QButtonGroup * m_saveChecksGroup;


    };
  }
}

#endif // MANTIDQTCUSTOMINTERFACES_HOMER_H_
