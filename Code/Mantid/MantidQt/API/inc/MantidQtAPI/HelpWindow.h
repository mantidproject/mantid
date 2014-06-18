#ifndef HELPWINDOW_H
#define HELPWINDOW_H

#include "MantidQtAPI/DllOption.h"
#include <string>

// forward declarations
class QString;
class QUrl;
class QWidget;

namespace MantidQt
{
namespace API
{

/**
  Copyright &copy; 2013-14 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

  File change history is stored at: <https://github.com/mantidproject/mantid>.
  Code Documentation is available at: <http://doxygen.mantidproject.org>
 */
class EXPORT_OPT_MANTIDQT_API HelpWindow
{
public:
  static void showPage(const std::string & url=std::string());
  static void showPage(const QString & url);
  static void showPage(const QUrl & url);
  static void showWikiPage(const std::string &page=std::string());
  static void showAlgorithm(const std::string &name=std::string(), const int version=-1, QWidget *parent=0);
  static void showAlgorithm(const QString &name, const int version=-1, QWidget *parent=0);
  static void showFitFunction(const std::string &name=std::string());
};
} // namespace API
} // namespace MantidQt
#endif // HELPWINDOW_H
