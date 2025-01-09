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
#include "MantidCurveFitting/DllConfig.h"
#include "MantidKernel/cow_ptr.h"

namespace Mantid {
namespace HistogramData {
class Points;
class BinEdges;
} // namespace HistogramData
namespace CurveFitting {

/** FunctionDomain1DSpectrumCreator :

    FunctionDomain1DSpectrumCreator creates an FunctionDomain1DSpectrum using a
   given
    MatrixWorkspace and workspace index. Currently it does not create an output
   workspace,
    since it is exclusively used in a context where it's not required.

      @author Michael Wedel, Paul Scherrer Institut - SINQ
      @date 30/05/2014

  */

class MANTID_CURVEFITTING_DLL FunctionDomain1DSpectrumCreator : public API::IDomainCreator {
public:
  FunctionDomain1DSpectrumCreator();

  void setMatrixWorkspace(API::MatrixWorkspace_sptr matrixWorkspace);
  void setWorkspaceIndex(size_t workspaceIndex);

  void createDomain(std::shared_ptr<API::FunctionDomain> &domain, std::shared_ptr<API::FunctionValues> &values,
                    size_t i0 = 0) override;

  size_t getDomainSize() const override;

protected:
  void throwIfWorkspaceInvalid() const;

  API::MatrixWorkspace_sptr m_matrixWorkspace;
  size_t m_workspaceIndex;
  bool m_workspaceIndexIsSet;
};

} // namespace CurveFitting
} // namespace Mantid
