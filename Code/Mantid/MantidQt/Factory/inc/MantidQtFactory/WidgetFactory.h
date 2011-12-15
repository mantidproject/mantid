#ifndef MANTID_FACTORY_WIDGETFACTORY_H_
#define MANTID_FACTORY_WIDGETFACTORY_H_

#include "DllOption.h"
#include "MantidKernel/System.h"
#include "MantidKernel/SingletonHolder.h"
#include "MantidQtSliceViewer/SliceViewer.h"
#include "MantidQtSliceViewer/SliceViewerWindow.h"
#include "qapplication.h"

namespace MantidQt
{
namespace Factory
{

  /** Factory class that handles the creation
   * of MantidQt widgets such as the SliceViewer.
   * This allows both C++ and Python to create these widgets
   * and both to handle them in a uniform way.
    
    @date 2011-12-15

    Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
  class EXPORT_OPT_MANTIDQT_FACTORY WidgetFactoryImpl
  {
  public:
    WidgetFactoryImpl();
    virtual ~WidgetFactoryImpl();
    
    MantidQt::SliceViewer::SliceViewerWindow* createSliceViewerWindow(const QString& wsName, const QString& label);
    MantidQt::SliceViewer::SliceViewer* createSliceViewer(const QString& wsName);

  };



} // namespace Factory
} // namespace MantidQt

/// Explicitely declare the WidgetFactory singleton
template class EXPORT_OPT_MANTIDQT_FACTORY Mantid::Kernel::SingletonHolder<MantidQt::Factory::WidgetFactoryImpl>;
typedef EXPORT_OPT_MANTIDQT_FACTORY Mantid::Kernel::SingletonHolder<MantidQt::Factory::WidgetFactoryImpl> WidgetFactory;


#endif  /* MANTID_FACTORY_WIDGETFACTORY_H_ */
