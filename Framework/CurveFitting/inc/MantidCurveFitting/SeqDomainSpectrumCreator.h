// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/FunctionDomain1D.h"
#include "MantidAPI/IDomainCreator.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidAPI/Workspace_fwd.h"
#include "MantidCurveFitting/DllConfig.h"

namespace Mantid {
namespace CurveFitting {

/** SeqDomainSpectrumCreator :

    SeqDomainSpectrumCreator creates a special type of SeqDomain, which contains
    one FunctionDomain1DSpectrum for each histogram of the Workspace2D this
    domain refers to. It can be used for functions that involve several
   histograms
    at once.

      @author Michael Wedel, Paul Scherrer Institut - SINQ
      @date 28/05/2014
  */

class MANTID_CURVEFITTING_DLL SeqDomainSpectrumCreator : public API::IDomainCreator {
public:
  SeqDomainSpectrumCreator(Kernel::IPropertyManager *manager, const std::string &workspacePropertyName);

  void createDomain(std::shared_ptr<API::FunctionDomain> &domain, std::shared_ptr<API::FunctionValues> &values,
                    size_t i0 = 0) override;

  API::Workspace_sptr
  createOutputWorkspace(const std::string &baseName, API::IFunction_sptr function,
                        std::shared_ptr<API::FunctionDomain> domain, std::shared_ptr<API::FunctionValues> values,
                        const std::string &outputWorkspacePropertyName = "OutputWorkspace") override;
  size_t getDomainSize() const override;

protected:
  void setParametersFromPropertyManager();
  void setMatrixWorkspace(const API::MatrixWorkspace_sptr &matrixWorkspace);

  bool histogramIsUsable(size_t i) const;

  std::string m_workspacePropertyName;
  API::MatrixWorkspace_sptr m_matrixWorkspace;
};

} // namespace CurveFitting
} // namespace Mantid
