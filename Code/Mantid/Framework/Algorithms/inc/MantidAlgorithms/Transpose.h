#ifndef MANTID_ALGORITHMS_TRANSPOSE_H_
#define MANTID_ALGORITHMS_TRANSPOSE_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"

namespace Mantid
{
  namespace Algorithms
  {
    /**
    This algorithm "transposes" the bins of the input workspace into a single spectra. So, given
    an input workspace of N1 Spectra with N2 Bins, the result is a workspace with N2 Spectra, and
    N1 Bins. The output workspace is data points, not histograms.

    @author Michael Whitty, STFC ISIS
    @date 09/09/2010

    Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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
    class DLLExport Transpose : public API::Algorithm
    {
    public:
      /// (Empty) Constructor
      Transpose() : API::Algorithm() {}
      /// Virtual destructor
      virtual ~Transpose() {}
      /// Algorithm's name
      virtual const std::string name() const { return "Transpose"; }
    ///Summary of algorithms purpose
    virtual const std::string summary() const {return "Transposes a workspace, so that an N1 x N2 workspace becomes N2 x N1.";}

      /// Algorithm's version
      virtual int version() const { return (1); }
      /// Algorithm's category for identification
      virtual const std::string category() const { return "Transforms\\Axes"; }

    private:
      
      /// Initialisation code
      void init();
      /// Execution code
      void exec();
      /// Create the output workspace
      API::MatrixWorkspace_sptr createOutputWorkspace(API::MatrixWorkspace_const_sptr workspace);
      /// Return the vertical axis on the workspace, throwing if it is not valid
      API::Axis *getVerticalAxis(API::MatrixWorkspace_const_sptr inputWorkspace) const;
    };

  } // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_TRANSPOSE_H_*/
