#ifndef MANTIDQT_API_HELPWINDOWINTERFACE_H_
#define MANTIDQT_API_HELPWINDOWINTERFACE_H_

#include "DllOption.h"
#include <QWidget>
#include <string>

class QString;
class QUrl;

namespace MantidQt
{
namespace API
{

/**
 *
  This class is an interface for the central widget for handling VATES visualization
  operations. Its main use is for the plugin mode operation of the viewer.

  Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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
class EXPORT_OPT_MANTIDQT_API MantidHelpInterface : public QWidget
{
  Q_OBJECT
public:
  /// Default constructor
  MantidHelpInterface();
  /// Default destructor.
  virtual ~MantidHelpInterface();

  virtual void showPage(const std::string & url=std::string());
  virtual void showPage(const QString & url);
  virtual void showPage(const QUrl & url);
  virtual void showWikiPage(const std::string &page=std::string());
  virtual void showAlgorithm(const std::string &name=std::string(), const int version=-1);
  virtual void showAlgorithm(const QString &name, const int version=-1);
  virtual void showConcept(const std::string &name);
  virtual void showConcept(const QString &name);
  virtual void showFitFunction(const std::string &name=std::string());

public slots:
  /// Perform any clean up on main window shutdown
  virtual void shutdown();
};

}
}

#endif // MANTIDQT_API_HELPWINDOWINTERFACE_H_
