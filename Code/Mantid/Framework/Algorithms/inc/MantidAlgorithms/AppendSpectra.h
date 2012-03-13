#ifndef MANTID_ALGORITHMS_APPENDSPECTRA_H_
#define MANTID_ALGORITHMS_APPENDSPECTRA_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"

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

    Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
  */
  class DLLExport AppendSpectra  : public API::Algorithm
  {
  public:
    AppendSpectra();
    virtual ~AppendSpectra();
    
    virtual const std::string name() const;
    virtual int version() const;
    virtual const std::string category() const;

  private:
    /// Sets documentation strings for this algorithm
    virtual void initDocs();
    // Overridden Algorithm methods
    void init();
    void exec();
    void execEvent();

    static void getMinMax(Mantid::API::MatrixWorkspace_const_sptr ws, specid_t& min, specid_t& max);

    using Mantid::API::Algorithm::validateInputs;
    void validateInputs(API::MatrixWorkspace_const_sptr ws1, API::MatrixWorkspace_const_sptr ws2);
    void fixSpectrumNumbers(API::MatrixWorkspace_const_sptr ws1, API::MatrixWorkspace_const_sptr ws2, API::MatrixWorkspace_sptr output);

    /// Progress reporting object
    API::Progress *m_progress;
    /// First event workspace input.
    DataObjects::EventWorkspace_const_sptr event_ws1;
    /// Second event workspace input.
    DataObjects::EventWorkspace_const_sptr event_ws2;
    /// True if spectra overlap
    bool m_overlapChecked;

  };


} // namespace Algorithms
} // namespace Mantid

#endif  /* MANTID_ALGORITHMS_APPENDSPECTRA_H_ */
