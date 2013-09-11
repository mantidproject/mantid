#ifndef MANTID_CUSTOMINTERFACES_INDIRECTBAYESTAB_H_
#define MANTID_CUSTOMINTERFACES_INDIRECTBAYESTAB_H_

#include "MantidKernel/System.h"

#include <QMap>
#include <QtDoublePropertyManager>
#include <QtTreePropertyBrowser>
#include <QWidget>

#include <qwt_plot.h>
#include <qwt_plot_curve.h>


namespace MantidQt
{
	namespace CustomInterfaces
	{
		/**
			This class defines a abstract base class for the different tabs of the Indirect Bayes interface.
			Any joint functionality shared between each of the tabs should be implemented here as well as defining
			shared member functions.
    
			@author Samuel Jackson, STFC

			Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

		/// precision of double properties in bayes tabs
		static const unsigned int NUM_DECIMALS = 6;

		class DLLExport IndirectBayesTab : public QWidget
		{
			Q_OBJECT

		public:
			IndirectBayesTab(QWidget * parent = 0);
			~IndirectBayesTab();

			virtual void help() = 0;
			virtual void validate() = 0;
			virtual void run() = 0;

		};
	} // namespace CustomInterfaces
} // namespace Mantid

#endif