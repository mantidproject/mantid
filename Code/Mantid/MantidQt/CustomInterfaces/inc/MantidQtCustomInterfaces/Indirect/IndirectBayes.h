#ifndef MANTIDQTCUSTOMINTERFACES_INDIRECTBAYES_H_
#define MANTIDQTCUSTOMINTERFACES_INDIRECTBAYES_H_

//----------------------
// Includes
//----------------------
#include "ui_IndirectBayes.h"
#include "MantidQtAPI/UserSubWindow.h"
#include "IndirectBayesTab.h"

#include <Poco/NObserver.h>
#include "MantidKernel/ConfigService.h"

namespace MantidQt
{
  namespace CustomInterfaces
  {
    /** 
    This class defines the Indirect Bayes interface. It handles the creation of the interface window and 
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

    class DLLExport IndirectBayes : public MantidQt::API::UserSubWindow
    {
      Q_OBJECT

		public: //public constants and enums

      /// Enumeration for the index of each tab
			enum TabChoice
			{
				RES_NORM,
				QUASI,
				STRETCH,
				JUMP_FIT
			};

    public: // public constructor, destructor and functions
      /// Default Constructor
      IndirectBayes(QWidget *parent = 0);
      ///Destructor
      ~IndirectBayes();
      /// Interface name
      static std::string name() { return "Bayes"; }
      // This interface's categories.
      static QString categoryInfo() { return "Indirect"; }
      virtual void initLayout();

    private slots:
      // Run the appropriate action depending based on the selected tab

      /// Slot for clicking on the run button
      void runClicked();
      /// Slot for clicking on the hlep button
      void helpClicked();
      /// Slot for clicking on the manage directories button
      void manageUserDirectories();
      /// Slot showing a message box to the user
      void showMessageBox(const QString& message);

		private:
      /// Called upon a close event.
      virtual void closeEvent(QCloseEvent*);
      /// handle POCO event
      void handleDirectoryChange(Mantid::Kernel::ConfigValChangeNotification_ptr pNf);
      /// Load default interface settings for each tab
      void loadSettings();

      /// Map of tabs indexed by position on the window
			std::map<unsigned int, IndirectBayesTab*> m_bayesTabs;
      /// Change Observer for ConfigService (monitors user directories)
      Poco::NObserver<IndirectBayes, Mantid::Kernel::ConfigValChangeNotification> m_changeObserver;
      ///Main interface window
      Ui::IndirectBayes m_uiForm;
    };
  }
}

#endif
