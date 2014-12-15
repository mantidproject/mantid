#ifndef MANTID_ALGORITHMS_APPENDSPECTRA_H_
#define MANTID_ALGORITHMS_APPENDSPECTRA_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/WorkspaceJoiners.h"

namespace Mantid
{
namespace Algorithms
{

  /**
    Joins two partial, non-overlapping workspaces into one. Used in the situation where you
    want to load a raw file in two halves, process the data and then join them back into
    a single dataset.
    The input workspaces must come from the same instrument, have common units and bins
    and no detectors that contribute to spectra should overlap.
    
    @date 2012-02-20

    Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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
  class DLLExport AppendSpectra : public WorkspaceJoiners
  {
  public:
    AppendSpectra();
    virtual ~AppendSpectra();
    
    virtual const std::string name() const;
    virtual int version() const;

  private:
    // Overridden Algorithm methods
    void init();
    void exec();

    virtual void fixSpectrumNumbers(API::MatrixWorkspace_const_sptr ws1, API::MatrixWorkspace_const_sptr ws2, API::MatrixWorkspace_sptr output);
    void combineLogs(const API::Run& lhs, const API::Run& rhs, API::Run& ans);
  };


} // namespace Algorithms
} // namespace Mantid

#endif  /* MANTID_ALGORITHMS_APPENDSPECTRA_H_ */
