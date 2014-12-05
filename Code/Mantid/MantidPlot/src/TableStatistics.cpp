/***************************************************************************
	File                 : TableStatistics.cpp
	Project              : QtiPlot
--------------------------------------------------------------------
	Copyright            : (C) 2006 by Knut Franke
	Email (use @ for *)  : knut.franke*gmx.de
	Description          : Table subclass that displays statistics on
	                       columns or rows of another table

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
#include "TableStatistics.h"

#include "ApplicationWindow.h"
#include "TSVSerialiser.h"
#include "MantidKernel/Strings.h"

#include <QList>
#include <gsl/gsl_vector.h>
#include <gsl/gsl_statistics.h>

#include <boost/algorithm/string.hpp>

TableStatistics::TableStatistics(ScriptingEnv *env, ApplicationWindow *parent, Table *base, Type t, QList<int> targets)
	: Table(env, 1, 1, "", parent, ""),
	d_base(base), d_type(t), d_targets(targets)
{
	// FIXME: Haven't found a set read-only method in Qt4 yet
	// d_table->setReadOnly(true);
	setCaptionPolicy(MdiSubWindow::Both);
	if (d_type == row)
	{
		setName(QString(d_base->objectName())+"-"+tr("RowStats"));
		setWindowLabel(tr("Row Statistics of %1").arg(base->objectName()));
		resizeRows(d_targets.size());
		resizeCols(9);
		setColName(0, tr("Row"));
		setColName(1, tr("Cols"));
		setColName(2, tr("Mean"));
		setColName(3, tr("StandardDev"));
		setColName(4, tr("Variance"));
		setColName(5, tr("Sum"));
		setColName(6, tr("Max"));
		setColName(7, tr("Min"));
		setColName(8, "N");

		for (int i=0; i < 9; i++)
			setColumnType(i, Text);

		for (int i=0; i < d_targets.size(); i++)
			setText(i, 0, QString::number(d_targets[i]+1));
		update(d_base, QString::null);
	}
	else if (d_type == column)
	{
		setName(QString(d_base->objectName())+"-"+tr("ColStats"));
		setWindowLabel(tr("Column Statistics of %1").arg(base->objectName()));
		resizeRows(d_targets.size());
		resizeCols(11);
		setColName(0, tr("Col"));
		setColName(1, tr("Rows"));
		setColName(2, tr("Mean"));
		setColName(3, tr("StandardDev"));
		setColName(4, tr("Variance"));
		setColName(5, tr("Sum"));
		setColName(6, tr("iMax"));
		setColName(7, tr("Max"));
		setColName(8, tr("iMin"));
		setColName(9, tr("Min"));
		setColName(10, "N");

		for (int i=0; i < 11; i++)
			setColumnType(i, Text);

		for (int i=0; i < d_targets.size(); i++)
		{
			setText(i, 0, d_base->colLabel(d_targets[i]));
			update(d_base, d_base->colName(d_targets[i]));
		}
	}
	int w=9*(d_table->horizontalHeader())->sectionSize(0);
	int h;
	if (numRows()>11)
		h=11*(d_table->verticalHeader())->sectionSize(0);
	else
		h=(numRows()+1)*(d_table->verticalHeader())->sectionSize(0);
	setGeometry(50,50,w + 45, h + 45);

	setColPlotDesignation(0, Table::X);
	setHeaderColType();
	
	connect(d_base, SIGNAL(modifiedData(Table*,const QString&)), this, SLOT(update(Table*,const QString&)));
	connect(d_base, SIGNAL(changedColHeader(const QString&, const QString&)), this, SLOT(renameCol(const QString&, const QString&)));
	connect(d_base, SIGNAL(removedCol(const QString&)), this, SLOT(removeCol(const QString&)));
	connect (d_base, SIGNAL(destroyed()), this, SLOT(closedBase()));
}

void TableStatistics::closedBase()
{
	d_base = NULL;
}

void TableStatistics::update(Table *t, const QString& colName)
{
	if (!d_base)
		return;
	
	if (t != d_base) return;

	int j;
	if (d_type == row)
		for (int r=0; r < d_targets.size(); r++)
		{
			int cols=d_base->numCols();
			int i = d_targets[r];
			int m = 0;
			for (j = 0; j < cols; j++)
				if (!d_base->text(i, j).isEmpty() && d_base->columnType(j) == Numeric)
					m++;

			if (!m)
			{//clear row statistics
				for (j = 1; j<9; j++)
					setText(r, j, QString::null);
			}

			if (m > 0)
			{
				double *dat = new double[m];
				gsl_vector *y = gsl_vector_alloc (m);
				int aux = 0;
				for (j = 0; j<cols; j++)
				{
					QString text = d_base->text(i,j);
					if (!text.isEmpty() && d_base->columnType(j) == Numeric)
					{
						double val = d_base->cell(i,j);
						gsl_vector_set (y, aux, val);
						dat[aux] = val;
						aux++;
					}
				}
				double mean = gsl_stats_mean (dat, 1, m);
				double min, max;
				gsl_vector_minmax (y, &min, &max);

				setText(r, 1, QString::number(d_base->numCols()));
				setText(r, 2, QString::number(mean));
				setText(r, 3, QString::number(gsl_stats_sd(dat, 1, m)));
				setText(r, 4, QString::number(gsl_stats_variance(dat, 1, m)));
				setText(r, 5, QString::number(mean*m));
				setText(r, 6, QString::number(max));
				setText(r, 7, QString::number(min));
				setText(r, 8, QString::number(m));

				gsl_vector_free (y);
				delete[] dat;
			}
		}
	else if (d_type == column)
		for (int c=0; c < d_targets.size(); c++)
			if (colName == QString(d_base->objectName())+"_"+text(c, 0))
			{
				int i = d_base->colIndex(colName);
				if (d_base->columnType(i) != Numeric) return;

				int rows = d_base->numRows();
				int start = -1, m = 0;
				for (j=0; j<rows; j++)
					if (!d_base->text(j,i).isEmpty())
					{
						m++;
						if (start<0) start=j;
					}

				if (!m)
				{//clear col statistics
					for (j = 1; j<11; j++)
						setText(c, j, QString::null);
					return;
				}

				if (start<0) return;

				double *dat = new double[m];
				gsl_vector *y = gsl_vector_alloc (m);

				int aux = 0, min_index = start, max_index = start;
				double val = d_base->cell(start, i);
				gsl_vector_set (y, 0, val);
				dat[0] = val;
				double min = val, max = val;
				for (j = start + 1; j<rows; j++)
				{
					if (!d_base->text(j, i).isEmpty())
					{
						aux++;
						val = d_base->cell(j, i);
						gsl_vector_set (y, aux, val);
						dat[aux] = val;
						if (val < min)
						{
							min = val;
							min_index = j;
						}
						if (val > max)
						{
							max = val;
							max_index = j;
						}
					}
				}
				double mean=gsl_stats_mean (dat, 1, m);

				setText(c, 1, "[1:"+QString::number(rows)+"]");
				setText(c, 2, QString::number(mean));
				setText(c, 3, QString::number(gsl_stats_sd(dat, 1, m)));
				setText(c, 4, QString::number(gsl_stats_variance(dat, 1, m)));
				setText(c, 5, QString::number(mean*m));
				setText(c, 6, QString::number(max_index + 1));
				setText(c, 7, QString::number(max));
				setText(c, 8, QString::number(min_index + 1));
				setText(c, 9, QString::number(min));
				setText(c, 10, QString::number(m));

				gsl_vector_free (y);
				delete[] dat;
			}

	for (int i=0; i<numCols(); i++)
		emit modifiedData(this, Table::colName(i));
}

void TableStatistics::renameCol(const QString &from, const QString &to)
{
	if (!d_base)
		return;
	
	if (d_type == row) return;
	for (int c=0; c < d_targets.size(); c++)
		if (from == QString(d_base->objectName())+"_"+text(c, 0))
		{
			setText(c, 0, to.section('_', 1, 1));
			return;
		}
}

void TableStatistics::removeCol(const QString &col)
{
	if (!d_base)
		return;
	
	if (d_type == row)
	{
		update(d_base, col);
		return;
	}
	for (int c=0; c < d_targets.size(); c++)
		if (col == QString(d_base->objectName())+"_"+text(c, 0))
		{
			d_targets.remove(d_targets.at(c));
			d_table->removeRow(c);
			return;
		}
}

void TableStatistics::loadFromProject(const std::string& lines, ApplicationWindow* app, const int fileVersion)
{
  Q_UNUSED(fileVersion);

  TSVSerialiser tsv(lines);

  if(tsv.selectLine("geometry"))
    app->restoreWindowGeometry(app, this, QString::fromStdString(tsv.lineAsString("geometry")));

  if(tsv.selectLine("header"))
  {
    QStringList header = QString::fromUtf8(tsv.lineAsString("header").c_str()).split("\t");
    header.pop_front();
    loadHeader(header);
  }

  if(tsv.selectLine("ColWidth"))
  {
    QStringList colWidths = QString::fromUtf8(tsv.lineAsString("ColWidth").c_str()).split("\t");
    colWidths.pop_front();
    setColWidths(colWidths);
  }

  if(tsv.selectLine("ColType"))
  {
    QStringList colTypes = QString::fromUtf8(tsv.lineAsString("ColType").c_str()).split("\t");
    colTypes.pop_front();
    setColumnTypes(colTypes);
  }

  if(tsv.selectLine("Comments"))
  {
    QStringList comments = QString::fromUtf8(tsv.lineAsString("Comments").c_str()).split("\t");
    comments.pop_front();
    setColComments(comments);
  }

  if(tsv.selectLine("WindowLabel"))
  {
    QString caption;
    int policy;
    tsv >> caption >> policy;
    setWindowLabel(caption);
    setCaptionPolicy((MdiSubWindow::CaptionPolicy)policy);
  }

  if(tsv.hasSection("com"))
  {
    std::vector<std::string> sections = tsv.sections("com");
    for(auto it = sections.begin(); it != sections.end(); ++it)
    {
      /* This is another special case because of legacy.
       * Format: `<col nr="X">\nYYY\n</col>`
       * where X is the row index (0..n), and YYY is the formula.
       * YYY may span multiple lines.
       * There may be multiple <col>s in each com section.
       */
      const std::string lines = *it;
      std::vector<std::string> valVec;
      boost::split(valVec, lines, boost::is_any_of("\n"));

      for(size_t i = 0; i < valVec.size(); ++i)
      {
        const std::string line = valVec[i];
        if(line.length() < 11)
          continue;
        const std::string colStr = line.substr(9, line.length() - 11);
        int col;
        Mantid::Kernel::Strings::convert<int>(colStr, col);
        std::string formula;
        for(++i; i < valVec.size() && valVec[i] != "</col>"; ++i)
        {
          //If we've already got a line, put a newline in first.
          if(formula.length() > 0)
            formula += "\n";

          formula += valVec[i];
        }
        setCommand(col, QString::fromUtf8(formula.c_str()));
      }
    }
  }
}

std::string TableStatistics::saveToProject(ApplicationWindow* app)
{
  TSVSerialiser tsv;
  tsv.writeRaw("<TableStatistics>");

  tsv.writeLine(objectName().toStdString());
  tsv << d_base->objectName();
  tsv << (d_type == row ? "row" : "col");
  tsv << birthDate();

  tsv.writeLine("Targets");
	for(auto it = d_targets.begin(); it != d_targets.end(); ++it)
		tsv << *it;

  tsv.writeRaw(app->windowGeometryInfo(this));

  tsv.writeRaw(saveTableMetadata());

  tsv.writeLine("WindowLabel") << windowLabel() << captionPolicy();
  tsv.writeRaw("</TableStatistics>");
  return tsv.outputLines();
}
