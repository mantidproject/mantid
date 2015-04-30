#ifndef MANTIDQT_API_PLOTAXISLABEL_H_
#define MANTIDQT_API_PLOTAXISLABEL_H_

#include "MantidQtAPI/DllOption.h"
#include "MantidAPI/MatrixWorkspace.h"

#include <QString>

namespace MantidQt
{
  namespace API
  {

    /**
      Deals with formatting a label for a plot axis for a given type of workspace

      Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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
    class EXPORT_OPT_MANTIDQT_API PlotAxis
    {
    public:
      /// Constructor with workspace & axis index
      PlotAxis(const Mantid::API::IMDWorkspace & workspace,
               const size_t index);
      /// Constructor with an IMDDimension
      PlotAxis(const Mantid::Geometry::IMDDimension & dim);
      /// Constructor with just a workspace (reverse order to above so compiler doesn't convert a
      /// a bool to an size_t and call the wrong thing
      PlotAxis(const bool plottingDistribution, const Mantid::API::MatrixWorkspace & workspace);

      /// Create a new axis title
      QString title() const;

    private:
      DISABLE_DEFAULT_CONSTRUCT(PlotAxis)

      /// Creates a title suitable for an axis attached to the given index
      void titleFromIndex(const Mantid::API::IMDWorkspace & workspace,
                          const size_t index);
      /// Creates a title suitable for an axis attached to the given dimension
      void titleFromDimension(const Mantid::Geometry::IMDDimension & dim);
      /// Creates a title suitable for the Y data values
      void titleFromYData(const Mantid::API::MatrixWorkspace & workspace,
                          const bool plottingDistribution);

      /// Title
      QString m_title;
    };

  } // namespace API
} // namespace MantidQt

#endif  /* MANTIDQT_API_PLOTAXISLABEL_H_ */
