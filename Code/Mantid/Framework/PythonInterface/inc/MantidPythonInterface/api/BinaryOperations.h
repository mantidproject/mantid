#ifndef MANTID_PYTHONINTERFACE_BINARYOPERATIONS_H_
#define MANTID_PYTHONINTERFACE_BINARYOPERATIONS_H_
//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include <string>

namespace Mantid
{
  namespace PythonInterface
  {
    /**
      Defines helpers to run the binary operation algorithms

      Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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
    /** @name Binary operation helpers */
    //@{
    /// Binary op for two workspaces
    template<typename LHSType, typename RHSType, typename ResultType>
    ResultType performBinaryOp(const LHSType lhs, const RHSType rhs,
                         const std::string & op, const std::string & name,
                         bool inplace, bool reverse);

    /// Binary op for two MDworkspaces
    template<typename LHSType, typename RHSType, typename ResultType>
    ResultType performBinaryOpMD(const LHSType lhs, const RHSType rhs,
                         const std::string & op, const std::string & name,
                         bool inplace, bool reverse);

    /// Binary op for a workspace and a double
    template<typename LHSType, typename ResultType>
    ResultType performBinaryOpWithDouble(const LHSType inputWS, const double value,
                         const std::string & op, const std::string & name,
                         bool inplace, bool reverse);

    /// Binary op for MDworkspaces + double
    template<typename LHSType, typename ResultType>
    ResultType performBinaryOpMDWithDouble(const LHSType lhs, const double value,
                         const std::string & op, const std::string & name,
                         bool inplace, bool reverse);
    //@}
  }
}

#endif /* MANTID_PYTHONINTERFACE_BINARYOPERATIONS_H_ */
