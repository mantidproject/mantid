#ifndef MANTID_CUSTOMINTERFACES_REFLMAINVIEWPRESENTER_H
#define MANTID_CUSTOMINTERFACES_REFLMAINVIEWPRESENTER_H

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidKernel/System.h"
#include "MantidQtCustomInterfaces/ReflMainView.h"
#include "MantidQtCustomInterfaces/IReflPresenter.h"

#include <Poco/AutoPtr.h>
#include <Poco/NObserver.h>

namespace MantidQt
{
  namespace CustomInterfaces
  {
    /** @class ReflMainViewPresenter

    ReflMainViewPresenter is a presenter class for teh Reflectometry Interface. It handles any interface functionality and model manipulation.

    Copyright &copy; 2011-14 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

    File change history is stored at: <https://github.com/mantidproject/mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
    */
    class DLLExport ReflMainViewPresenter: public IReflPresenter
    {
    public:
      ReflMainViewPresenter(ReflMainView* view);
      virtual ~ReflMainViewPresenter();
      virtual void notify(int flag);

      //Public for the purposes of unit testing
      static std::map<std::string,std::string> parseKeyValueString(const std::string& str);
    protected:
      //the model the table is currently representing
      Mantid::API::ITableWorkspace_sptr m_model;
      //the name of the workspace/table/model in the ADS, blank if unsaved
      std::string m_wsName;
      //the view we're managing
      ReflMainView* m_view;
      //stores whether or not the table has changed since it was last saved
      bool m_tableDirty;

      //process selected rows
      virtual void process();
      //load a run into the ADS, or re-use one in the ADS if possible
      Mantid::API::Workspace_sptr loadRun(const std::string& run, const std::string& instrument);
      //get the run number of a TOF workspace
      std::string getRunNumber(const Mantid::API::Workspace_sptr& ws);
      //get an unused group id
      int getUnusedGroup(std::vector<size_t> ignoredRows = std::vector<size_t>()) const;
      //make a transmission workspace
      Mantid::API::MatrixWorkspace_sptr makeTransWS(const std::string& transString);
      //Validate a row
      void validateRow(size_t rowNo) const;
      //Autofill a row with sensible values
      void autofillRow(size_t rowNo);
      //calculates qmin and qmax
      static std::vector<double> calcQRange(Mantid::API::MatrixWorkspace_sptr ws, double theta);
      //Process a row
      void processRow(size_t rowNo);
      //Stitch some rows
      void stitchRows(std::vector<size_t> rows);
      //insert a row in the model before the given index
      virtual void insertRow(size_t before);
      //add row(s) to the model
      virtual void addRow();
      //delete row(s) from the model
      virtual void deleteRow();
      //group selected rows together
      virtual void groupRows();
      //table io methods
      virtual void newTable();
      virtual void openTable();
      virtual void saveTable();
      virtual void saveTableAs();

      //List of workspaces the user can open
      std::set<std::string> m_workspaceList;

      //To maintain a list of workspaces the user may open, we observe the ADS
      Poco::NObserver<ReflMainViewPresenter, Mantid::API::WorkspaceAddNotification> m_addObserver;
      Poco::NObserver<ReflMainViewPresenter, Mantid::API::WorkspacePostDeleteNotification> m_remObserver;
      Poco::NObserver<ReflMainViewPresenter, Mantid::API::ClearADSNotification> m_clearObserver;
      Poco::NObserver<ReflMainViewPresenter, Mantid::API::WorkspaceRenameNotification> m_renameObserver;
      Poco::NObserver<ReflMainViewPresenter, Mantid::API::WorkspaceAfterReplaceNotification> m_replaceObserver;

      void handleAddEvent(Mantid::API::WorkspaceAddNotification_ptr pNf);
      void handleRemEvent(Mantid::API::WorkspacePostDeleteNotification_ptr pNf);
      void handleClearEvent(Mantid::API::ClearADSNotification_ptr pNf);
      void handleRenameEvent(Mantid::API::WorkspaceRenameNotification_ptr pNf);
      void handleReplaceEvent(Mantid::API::WorkspaceAfterReplaceNotification_ptr pNf);

    public:
      static const int COL_RUNS         = 0;
      static const int COL_ANGLE        = 1;
      static const int COL_TRANSMISSION = 2;
      static const int COL_QMIN         = 3;
      static const int COL_QMAX         = 4;
      static const int COL_DQQ          = 5;
      static const int COL_SCALE        = 6;
      static const int COL_GROUP        = 7;
      static const int COL_OPTIONS      = 8;
    };
  }
}
#endif
