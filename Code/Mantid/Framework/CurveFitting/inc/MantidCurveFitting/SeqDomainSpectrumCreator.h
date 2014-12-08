#ifndef MANTID_CURVEFITTING_SEQDOMAINSPECTRUMCREATOR_H_
#define MANTID_CURVEFITTING_SEQDOMAINSPECTRUMCREATOR_H_

#include "MantidKernel/System.h"
#include "MantidAPI/IDomainCreator.h"
#include "MantidAPI/FunctionDomain1D.h"
#include "MantidAPI/MatrixWorkspace.h"

namespace Mantid
{
namespace CurveFitting
{

/** SeqDomainSpectrumCreator :

    SeqDomainSpectrumCreator creates a special type of SeqDomain, which contains
    one FunctionDomain1DSpectrum for each histogram of the Workspace2D this
    domain refers to. It can be used for functions that involve several histograms
    at once.
    
      @author Michael Wedel, Paul Scherrer Institut - SINQ
      @date 28/05/2014

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

class DLLExport SeqDomainSpectrumCreator : public API::IDomainCreator
{
public:
    SeqDomainSpectrumCreator(Kernel::IPropertyManager* manager,
                             const std::string& workspacePropertyName);

    virtual ~SeqDomainSpectrumCreator() { }

    virtual void createDomain(boost::shared_ptr<API::FunctionDomain> &domain,
                              boost::shared_ptr<API::FunctionValues> &values,
                              size_t i0 = 0);

    virtual API::Workspace_sptr createOutputWorkspace(const std::string &baseName,
                                       API::IFunction_sptr function,
                                       boost::shared_ptr<API::FunctionDomain> domain,
                                       boost::shared_ptr<API::FunctionValues> values,
                                       const std::string &outputWorkspacePropertyName = "OutputWorkspace");
    virtual size_t getDomainSize() const;

protected:
    void setParametersFromPropertyManager();
    void setMatrixWorkspace(API::MatrixWorkspace_sptr matrixWorkspace);

    bool histogramIsUsable(size_t i) const;

    std::string m_workspacePropertyName;
    API::MatrixWorkspace_sptr m_matrixWorkspace;
};


} // namespace CurveFitting
} // namespace Mantid

#endif  /* MANTID_CURVEFITTING_SEQDOMAINSPECTRUMCREATOR_H_ */
