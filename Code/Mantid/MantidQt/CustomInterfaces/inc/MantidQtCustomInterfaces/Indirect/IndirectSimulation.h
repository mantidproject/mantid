#ifndef MANTIDQTCUSTOMINTERFACES_INDIRECTSIMULATION_H_
#define MANTIDQTCUSTOMINTERFACES_INDIRECTSIMULATION_H_

//----------------------
// Includes
//----------------------
#include "ui_IndirectSimulation.h"
#include "MantidKernel/ConfigService.h"
#include "MantidQtAPI/UserSubWindow.h"
#include "IndirectSimulationTab.h"

#include <Poco/NObserver.h>

namespace MantidQt
{
  namespace CustomInterfaces
  {
    /**
    This class defines the Indirect Simulation interface. It handles the creation of the interface window and
		handles the interaction between the child tabs on the window.

    @author Samuel Jackson, STFC

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

    class DLLExport IndirectSimulation : public MantidQt::API::UserSubWindow
    {
      Q_OBJECT

		public: //public constants and enums

      /// Enumeration for the index of each tab
			enum TabChoice
			{
				MOLDYN,
        SASSENA,
        DOS
			};

    public: // public constructor, destructor and functions
      /// Default Constructor
      IndirectSimulation(QWidget *parent = 0);
      /// Destructor
      ~IndirectSimulation();
      /// Interface name
      static std::string name() { return "Simulation"; }
      /// This interface's categories.
      static QString categoryInfo() { return "Indirect"; }
      /// Setup tab UI
      virtual void initLayout();

    private slots:
      /// Slot for clicking on the run button
      void runClicked();
      /// Slot for clicking on the help button
      void helpClicked();
      /// Slot for clicking on the manage directories button
      void manageUserDirectories();
      /// Slot showing a message box to the user
      void showMessageBox(const QString& message);

		private:
      /// Load default interface settings for each tab
      void loadSettings();
      /// Called upon a close event.
      virtual void closeEvent(QCloseEvent*);
      /// handle POCO event
      void handleDirectoryChange(Mantid::Kernel::ConfigValChangeNotification_ptr pNf);

      /// Map of tabs indexed by position on the window
			std::map<unsigned int, IndirectSimulationTab*> m_simulationTabs;
      /// Change Observer for ConfigService (monitors user directories)
      Poco::NObserver<IndirectSimulation, Mantid::Kernel::ConfigValChangeNotification> m_changeObserver;
      ///Main interface window
      Ui::IndirectSimulation m_uiForm;

    };
  }
}

#endif
