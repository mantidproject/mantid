#ifndef MANTIDQTCUSTOMINTERFACES_DYNAMICPDF_DPDFFITBROWSER_H_
#define MANTIDQTCUSTOMINTERFACES_DYNAMICPDF_DPDFFITPROPERTYBROWSER_H_

#include "MantidQtCustomInterfaces/DllConfig.h"
#include "MantidQtMantidWidgets/FitPropertyBrowser.h"

// forward declarations
namespace MantidQt {
namespace MantidWidgets {
  class FitPropertyBrowser;
}
}

/**
This is a Helper class for the DynamicPDF custom interface. In particular this
helper class defines all properties to carry out the fits that remove the
multi-phonon contributions to the Q-slices.

Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
National Laboratory & European Spallation Source

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
namespace MantidQt {
namespace CustomInterfaces {
namespace DynamicPDF {

class MANTIDQT_CUSTOMINTERFACES_DLL DPDFFitPropertyBrowser
    : public MantidQt::MantidWidgets::FitPropertyBrowser {
  Q_OBJECT

public:
  DPDFFitPropertyBrowser(QWidget *parent = NULL, QObject* mantidui = NULL);
  void init() override;

private:
  void initProperties();
  void createModelPropertyGroup(QWidget *w);
  void initializeFunctionObserver();

}; // class DPDFFitPropertyBrowser

} // namespace DynamicPDF
} // namespace CustomInterfaces
} // namespace MantidQt
#endif // MANTIDQTCUSTOMINTERFACES_DYNAMICPDF_DPDFFITPROPERTYBROWSER_H_
