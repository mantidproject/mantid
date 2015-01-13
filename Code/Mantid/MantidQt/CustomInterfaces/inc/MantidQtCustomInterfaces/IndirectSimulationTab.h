#ifndef MANTID_CUSTOMINTERFACES_INDIRECTSIMULATIONTAB_H_
#define MANTID_CUSTOMINTERFACES_INDIRECTSIMULATIONTAB_H_

#include "MantidAPI/AlgorithmManager.h"
#include "MantidKernel/System.h"
#include "MantidQtAPI/AlgorithmRunner.h"
#include "MantidQtCustomInterfaces/IndirectTab.h"
#include <QSettings>
#include <QWidget>

namespace MantidQt
{
	namespace CustomInterfaces
	{
		/**
			This class defines a abstract base class for the different tabs of the Indirect Simulation interface.
			Any joint functionality shared between each of the tabs should be implemented here as well as defining
			shared member functions.

			@author Samuel Jackson, STFC

			Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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

		class DLLExport IndirectSimulationTab : public IndirectTab
		{
			Q_OBJECT

		public:
			IndirectSimulationTab(QWidget * parent = 0);
			~IndirectSimulationTab();

			/// Returns a URL for the wiki help page for this interface
			QString tabHelpURL();

			virtual QString help() = 0;
			virtual void loadSettings(const QSettings& settings) = 0;

		};
	} // namespace CustomInterfaces
} // namespace Mantid

#endif
