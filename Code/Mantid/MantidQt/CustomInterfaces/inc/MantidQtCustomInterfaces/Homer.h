#ifndef MANTIDQTCUSTOMINTERFACES_HOMER_H_
#define MANTIDQTCUSTOMINTERFACES_HOMER_H_

#include "ui_DirectConvertToEnergy.h"
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
  namespace MantidWidgets
  {
    //-----------------------------------------------------
    // Forward declarations
    //-----------------------------------------------------
    class MWDiag;
  }

  namespace CustomInterfaces
  {
    //-----------------------------------------------------
    // Forward declarations
    //-----------------------------------------------------
    class Background;

    /** 
    This class implements the DirectConvertToEnergy interface for the direct instruments

    @author Steve Williams, ISIS Computing Group, RAL
    @author Martyn Gigg, Tessella Support Services plc
    @date 24/02/2009

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
    class Homer : public MantidQt::API::UserSubWindow
    {
      Q_OBJECT

    public:
      Homer(QWidget *parent, Ui::DirectConvertToEnergy & uiForm);

      // Initialize the layout
      virtual void initLayout();

      void runClicked();
      void helpClicked();
      void setIDFValues(const QString & prefix);  

    private:
      void showEvent(QShowEvent *event);
      void hideEvent(QHideEvent *event);
      void closeEvent(QCloseEvent *event);
      /// Enable the run button if the results dialog has been closed and the python has stopped
      void pythonIsRunning(bool running = true);
      QString defaultName();
      std::string insertNumber(const std::string &filename, const int number);

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

      bool runScripts();
      void readSettings();
      void saveSettings();
      QString getGeneralSettingsGroup() const;
      QString getInstrumentSettingsGroup() const;
      QString openFileDia(const bool save, const QStringList &exts);
      void syncBackgroundSettings();

    signals:
      // these signals send data to the find bad detectors (diag) widget
      void MWDiag_updateWBV(const QString &);
      void MWDiag_updateTOFs(const double &, const double &);
      void MWDiag_sendRuns(const QStringList&);

    private slots:
      void validateAbsEi(const QString &);
      void validateRunEi(const QString &);
      void validateRebinBox(const QString &);

      void browseSaveFile();

      void runFilesChanged();
      void updateVanadiumMapFile();
      void updateSaveName();
      void saveNameUpd();
      void updateWBV();
      void bgRemoveClick();
      void bgRemoveReadSets();
      void saveFormatOptionClicked(QAbstractButton*);
      void updateAbsEi(const QString & text);
      void markAbsEiDirty(bool dirty = true);

    private:
      Ui::DirectConvertToEnergy m_uiForm;
      Background *m_backgroundDialog;
      /// A pointer to the widget with the user controls for finding bad detectors
      MantidWidgets::MWDiag *m_diagPage;

      /// Saves if the user specified their own name for the SPE output file
      bool m_saveChanged; 
      bool m_backgroundWasVisible;
      bool m_absEiDirty;
      QHash<const QWidget * const, QLabel *> m_validators;
      QButtonGroup *m_saveChecksGroup;
      QString m_topSettingsGroup;
      //@todo These all should be handled by the file widget
      QString m_lastSaveDir;
      QString m_lastLoadDir;
    };
  }
}

#endif // MANTIDQTCUSTOMINTERFACES_HOMER_H_
