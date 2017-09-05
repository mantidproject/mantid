#ifndef MANTIDQTMANTIDWIDGETS_DATAPROCESSORTREEDATA_H
#define MANTIDQTMANTIDWIDGETS_DATAPROCESSORTREEDATA_H
/** This file defines the RowData, GroupData and TreeData type aliases used by
   the
    DataProcessor widget.

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

#include "MantidKernel/System.h"
#include "MantidQtWidgets/Common/DataProcessorUI/PostprocessingAlgorithm.h"
#include "MantidQtWidgets/Common/DataProcessorUI/PreprocessingAlgorithm.h"
#include "MantidQtWidgets/Common/DataProcessorUI/ProcessingAlgorithm.h"
#include "MantidQtWidgets/Common/DataProcessorUI/WhiteList.h"
#include "MantidQtWidgets/Common/DataProcessorUI/TreeData.h"
namespace MantidQt {
namespace MantidWidgets {
namespace DataProcessor {
using RowData = QStringList;
using GroupData = std::map<int, RowData>;
using TreeData = std::map<int, GroupData>;
}
}
}
#endif // MANTIDQTMANTIDWIDGETS_DATAPROCESSORTREEDATA_H
