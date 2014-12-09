#ifndef MANTIDQT_API_SIGNALRANGE_H_
#define MANTIDQT_API_SIGNALRANGE_H_

#include "MantidQtAPI/DllOption.h"
#include "MantidAPI/IMDWorkspace.h"
#include "MantidKernel/ClassMacros.h"

#include <qwt_double_interval.h>

namespace MantidQt
{
  namespace API
  {
    /**
     Calculates the signal range from a given workspace and optional MDFunction

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

     File change history is stored at: <https://github.com/mantidproject/mantid>.
     Code Documentation is available at: <http://doxygen.mantidproject.org>
    */
    class EXPORT_OPT_MANTIDQT_API SignalRange
    {
    public:
      SignalRange(const Mantid::API::IMDWorkspace & workspace,
                  const Mantid::API::MDNormalization normalization = Mantid::API::NoNormalization);
      SignalRange(const Mantid::API::IMDWorkspace & workspace,
                  Mantid::Geometry::MDImplicitFunction &function,
                  const Mantid::API::MDNormalization normalization = Mantid::API::NoNormalization);

      /// Returns the range of the workspace signal values
      QwtDoubleInterval interval() const;

    private:
      DISABLE_DEFAULT_CONSTRUCT(SignalRange);

      /// Find the min/max signal values in the entire workspace
      void findFullRange(const Mantid::API::IMDWorkspace & workspace,
                         Mantid::Geometry::MDImplicitFunction *function);
      /// Get the range of signal, in parallel, given an iterator
      QwtDoubleInterval getRange(const std::vector<Mantid::API::IMDIterator *> & iterators);
      ///Get the range of signal given an iterator
      QwtDoubleInterval getRange(Mantid::API::IMDIterator * it);

      /// The range of the signal data
      QwtDoubleInterval m_interval;
      /// The normalization used for the signals
      Mantid::API::MDNormalization m_normalization;
    };

  } // namespace API
} // namespace MantidQt

#endif /* MANTIDQT_API_SIGNALRANGE_H_ */
