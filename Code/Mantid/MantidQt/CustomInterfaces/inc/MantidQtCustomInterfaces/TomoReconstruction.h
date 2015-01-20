#ifndef MANTIDQTCUSTOMINTERFACES_TOMORECONSTRUCTION_H_
#define MANTIDQTCUSTOMINTERFACES_TOMORECONSTRUCTION_H_

//----------------------
// Includes
//----------------------
#include "ui_TomoReconstruction.h"
#include "MantidQtAPI/UserSubWindow.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/ScopedWorkspace.h"

class QTreeWidgetItem;

namespace MantidQt
{
  namespace CustomInterfaces
  {
    /**
    Tomographic reconstruction GUI. Interface for editing parameters and
    running jobs.
    @author John R Hill, STFC

    Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory, NScD
    Oak Ridge National Laboratory & European Spallation Source

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

    File change history is stored at: <https://github.com/mantidproject/mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
    */

    class DLLExport TomoReconstruction : public MantidQt::API::UserSubWindow
    {
      Q_OBJECT

    public: // public constructor, destructor and functions
      /// Default Constructor
      TomoReconstruction(QWidget *parent = 0);
      /// Destructor
      ~TomoReconstruction() {}
      /// Interface name
      static std::string name() { return "Tomographic Reconstruction"; }
      /// This interface's categories.
      static QString categoryInfo() { return "Diffraction"; }
      /// Setup tab UI
      virtual void initLayout();

    private slots:
      void menuSaveClicked();
      void menuSaveAsClicked();
      void availablePluginSelected();
      void currentPluginSelected();
      void transferClicked();
      void moveUpClicked();
      void moveDownClicked();
      void removeClicked();
      void menuOpenClicked();      
      void paramValModified(QTreeWidgetItem*,int);
      void expandedItem(QTreeWidgetItem*);

    private:
      /// Load default interface settings for each tab
      void loadSettings();
      void loadAvailablePlugins();
      void refreshAvailablePluginListUI();
      void refreshCurrentPluginListUI();
      QString tableWSToString(Mantid::API::ITableWorkspace_sptr table);
      void loadTomoConfig(std::string &filePath, std::vector<Mantid::API::ITableWorkspace_sptr> &currentPlugins);
      std::string createUniqueNameHidden();
      void createPluginTreeEntry(Mantid::API::ITableWorkspace_sptr table);

      ///Main interface window
      Ui::TomoReconstruction m_uiForm;
      std::vector<Mantid::API::ITableWorkspace_sptr> m_availPlugins;
      std::vector<Mantid::API::ITableWorkspace_sptr> m_currPlugins;
      std::string m_currentParamPath;
      static size_t nameSeqNo;
    };
  }
}

#endif
