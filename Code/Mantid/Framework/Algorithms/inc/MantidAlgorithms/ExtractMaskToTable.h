#ifndef MANTID_ALGORITHMS_EXTRACTMASKTOTABLE_H_
#define MANTID_ALGORITHMS_EXTRACTMASKTOTABLE_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidDataObjects/TableWorkspace.h"


namespace Mantid
{
namespace Algorithms
{

  /** ExtractMaskToTable : Extract the mask in a workspace to a table workspace.
    The table workspace must be compatible to algorithm MaskBinsFromTable.
    
    Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
  class DLLExport ExtractMaskToTable : public API::Algorithm
  {
  public:
    ExtractMaskToTable();
    virtual ~ExtractMaskToTable();

    /// Algorithm's name
    virtual const std::string name() const { return "ExtractMaskToTable"; }
    /// Algorithm's version
    virtual int version() const { return 1; }
    /// Algorithm's category for identification
    virtual const std::string category() const { return "Transforms\\Masking"; }

  private:
    /// Sets documentation strings for this algorithm
    virtual void initDocs();
    /// Initialisation code
    void init();
    /// Execution code
    void exec();

    /// Input matrix workspace
    API::MatrixWorkspace_const_sptr m_dataWS;
    /// Input table workspace
    DataObjects::TableWorkspace_sptr m_inputTableWS;

    /// Minimum X range
    double m_XMin;

    /// Maximum X range
    double m_XMax;
    
  };


} // namespace Algorithms
} // namespace Mantid

#endif  /* MANTID_ALGORITHMS_EXTRACTMASKTOTABLE_H_ */
