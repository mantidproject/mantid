/***************************************************************************
    File                 : Note.cpp
    Project              : QtiPlot
    --------------------------------------------------------------------
    Copyright            : (C) 2006 by Ion Vasilief,
                           Tilman Hoener zu Siederdissen,
					  Knut Franke
    Email (use @ for *)  : ion_vasilief*yahoo.fr, thzs*gmx.net,
                           knut.franke*gmx.de
    Description          : Notes window class

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
#include "Note.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QTextStream>
#include <Qsci/qsciprinter.h>
#include <QPrintDialog>
#include "MantidKernel/ConfigService.h"

Note::Note(const QString& label, ApplicationWindow* parent, const QString& name, Qt::WFlags f)
  : MdiSubWindow(parent, label, name, f)
{
  init();
}

void Note::init()
{
  te = new QTextEdit(this, name());
  setWidget(te);

  setGeometry(0, 0, 500, 200);
  connect(te, SIGNAL(textChanged()), this, SLOT(modifiedNote()));
}

void Note::setName(const QString& name)
{
  te->setObjectName(name);
  MdiSubWindow::setName(name);
}

void Note::modifiedNote()
{
  emit modifiedWindow(this);
}

QString Note::saveToString(const QString &info, bool)
{
  QString s= "<note>\n";
  s+= QString(name()) + "\t" + birthDate() + "\n";
  s+= info;
  s+= "WindowLabel\t" + windowLabel() + "\t" + QString::number(captionPolicy()) + "\n";
  s+= "<content>\n"+te->text().stripWhiteSpace()+"\n</content>";
  s+="\n</note>\n";
  return s;
}

void Note::restore(const QStringList& data)
{
  QStringList::ConstIterator line = data.begin();
  QStringList fields;

  fields = (*line).split("\t");
  // Needed for backwards compatability
  if (fields[0] == "AutoExec"){
    line++;
  }

  if (*line == "<content>") line++;
  while (line != data.end() && *line != "</content>")
    te->append((*line++)+"\n");  //Mantid - changes for QScintilla
}

void Note::print()
{
  QsciPrinter printer(QPrinter::HighResolution);
  printer.setColorMode(QPrinter::GrayScale);
  printer.setOutputFormat(QPrinter::PostScriptFormat);
  QPrintDialog printDialog(&printer);
  printDialog.setWindowTitle("MantidPlot - Print Note");
  if (printDialog.exec() == QDialog::Accepted)
  {
    te->document()->print(&printer);
  }
}

void Note::exportPDF(const QString& fileName)
{
  QPrinter printer;
  printer.setColorMode(QPrinter::GrayScale);
  printer.setCreator("MantidPlot");
  printer.setOutputFormat(QPrinter::PdfFormat);
  printer.setOutputFileName(fileName);
  te->document()->print(&printer);
}

QString Note::exportASCII(const QString &filename)
{
  QString filter;
  filter += tr("Text") + " (*.txt *.TXT);;";
  filter += tr("All Files")+" (*)";

  QString selectedFilter;
  QString fn;
  if (filename.isEmpty())
  {
    QString dir(Mantid::Kernel::ConfigService::Instance().getString("defaultsave.directory").c_str());
    fn = QFileDialog::getSaveFileName(this, tr("Save Text to File"),dir,filter, &selectedFilter);
  }
  else
    fn = filename;

  if ( !fn.isEmpty() ){
    QFileInfo fi(fn);

    QString baseName = fi.fileName();
    if (!baseName.contains(".")){
      if (selectedFilter.contains(".txt"))
        fn.append(".txt");
      else if (selectedFilter.contains(".py"))
        fn.append(".py");
    }

    QFile f(fn);
    if (!f.open(IO_WriteOnly)){
      QMessageBox::critical(0, tr("MantidPlot - File Save Error"),
            tr("Could not write to file: <br><h4> %1 </h4><p>Please verify that you have the right to write to this location!").arg(fn));
      return QString::null;
    }

    QTextStream t( &f );
    t.setEncoding(QTextStream::UnicodeUTF8);
    t << text();
    f.close();
  }
  return fn;
}
