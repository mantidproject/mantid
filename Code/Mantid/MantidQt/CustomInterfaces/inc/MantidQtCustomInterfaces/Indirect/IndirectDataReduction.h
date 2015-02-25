#ifndef MANTIDQTCUSTOMINTERFACES_INDIRECTDATAREDUCTION_H_
#define MANTIDQTCUSTOMINTERFACES_INDIRECTDATAREDUCTION_H_

//----------------------
// Includes
//----------------------
#include "ui_IndirectDataReduction.h"

#include "MantidQtAPI/AlgorithmRunner.h"
#include "MantidQtAPI/UserSubWindow.h"
#include "MantidQtCustomInterfaces/Indirect/IndirectDataReductionTab.h"

#include <QScrollArea>


namespace MantidQt
{
  namespace CustomInterfaces
  {
    //-------------------------------------------
    // Forward declarations
    //-------------------------------------------
    class IndirectDataReductionTab;

    /**
    This class defines the IndirectDataReduction interface. It handles the overall instrument settings
    and sets up the appropriate interface depending on the deltaE mode of the instrument. The deltaE
    mode is defined in the instrument definition file using the "deltaE-mode".

    @author Martyn Gigg, Tessella Support Services plc
    @author Michael Whitty

    Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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

    class IndirectDataReduction : public MantidQt::API::UserSubWindow
    {
      Q_OBJECT

    public:
      /// Default Constructor
      IndirectDataReduction(QWidget *parent = 0);
      ///Destructor
      ~IndirectDataReduction();
      /// Interface name
      static std::string name() { return "Data Reduction"; }
      // This interface's categories.
      static QString categoryInfo() { return "Indirect"; }

      /// Initialize the layout
      virtual void initLayout();
      /// Run Python-based initialisation commands
      virtual void initLocalPython();

      /// Handled configuration changes
      void handleConfigChange(Mantid::Kernel::ConfigValChangeNotification_ptr pNf);

      Mantid::API::MatrixWorkspace_sptr loadInstrumentIfNotExist(std::string instrumentName,
          std::string analyser = "", std::string reflection = "");

      std::vector<std::pair<std::string, std::vector<std::string> > > getInstrumentModes();
      std::map<QString, QString> getInstrumentDetails();

    signals:
      /// Emitted when the instrument setup is changed
      void newInstrumentConfiguration();

    private slots:
      /// Shows/hides tabs based on facility
      void filterUiForFacility(QString facility);
      /// Opens the help page for the current tab
      void helpClicked();
      /// Exports the current tab algorithms as a Python script
      void exportTabPython();
      /// Runs the current tab
      void runClicked();
      /// Opens the manage directory dialog
      void openDirectoryDialog();

      /// Shows a information dialog box
      void showMessageBox(const QString& message);
      /// Updates the state of the Run button
      void updateRunButton(bool enabled = true, QString message = "Run", QString tooltip = "");

      /// Called when the load instrument algorithms complete
      void instrumentLoadingDone(bool error);

      /// Called when the instrument setup has been changed
      void instrumentSetupChanged(const QString & instrumentName, const QString & analyser, const QString & reflection);

    private:
      QString getInstrumentParameterFrom(Mantid::Geometry::IComponent_const_sptr comp, std::string param);

      void readSettings();
      void saveSettings();

      /// Set and show an instrument-specific widget
      virtual void closeEvent(QCloseEvent* close);

      template <typename T>
      void addTab(const QString & name)
      {
        QWidget * tabWidget = new QWidget(m_uiForm.twIDRTabs);
        QVBoxLayout * tabLayout = new QVBoxLayout(tabWidget);
        tabWidget->setLayout(tabLayout);

        QScrollArea * tabScrollArea = new QScrollArea(tabWidget);
        tabLayout->addWidget(tabScrollArea);
        tabScrollArea->setWidgetResizable(true);

        QWidget * tabContent = new QWidget(tabScrollArea);
        tabScrollArea->setWidget(tabContent);
        tabScrollArea->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

        IndirectDataReductionTab * tabIDRContent = new T(this, tabContent);
        tabIDRContent->setupTab();
        tabContent->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

        connect(tabIDRContent, SIGNAL(runAsPythonScript(const QString&, bool)), this, SIGNAL(runAsPythonScript(const QString&, bool)));
        connect(tabIDRContent, SIGNAL(showMessageBox(const QString&)), this, SLOT(showMessageBox(const QString&)));
        connect(tabIDRContent, SIGNAL(updateRunButton(bool, QString, QString)), this, SLOT(updateRunButton(bool, QString, QString)));

        m_tabs[name] = qMakePair(tabWidget, tabIDRContent);
      }

      friend class IndirectDataReductionTab;
      /// The .ui form generated by Qt Designer
      Ui::IndirectDataReduction m_uiForm;
      /// Instrument the interface is currently set for.
      QString m_instrument;
      /// The settings group
      QString m_settingsGroup;
      /// Runner for insturment load algorithm
      MantidQt::API::AlgorithmRunner* m_algRunner;

      // All indirect tabs
      QMap<QString, QPair<QWidget *, IndirectDataReductionTab *>> m_tabs;

      /// Poco observer for changes in user directory settings
      Poco::NObserver<IndirectDataReduction, Mantid::Kernel::ConfigValChangeNotification> m_changeObserver;
      QString m_dataDir; ///< default data search directory
      QString m_saveDir; ///< default data save directory

      // Pointer to the current empty instrument workspace
      Mantid::API::MatrixWorkspace_sptr m_instWorkspace;
    };

  }
}

#endif //MANTIDQTCUSTOMINTERFACES_INDIRECTDATAREDUCTION_H_
