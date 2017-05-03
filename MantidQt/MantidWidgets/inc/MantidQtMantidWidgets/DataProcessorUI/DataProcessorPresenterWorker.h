#ifndef MANTIDQTCUSTOMINTERFACES_DATAPROCESSORPRESENTER_DATAPROCESSORPRESENTERWORKER_H_
#define MANTIDQTCUSTOMINTERFACES_DATAPROCESSORPRESENTER_DATAPROCESSORPRESENTERWORKER_H_

#include "MantidQtMantidWidgets/WidgetDllOption.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorPresenter.h"
#include "MantidKernel/Logger.h"

#include <QThread>
#include <stdexcept>

namespace MantidQt {
namespace MantidWidgets {

/**
Worker to run the DataProcessorPesenter asynchronously from the GUI
that invoked it.

Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory, NScD
Oak Ridge National Laboratory & European Spallation Source

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
class EXPORT_OPT_MANTIDQT_MANTIDWIDGETS DataProcessorPresenterWorker : public QObject {
  Q_OBJECT

public:
  DataProcessorPresenterWorker(DataProcessorPresenter *presenter)
      : m_presenter(presenter) {}

public slots:

  void autoreduce() {
    m_presenter->notify(DataProcessorPresenter::SelectAllGroupsFlag);
    m_presenter->notify(DataProcessorPresenter::ProcessFlag);
    emit finished();
  }

signals:
  void finished();

private:
  DataProcessorPresenter *m_presenter;
};

} // namespace CustomInterfaces
} // namespace MantidQt

#endif // MANTIDQTCUSTOMINTERFACES_DATAPROCESSORPRESENTER_DATAPROCESSORPRESENTERWORKER_H_
