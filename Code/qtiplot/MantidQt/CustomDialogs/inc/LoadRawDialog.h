#ifndef MANTIDQT_CUSTOM_DIALOGSLOADRAWDIALOG_H
#define MANTIDQT_CUSTOM_DIALOGSLOADRAWDIALOG_H

//----------------------
// Includes
//----------------------
#include "MantidQtAPI/AlgorithmDialog.h"

#include <QString>
#include <QHash>

//---------------------------
// Qt Forward declarations
//---------------------------
class QVBoxLayout;
class QLineEdit;
class QComboBox;
class QPushButton;

namespace MantidQt
{
namespace CustomDialogs
{

/** 
    This class gives specialised dialog for the LoadRaw algorithm.

    @author Martyn Gigg, Tessella Support Services plc
    @date 24/02/2009

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

class LoadRawDialog : public MantidQt::API::AlgorithmDialog
{

  Q_OBJECT
	
public:

  /// Constructor
  LoadRawDialog(QWidget *parent = 0);
  ///Destructor
  ~LoadRawDialog();

private:

  /** @name Virtual functions. */
  //@{
  /// Create the layout
  void initLayout();

  /// Parse the user's input
  void parseInput(); 		   
  //@}
	
private slots:

  /// A slot for the browse button clicked signal
  void browseClicked();

private:

  /** @name Utility functions */
  //@{
  ///Filename property input
  void addFilenameInput();
  
  ///Output workspace property
  void addOutputWorkspaceInput();

  ///Spectra related properties
  void addSpectraInput();
  
  ///Cache combo box 
  void addCacheOptions();

  /// Set old input for text edit field
  void setOldTextEditInput(const QString & propName, QLineEdit* field);

  /// Set old input for combo box
  void setOldComboField(const QString & propName);
  //@}
  
  /// The main layout
  QVBoxLayout *m_mainLayout;

  ///The line inputs
  QLineEdit *m_pathBox, *m_wsBox, *m_minSpec, *m_maxSpec, *m_specList;
  
  /// A box to store the cache options
  QComboBox *m_cacheBox;
  
  ///The browse button
  QPushButton *m_browseBtn;
  
  ///Store the allowed extensions
  QString m_fileFilter;
  
  ///Previous values
  QHash<QString, QString> m_oldValues;
};

}
}

#endif //MANTIDQT_CUSTOMDIALOGS_LOADRAWDIALOG_H
