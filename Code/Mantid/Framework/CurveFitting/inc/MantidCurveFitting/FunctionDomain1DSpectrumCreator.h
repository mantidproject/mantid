#ifndef MANTID_CURVEFITTING_FUNCTIONDOMAIN1DSPECTRUMCREATOR_H_
#define MANTID_CURVEFITTING_FUNCTIONDOMAIN1DSPECTRUMCREATOR_H_

#include "MantidKernel/System.h"
#include "MantidAPI/IDomainCreator.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/FunctionDomain1D.h"

namespace Mantid {
namespace CurveFitting {

/** FunctionDomain1DSpectrumCreator :

    FunctionDomain1DSpectrumCreator creates an FunctionDomain1DSpectrum using a
   given
    MatrixWorkspace and workspace index. Currently it does not create an output
   workspace,
    since it is exclusively used in a context where it's not required.

      @author Michael Wedel, Paul Scherrer Institut - SINQ
      @date 30/05/2014

    Copyright © 2014 PSI-MSS

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

class DLLExport FunctionDomain1DSpectrumCreator : public API::IDomainCreator {
public:
  FunctionDomain1DSpectrumCreator();
  virtual ~FunctionDomain1DSpectrumCreator() {}

  void setMatrixWorkspace(API::MatrixWorkspace_sptr matrixWorkspace);
  void setWorkspaceIndex(size_t workspaceIndex);

  virtual void createDomain(boost::shared_ptr<API::FunctionDomain> &domain,
                            boost::shared_ptr<API::FunctionValues> &values,
                            size_t i0 = 0);

  virtual size_t getDomainSize() const;

protected:
  void throwIfWorkspaceInvalid() const;

  MantidVec getVectorHistogram() const;
  MantidVec getVectorNonHistogram() const;

  API::MatrixWorkspace_sptr m_matrixWorkspace;
  size_t m_workspaceIndex;
  bool m_workspaceIndexIsSet;
};

} // namespace CurveFitting
} // namespace Mantid

#endif /* MANTID_CURVEFITTING_FUNCTIONDOMAIN1DSPECTRUMCREATOR_H_ */
