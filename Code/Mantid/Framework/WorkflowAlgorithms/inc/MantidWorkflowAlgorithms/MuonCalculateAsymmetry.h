#ifndef MANTID_WORKFLOWALGORITHMS_MUONCALCULATEASYMMETRY_H_
#define MANTID_WORKFLOWALGORITHMS_MUONCALCULATEASYMMETRY_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"

namespace Mantid
{
namespace WorkflowAlgorithms
{
  using namespace Kernel;
  using namespace API;

  /** MuonCalculateAsymmetry : TODO: DESCRIPTION
    
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
  class DLLExport MuonCalculateAsymmetry  : public API::Algorithm
  {
  public:
    MuonCalculateAsymmetry();
    virtual ~MuonCalculateAsymmetry();
    
    virtual const std::string name() const;
    virtual int version() const;
    virtual const std::string category() const;

  private:
    virtual void initDocs();
    void init();
    void exec();

    /// TODO: comment
    MatrixWorkspace_sptr convertWorkspace(MatrixWorkspace_sptr ws);
  
    /// TODO: comment
    MatrixWorkspace_sptr mergePeriods(MatrixWorkspace_sptr ws1, MatrixWorkspace_sptr ws2);
  };


} // namespace WorkflowAlgorithms
} // namespace Mantid

#endif  /* MANTID_WORKFLOWALGORITHMS_MUONCALCULATEASYMMETRY_H_ */
