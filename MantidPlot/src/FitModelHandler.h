// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
/***************************************************************************
    File                 : FitModelHandler.h
    Project              : QtiPlot
    --------------------------------------------------------------------
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the Free Software           *
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor,                    *
 *   Boston, MA  02110-1301  USA                                           *
 *                                                                         *
 ***************************************************************************/
#ifndef FITMODELHANDLER_H
#define FITMODELHANDLER_H

#include <QVarLengthArray>
#include <QXmlDefaultHandler>

class Fit;

class FitModelHandler : public QXmlDefaultHandler {
public:
  explicit FitModelHandler(Fit *fit);

  bool startElement(const QString &namespaceURI, const QString &localName,
                    const QString &qName,
                    const QXmlAttributes &attributes) override;
  bool endElement(const QString &namespaceURI, const QString &localName,
                  const QString &qName) override;
  bool characters(const QString &str) override;
  bool fatalError(const QXmlParseException &) override { return false; };
  QString errorString() const override;

private:
  Fit *d_fit;
  bool metFitTag;
  QString currentText;
  QString errorStr;
  QString d_formula;
  QStringList d_parameters;
  QStringList d_explanations;
  QVarLengthArray<double> d_values;
};

#endif
