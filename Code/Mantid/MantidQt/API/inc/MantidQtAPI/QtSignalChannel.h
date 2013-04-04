#ifndef MANTIDQTAPI_QTSIGNALCHANNEL_H_
#define MANTIDQTAPI_QTSIGNALCHANNEL_H_

#include "DllOption.h"

#include <QObject>
#include <Poco/Channel.h>

namespace MantidQt
{
  namespace API
  {
    /** 
    Provides a translation layer that takes a Poco::Message and converts it
    to a Qt signal.

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
    class EXPORT_OPT_MANTIDQT_API QtSignalChannel : public QObject, public Poco::Channel
    {
    Q_OBJECT

    public:
      /// Constructor
      QtSignalChannel();
      /// Destructor
      ~QtSignalChannel();

      /// Converts the Poco::Message to a Qt signal
      void log(const Poco::Message& msg);

    public slots:
      /// Set the log level for all loggers
      void setGlobalLogLevel(int level);

    signals:
      // Emitted when a Poco log message is received in this channel
      void messageReceived(const QString & msg);

    private:
      Q_DISABLE_COPY(QtSignalChannel);
   };
  }
}

#endif //MANTIDQTAPI_MANTIDWIDGET_H_
