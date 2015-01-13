#ifndef SIMPLEGUIAPP_H
#define SIMPLEGUIAPP_H

#include <QApplication>

class pqPVApplicationCore;
/**
 *
 This is a wrapper to QApplication in order to handle exceptions and present
 them to the user in dialog boxes for better error reporting.

 @author Michael Reuter
 @date 04/08/2011

 Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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
class SimpleGuiApp : public QApplication
{
  Q_OBJECT
public:
  /**
   * Object constructor.
   * @param argc number of command-line arguments
   * @param argv list of command-line agruments
   */
  SimpleGuiApp(int &argc, char **argv);
  /**
   * Intercept exceptions and print them in dialog boxes.
   * @param receiver receipient of an event
   * @param event the event to pass to the receiver
   * @return value from the receiver's event handler
   */
  virtual bool notify(QObject *receiver, QEvent *event);
private:
  pqPVApplicationCore *pvApp; ///< ParaView application engine

};

#endif // SIMPLEGUIAPP_H
