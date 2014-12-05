#ifndef MANTIDQTCUSTOMINTERFACES_INDIRECTTOOLS_H_
#define MANTIDQTCUSTOMINTERFACES_INDIRECTTOOLS_H_

//----------------------
// Includes
//----------------------
#include "ui_IndirectTools.h"
#include "MantidKernel/ConfigService.h"
#include "MantidQtAPI/UserSubWindow.h"
#include "MantidQtCustomInterfaces/IndirectToolsTab.h"

#include <Poco/NObserver.h>

namespace MantidQt
{
  namespace CustomInterfaces
  {
    /**
    This class defines the Indirect Foreign interface. It handles the creation of the interface window and
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

    class DLLExport IndirectTools : public MantidQt::API::UserSubWindow
    {
      Q_OBJECT

		public: //public constants and enums

      /// Enumeration for the index of each tab
			enum TabChoice
			{
        TRANSMISSION,
				LOAD_ILL
			};

    public: // public constructor, destructor and functions
      /// Default Constructor
      IndirectTools(QWidget *parent = 0);
      ///Destructor
      ~IndirectTools();
      /// Interface name
      static std::string name() { return "Tools"; }
      // This interface's categories.
      static QString categoryInfo() { return "Indirect"; }

      virtual void initLayout();

    private slots:
      /// Slot for clicking on the run button
      void runClicked();
      /// Slot for clicking on the hlep button
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
      /// Handle POCO event
      void handleDirectoryChange(Mantid::Kernel::ConfigValChangeNotification_ptr pNf);

      /// Map of tabs indexed by position on the window
			std::map<unsigned int, IndirectToolsTab*> m_tabs;
      /// Change Observer for ConfigService (monitors user directories)
      Poco::NObserver<IndirectTools, Mantid::Kernel::ConfigValChangeNotification> m_changeObserver;
      /// Main interface window
      Ui::IndirectTools m_uiForm;
    };
  }
}

#endif
