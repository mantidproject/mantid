#ifndef MANTID_DATAOBJECTS_REBINNEDOUTPUT_H_
#define MANTID_DATAOBJECTS_REBINNEDOUTPUT_H_

#include "MantidAPI/ISpectrum.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/System.h"

namespace Mantid
{
//----------------------------------------------------------------------
// Forward declarations
//----------------------------------------------------------------------
namespace Kernel
{
  class Logger;
}
namespace DataObjects
{

  /** RebinnedOutput

    This class will handle the needs for 2D fractional overlap rebinning.
    The rebinning method requires the separate tracking of counts and
    fractional area. The workspace will always present the correct data to a
    2D display. Integration and rebinning will be handled via the fundamental
    algorithms.
    
    @date 2012-04-05

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
  class DLLExport RebinnedOutput : public Workspace2D
  {
  public:
    /// Class constructor.
    RebinnedOutput();
    /// Class destructor.
    virtual ~RebinnedOutput();

    /// Get the workspace ID.
    virtual const std::string id() const;

    
  protected:
    /// Called by initialize() in MatrixWorkspace
    virtual void init(const std::size_t &NVectors, const std::size_t &XLength, const std::size_t &YLength);

    /// A vector that holds the 1D histograms for the fractional area.
    std::vector<Mantid::API::ISpectrum *> fracArea;

    /// Static reference to the logger class
    static Kernel::Logger &g_log;

  private:
    /// Private copy constructor. NO COPY ALLOWED
    RebinnedOutput(const RebinnedOutput&);
    /// Private copy assignment operator. NO ASSIGNMENT ALLOWED
    RebinnedOutput& operator=(const RebinnedOutput&);

  };

  ///shared pointer to the RebinnedOutput class
  typedef boost::shared_ptr<RebinnedOutput> RebinnedOutput_sptr;
  ///shared pointer to a const RebinnedOutput
  typedef boost::shared_ptr<const RebinnedOutput> RebinnedOutput_const_sptr;

} // namespace DataObjects
} // namespace Mantid

#endif  /* MANTID_DATAOBJECTS_REBINNEDOUTPUT_H_ */
