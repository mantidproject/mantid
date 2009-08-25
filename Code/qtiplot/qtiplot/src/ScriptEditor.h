#ifndef SCRIPTEDITOR_H_
#define SCRIPTEDITOR_H_

//----------------------------------
// Includes
//----------------------------------
#include <Qsci/qsciscintilla.h>


//----------------------------------
// Forward declarations
//----------------------------------
class QAction;

/** 
    This class provides an area to write scripts. It inherits from QScintilla to use
    functionality such as auto-indent and if supported, syntax highlighting.
        
    @author Martyn Gigg, Tessella Support Services plc
    @date 19/08/2009

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
class ScriptEditor : public QsciScintilla
{
  // Qt macro
  Q_OBJECT;
  
public:
  /// Constructor
  ScriptEditor(QWidget* parent = 0);
  ///Destructor
  ~ScriptEditor();

  /// Save a the text to the given filename
  bool saveScript(const QString & filename);

  /// The current filename
  inline QString fileName() const
  {
    return m_filename;
  }

  /**
   * Set a new file name
   * @param filename The new filename
   */
  inline void setFileName(const QString & filename)
  {
    m_filename = filename;
  }

  /// Undo action for this editor
  inline QAction* undoAction() const
  {
    return m_undo;
  }
  /// Redo action for this editor
  inline QAction* redoAction() const
  {
    return m_redo;
  }

  /// Cut action for this editor
  inline QAction* cutAction() const
  {
    return m_cut;
  }
  /// Copy action for this editor
  inline QAction* copyAction() const
  {
    return m_copy;
  }
  /// Paste action for this editor
  inline QAction* pasteAction() const
  {
    return m_paste;
  }
  

signals:
  /// Inform observers that undo information is available
  void undoAvailable(bool);
  /// Inform observers that redo information is available
  void redoAvailable(bool);

private slots:
  /// Update the editor
  void update();

private:
  /// The file name associated with this editor
  QString m_filename;

  //Each editor needs its own undo/redo etc
  QAction *m_undo, *m_redo, *m_cut, *m_copy, *m_paste;
};


#endif //SCRIPTEDITOR_H_
