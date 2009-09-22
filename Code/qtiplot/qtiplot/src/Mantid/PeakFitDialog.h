#ifndef PEAKFITDIALOG_H
#define PEAKFITDIALOG_H

//----------------------------
//   Includes
//----------------------------

#include <qdialog.h>
#include <qcheckbox.h>
#include <qlineedit.h>
#include <boost/shared_ptr.hpp>
#include "ui_PeakFitDialog.h"

//----------------------------
//   Forward declarations
//----------------------------

class PeakPickerTool;
class MantidUI;
namespace Mantid
{
  namespace API
  {
    class IAlgorithm;
  }
}

/** 
    This class gives a dialog for fitting peaks.

    @author Roman Tolchenov, Tessella plc
    @date 12/08/2009

    Copyright &copy; 2009 STFC Rutherford Appleton Laboratories

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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>    
*/
class PeakFitDialog : public QDialog
{
  Q_OBJECT

public:
  
  /// Default constructor
  PeakFitDialog(QWidget *parent,PeakPickerTool* peakTool);

private slots:

  void fit();
  void returnPressed();
  void setLayout(const QString& functName);
  void setUserParams();
  void setParamTable(const QStringList& params);
  void centreNameChanged ( const QString &);
  void heightNameChanged ( const QString &);
  void widthNameChanged ( const QString &);

private:
  // number of paramters per one peak
  int paramCount()const{return ui.tableParams->rowCount();}
  // is the i-th parameter fixed?
  bool isFixed(int i)const;
  // get fixed parameter's value
  std::string getValue(int i)const;
  // get fixed parameter's value
  std::string getValue(const std::string& name)const;
  // get parameter name
  std::string getName(int i)const;
  // Creates a list of fixed parameters written to a string
  QString getFixedList()const;
  // Create the right algorithm 
  boost::shared_ptr<Mantid::API::IAlgorithm> createAlgorithm();
  // Fit profiles
  void fitPeaks();

  std::string m_heightName;
  std::string m_centreName;
  std::string m_widthName;

  QMap<std::string,double> m_params;

  // formula to transform the FWHM to the "width" parameter 
  std::string m_widthCorrectionFormula;
  std::string m_backgroundFormula;
  std::string m_profileFormula;

  // Ready for fitting
  bool m_ready;
  // The peak picking tool
  PeakPickerTool* m_peakTool;
  // set when return is pressed inside ui.leExpression
  bool m_pressedReturnInExpression;
  // Mantid UI
  MantidUI* m_mantidUI;

  // The form generated with Qt Designer
  Ui::PeakFitDialog ui;
};

/**  Widget to set a parameter fixed and its value
 */
class FixedSetter:public QWidget
{
  Q_OBJECT
public:
  // Constructor
  FixedSetter():QWidget()
  {
    m_check = new QCheckBox;
    m_value = new QLineEdit;
    m_value->setFrame(false);
    QHBoxLayout *layout = new QHBoxLayout;
    layout->setContentsMargins(11,0,11,0);
    layout->addWidget(m_check);
    layout->addWidget(m_value);
    setLayout(layout);
    connect(m_check,SIGNAL(stateChanged(int)),this,SLOT(changed(int)));
  }
  // Is checked
  bool isChecked()const
  {
    return m_check->checkState() == Qt::Checked;
  }
  // Get the value for the fixed parameter
  QString getValue()const
  {
    return m_value->text();
  }
private slots:

  // Called when the state is changed
  void changed(int state)
  {
    if (state == Qt::Unchecked)
    {
      if (m_value->text() == "<value>")
        m_value->clear();
    }
    else
    {
      if (m_value->text().isEmpty())
        m_value->setText("<value>");
      m_value->setFocus();
      m_value->selectAll();
    }
  }
private:

  QCheckBox* m_check; // check box indicating fixed parameter
  QLineEdit* m_value; // edit the parameter's value
};

#endif /* PEAKFITDIALOG_H */