#ifndef MANTID_ALGORITHMS_STITCH1D_H_
#define MANTID_ALGORITHMS_STITCH1D_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"

namespace Mantid
{
namespace Algorithms
{

  /** Stitch1D : Stitches two Matrix Workspaces together into a single output.
    
    Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
  class DLLExport Stitch1D  : public API::Algorithm
  {
  public:
    Stitch1D();
    virtual ~Stitch1D();
    
    virtual const std::string name() const;
    virtual int version() const;
    virtual const std::string category() const;

  private:
    virtual void initDocs();
    void init();
    void exec();
    double getStartOverlap(const double& min, const double& max) const;
    double getEndOverlap(const double& min, const double& max) const;
    Mantid::MantidVec getRebinParams(Mantid::API::MatrixWorkspace_sptr& lhsWS, Mantid::API::MatrixWorkspace_sptr& rhsWS) const;
    Mantid::API::MatrixWorkspace_sptr rebin(Mantid::API::MatrixWorkspace_sptr& input, const Mantid::MantidVec& params);
    Mantid::API::MatrixWorkspace_sptr integration(Mantid::API::MatrixWorkspace_sptr& input, const double& start, const double& stop);

  };


} // namespace Algorithms
} // namespace Mantid

#endif  /* MANTID_ALGORITHMS_STITCH1D_H_ */
