#ifndef MANTID_CUSTOMINTERFACES_REFLMAINVIEWPRESENTER_H
#define MANTID_CUSTOMINTERFACES_REFLMAINVIEWPRESENTER_H

#include "MantidKernel/System.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidQtCustomInterfaces/ReflMainView.h"
#include "MantidQtCustomInterfaces/IReflPresenter.h"
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
      ReflMainViewPresenter(Mantid::API::ITableWorkspace_sptr model, ReflMainView* view);
      ReflMainViewPresenter(ReflMainView* view);
      virtual ~ReflMainViewPresenter() = 0;
      virtual void notify();
    protected:
      //The model and backup copy of the original model
      Mantid::API::ITableWorkspace_sptr m_model;
      Mantid::API::ITableWorkspace_sptr m_cache;
      std::string m_cache_name;
      //the view
      ReflMainView* m_view;

      //Load the model into the view
      virtual void load();
      //process selected rows
      virtual void process();
      //make a transmission workspace
      virtual void makeTransWS(const std::string& transString);
      //Process a row
      std::string processRow(size_t rowNo, std::string lastTrans = "");
      //add row(s) to the model
      virtual void addRow();
      //delete row(s) from the model
      virtual void deleteRow();
      //virtual save methods
      virtual void save() = 0;
      virtual void saveAs() = 0;

      static const int COL_RUNS;
      static const int COL_ANGLE;
      static const int COL_TRANSMISSION;
      static const int COL_QMIN;
      static const int COL_QMAX;
      static const int COL_DQQ;
      static const int COL_SCALE;
      static const int COL_GROUP;
    };
  }
}
#endif
