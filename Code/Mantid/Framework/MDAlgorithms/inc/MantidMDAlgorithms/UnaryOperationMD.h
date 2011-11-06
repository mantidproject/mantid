#ifndef MANTID_MDALGORITHMS_UNARYOPERATIONMD_H_
#define MANTID_MDALGORITHMS_UNARYOPERATIONMD_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/IMDWorkspace.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidMDEvents/MDHistoWorkspace.h"

namespace Mantid
{
namespace MDAlgorithms
{

  /** Abstract base class for unary operations (e.g. Log or Exp)
   * on MDWorkspaces.

    Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
  class DLLExport UnaryOperationMD  : public API::Algorithm
  {
  public:
    UnaryOperationMD();
    ~UnaryOperationMD();

    virtual const std::string name() const;
    virtual int version() const;
    virtual const std::string category() const;

  protected:
    /// The name of the input workspace property
    virtual const std::string inputPropName() const { return "InputWorkspace";}
    /// The name of the output workspace property
    virtual const std::string outputPropName() const { return "OutputWorkspace";}

    void init();
    void exec();

    /// Run the algorithm on a MDEventWorkspace
    virtual void execEvent(Mantid::API::IMDEventWorkspace_sptr out) = 0;

    /// Run the algorithm with a MDHistoWorkspace
    virtual void execHisto(Mantid::MDEvents::MDHistoWorkspace_sptr out) = 0;

    /// Input workspace
    Mantid::API::IMDWorkspace_sptr m_in;
    /// Output workspace
    Mantid::API::IMDWorkspace_sptr m_out;

  };


} // namespace MDAlgorithms
} // namespace Mantid

#endif  /* MANTID_MDALGORITHMS_UNARYOPERATIONMD_H_ */
