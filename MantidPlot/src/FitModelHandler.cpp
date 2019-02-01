// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
/***************************************************************************
    File                 : FitModelHandler.cpp
    Project              : QtiPlot
    --------------------------------------------------------------------
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the Free Software           *
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor,                    *
 *   Boston, MA  02110-1301  USA                                           *
 *                                                                         *
 ***************************************************************************/
#include "FitModelHandler.h"
#include "Fit.h"

#include <QMessageBox>

FitModelHandler::FitModelHandler(Fit *fit) : d_fit(fit) { metFitTag = false; }

bool FitModelHandler::startElement(const QString & /* namespaceURI */,
                                   const QString & /* localName */,
                                   const QString &qName,
                                   const QXmlAttributes &attributes) {
  if (!metFitTag && qName != "fit") {
    errorStr = QObject::tr("The file is not an QtiPlot fit model file.");
    return false;
  }

  if (qName == "fit") {
    QString version = attributes.value("version");
    if (!version.isEmpty() && version != "1.0") {
      errorStr =
          QObject::tr("The file is not an QtiPlot fit model version 1.0 file.");
      return false;
    }
    metFitTag = true;
  }

  currentText.clear();
  return true;
}

bool FitModelHandler::endElement(const QString & /* namespaceURI */,
                                 const QString & /* localName */,
                                 const QString &qName) {
  if (qName == "model")
    d_fit->setObjectName(currentText);
  else if (qName == "type")
    d_fit->setType((Fit::FitType)currentText.toInt());
  else if (qName == "function")
    d_formula = currentText.replace("&lt;", "<").replace("&gt;", ">");
  else if (qName == "name" && !currentText.isEmpty())
    d_parameters << currentText;
  else if (qName == "explanation")
    d_explanations << currentText;
  else if (qName == "value")
    d_values.append(currentText.toDouble());
  else if (qName == "fit") {
    d_fit->setParametersList(d_parameters);
    d_fit->setFormula(d_formula);
    d_fit->setInitialGuesses(d_values.data());
    d_fit->setParameterExplanations(d_explanations);
  }
  return true;
}

bool FitModelHandler::characters(const QString &str) {
  currentText += str;
  return true;
}

QString FitModelHandler::errorString() const { return errorStr; }
