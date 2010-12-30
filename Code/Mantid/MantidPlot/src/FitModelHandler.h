/***************************************************************************
    File                 : FitModelHandler.h
    Project              : QtiPlot
    --------------------------------------------------------------------
    Copyright            : (C) 2007 by Ion Vasilief
    Email (use @ for *)  : ion_vasilief*yahoo.fr
    Description          : An XML handler for the Fit class

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *  This program is free software; you can redistribute it and/or modify   *
 *  it under the terms of the GNU General Public License as published by   *
 *  the Free Software Foundation; either version 2 of the License, or      *
 *  (at your option) any later version.                                    *
 *                                                                         *
 *  This program is distributed in the hope that it will be useful,        *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *  GNU General Public License for more details.                           *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the Free Software           *
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor,                    *
 *   Boston, MA  02110-1301  USA                                           *
 *                                                                         *
 ***************************************************************************/
#ifndef FITMODELHANDLER_H
#define FITMODELHANDLER_H

#include <QXmlDefaultHandler>
#include <QVarLengthArray>

class Fit;

class FitModelHandler : public QXmlDefaultHandler
{
public:
    FitModelHandler(Fit *fit);

    bool startElement(const QString &namespaceURI, const QString &localName,
                       const QString &qName, const QXmlAttributes &attributes);
    bool endElement(const QString &namespaceURI, const QString &localName,
                     const QString &qName);
    bool characters(const QString &str);
    bool fatalError(const QXmlParseException &){return false;};
    QString errorString() const;

private:
    Fit* d_fit;
    bool metFitTag;
    QString currentText;
    QString errorStr;
    QString d_formula;
    QStringList d_parameters;
    QStringList d_explanations;
    QVarLengthArray<double> d_values;
};

#endif
