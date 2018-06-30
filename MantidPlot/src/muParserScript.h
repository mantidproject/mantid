/***************************************************************************
    File                 : muParserScript.h
    Project              : QtiPlot
    --------------------------------------------------------------------

    Copyright            : (C) 2006 by Ion Vasilief, Knut Franke
    Email (use @ for *)  : ion_vasilief*yahoo.fr, knut.franke*gmx.de
    Description          : Evaluate mathematical expressions using muParser

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
#ifndef MUPARSER_SCRIPT_H
#define MUPARSER_SCRIPT_H

#include "Script.h"

#include "MantidGeometry/muParser_Silent.h"
#include "math.h"
#include <QMap>
#include <gsl/gsl_sf.h>

class ScriptingEnv;

class muParserScript : public Script {
  Q_OBJECT

public:
  muParserScript(ScriptingEnv *env, const QString &name, QObject *context,
                 bool checkMultilineCode = true);
  ~muParserScript();

  bool compilesToCompleteStatement(const QString &) const override {
    return true;
  };

public slots:
  QVariant evaluateImpl() override;
  double evalSingleLine();
  QString evalSingleLineToString(const QLocale &locale, char f, int prec);
  bool compileImpl() override;
  bool executeImpl() override;
  void abortImpl() override;
  bool setQObject(QObject *val, const char *name) override;
  bool setInt(int val, const char *name) override;
  bool setDouble(double val, const char *name) override;
  double *defineVariable(const char *name, double val = 0.0);
  int codeLines() { return muCode.size(); };

private:
  double col(const QString &arg);
  double tablecol(const QString &arg);
  double cell(int row, int col);
  double tableCell(int col, int row);
  double *addVariable(const char *name);
  double *addVariableR(const char *name);
  static double *mu_addVariableR(const char *name) {
    return current->addVariableR(name);
  }
  static double mu_col(const char *arg) { return current->col(arg); }
  static double mu_cell(double row, double col) {
    return current->cell(qRound(row), qRound(col));
  }
  static double mu_tableCell(double col, double row) {
    return current->tableCell(qRound(col), qRound(row));
  }
  static double mu_tablecol(const char *arg) { return current->tablecol(arg); }
  static double *mu_addVariable(const char *name, void *) {
    return current->addVariable(name);
  }
  static double *mu_addVariableR(const char *name, void *) {
    return current->addVariableR(name);
  }
  static QString compileColArg(const QString &in);
  static void clearVariables(QMap<QString, double *> &vars);

  mu::Parser parser, rparser;
  QMap<QString, double *> variables, rvariables;
  QStringList muCode;
  //! Flag telling is the parser should warn users on multiline code input
  bool d_warn_multiline_code;

public:
  static muParserScript *current;
};

#endif
