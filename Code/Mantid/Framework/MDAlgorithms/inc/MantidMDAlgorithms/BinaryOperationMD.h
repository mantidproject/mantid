#ifndef MANTID_MDALGORITHMS_BINARYOPERATIONMD_H_
#define MANTID_MDALGORITHMS_BINARYOPERATIONMD_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/IMDWorkspace.h"
#include "MantidMDEvents/MDEventWorkspace.h"
#include "MantidMDEvents/MDHistoWorkspace.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidDataObjects/WorkspaceSingleValue.h"

namespace Mantid
{
namespace MDAlgorithms
{

  /** Abstract base class for binary operations on IMDWorkspaces,
    e.g. A = B + C or A = B / C.

    Handles most of the validation and delegates to a handful of execXXX functions.

    This will be subclassed by, e.g. PlusMD, MinusMD, MultiplyMD, etc.
    
    @author Janik Zikovsky
    @date 2011-11-04

    Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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
  class DLLExport BinaryOperationMD  : public API::Algorithm
  {
  public:
    BinaryOperationMD();
    virtual ~BinaryOperationMD();
    
    virtual const std::string name() const;
    virtual int version() const;
    virtual const std::string category() const;
    virtual const std::string summary() const { return "Abstract base class for binary operations on IMDWorkspaces, e.g. A = B + C or A = B / C."; }
    
  protected:
    /// Is the operation commutative?
    virtual bool commutative() const = 0;

    /// Check the inputs and throw if the algorithm cannot be run
    virtual void checkInputs() = 0;

    /// Run the algorithm with an MDEventWorkspace as output
    virtual void execEvent() = 0;

    /// Run the algorithm with a MDHisotWorkspace as output and operand
    virtual void execHistoHisto(Mantid::MDEvents::MDHistoWorkspace_sptr out, Mantid::MDEvents::MDHistoWorkspace_const_sptr operand) = 0;

    /// Run the algorithm with a MDHisotWorkspace as output, scalar and operand
    virtual void execHistoScalar(Mantid::MDEvents::MDHistoWorkspace_sptr out, Mantid::DataObjects::WorkspaceSingleValue_const_sptr scalar) = 0;

    /// The name of the first input workspace property
    virtual std::string inputPropName1() const { return "LHSWorkspace";}
    /// The name of the second input workspace property
    virtual std::string inputPropName2() const { return "RHSWorkspace";}
    /// The name of the output workspace property
    virtual std::string outputPropName() const { return "OutputWorkspace";}

    void init();
    virtual void initExtraProperties();
    virtual void exec();

    /// LHS workspace
    Mantid::API::IMDWorkspace_sptr m_lhs;
    /// RHS workspace
    Mantid::API::IMDWorkspace_sptr m_rhs;
    /// Output workspace
    Mantid::API::IMDWorkspace_sptr m_out;

    /// For checkInputs
    Mantid::API::IMDEventWorkspace_sptr m_lhs_event;
    Mantid::API::IMDEventWorkspace_sptr m_rhs_event;
    Mantid::MDEvents::MDHistoWorkspace_sptr m_lhs_histo;
    Mantid::MDEvents::MDHistoWorkspace_sptr m_rhs_histo;
    Mantid::DataObjects::WorkspaceSingleValue_sptr m_lhs_scalar;
    Mantid::DataObjects::WorkspaceSingleValue_sptr m_rhs_scalar;

    /// Operand MDEventWorkspace
    Mantid::API::IMDEventWorkspace_sptr m_operand_event;
    /// Output MDEventWorkspace
    Mantid::API::IMDEventWorkspace_sptr m_out_event;

    /// Operand MDHistoWorkspace
    Mantid::MDEvents::MDHistoWorkspace_sptr m_operand_histo;
    /// Output MDHistoWorkspace
    Mantid::MDEvents::MDHistoWorkspace_sptr m_out_histo;

    /// Operand WorkspaceSingleValue
    Mantid::DataObjects::WorkspaceSingleValue_sptr m_operand_scalar;


  };


} // namespace MDAlgorithms
} // namespace Mantid

#endif  /* MANTID_MDALGORITHMS_BINARYOPERATIONMD_H_ */
