#ifndef MANTID_CUSTOMINTERFACES_REFLGENERATENOTEBOOK_H
#define MANTID_CUSTOMINTERFACES_REFLGENERATENOTEBOOK_H

/** @class ReflGenerateNotebook

    This class creates ipython notebooks from the ISIS Reflectometry
    (Polref) interface

    Copyright &copy; 2007-8 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
    National Laboratory & European Spallation Source

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

#include "MantidQtCustomInterfaces/QReflTableModel.h"
#include "MantidKernel/System.h"

#include <set>
#include <map>
#include <string>


namespace MantidQt {
  namespace CustomInterfaces {


    // Column numbers to find data in model
    struct ColNumbers {

      ColNumbers(const int runs_column, const int transmission_column, const int options_column,
                 const int angle_column, const int qmin_column, const int qmax_column,
                 const int dqq_column, const int scale_column, const int group_column)
        : runs(runs_column), transmission(transmission_column), options(options_column), angle(angle_column),
          qmin(qmin_column), qmax(qmax_column), dqq(dqq_column), scale(scale_column), group(group_column) {}

      const int runs;
      const int transmission;
      const int options;
      const int angle;
      const int qmin;
      const int qmax;
      const int dqq;
      const int scale;
      const int group;
    };

    std::string plot1DString(const std::vector<std::string> & ws_names,
                             const std::string & title);

    std::string tableString(QReflTableModel_sptr model, ColNumbers col_nums, const std::set<int> & rows);

    template<typename T, typename A>
    std::string vectorString(const std::vector<T,A> &param_vec);

    std::tuple<std::string, std::string>
      stitchGroupString(const std::set<int> & rows, const std::string & instrument, QReflTableModel_sptr model,
                        ColNumbers col_nums);

    std::string plotsFunctionString();

    std::tuple<std::string, std::string, std::string>
      reduceRowString(const int rowNo, const std::string & instrument, QReflTableModel_sptr model, ColNumbers col_nums);

    std::tuple<std::string, std::string> loadWorkspaceString(const std::string & runStr, const std::string & instrument);

    std::string plusString(const std::string & input_name, const std::string & output_name);

    std::tuple<std::string, std::string> loadRunString(const std::string & run, const std::string & instrument);

    std::string getRunNumber(const std::string & ws_name);

    std::tuple<std::string, std::string> scaleString(const std::string & runNo, const double scale);

    std::tuple<std::string, std::string> convertToPointString(const std::string & wsName);

    template<typename T, typename A>
    std::string vectorParamString(const std::string & param_name, const std::vector<T,A> &param_vec);

    std::tuple<std::string, std::string>
      rebinString(const int rowNo, const std::string & runNo, QReflTableModel_sptr model, ColNumbers col_nums);

    std::tuple<std::string, std::string> transWSString(const std::string & trans_ws_str, const std::string & instrument);


    class DLLExport ReflGenerateNotebook {

    public:

      ReflGenerateNotebook(std::string name,
                           QReflTableModel_sptr model,
                           const std::string instrument,
                           const int col_runs, const int col_transmission, const int col_options, const int col_angle,
                           const int col_qmin, const int col_qmax, const int col_dqq,
                           const int col_scale, const int col_group);

      virtual ~ReflGenerateNotebook(){};

      std::string generateNotebook(std::map<int, std::set<int>> groups, std::set<int> rows);

    private:

      std::string m_wsName;
      QReflTableModel_sptr m_model;
      const std::string m_instrument;

      ColNumbers col_nums;

    };

  }
}

#endif //MANTID_CUSTOMINTERFACES_REFLGENERATENOTEBOOK_H
