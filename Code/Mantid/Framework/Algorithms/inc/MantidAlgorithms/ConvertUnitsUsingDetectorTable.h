#ifndef MANTID_ALGORITHMS_CONVERTUNITSUSINGDETECTORTABLE_H_
#define MANTID_ALGORITHMS_CONVERTUNITSUSINGDETECTORTABLE_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"

namespace Mantid
{
namespace Algorithms
{

  /** ConvertUnitsUsingDetectorTable : Converts the units in which a workspace is represented.

    Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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
  class DLLExport ConvertUnitsUsingDetectorTable  : public API::Algorithm
  {
  public:
    ConvertUnitsUsingDetectorTable();
    virtual ~ConvertUnitsUsingDetectorTable();
    
    virtual const std::string name() const;
    virtual int version() const;
    virtual const std::string category() const;
    virtual const std::string summary() const;

  private:
    void init();
    void exec();

    void setupMemberVariables(const API::MatrixWorkspace_const_sptr inputWS);
    API::MatrixWorkspace_sptr setupOutputWorkspace(const API::MatrixWorkspace_const_sptr inputWS);
    void fillOutputHist(const API::MatrixWorkspace_const_sptr inputWS, const API::MatrixWorkspace_sptr outputWS);

    void putBackBinWidth(const API::MatrixWorkspace_sptr outputWS);


    /// Convert the workspace units according to a simple output = a * (input^b) relationship
    void convertQuickly(API::MatrixWorkspace_sptr outputWS, const double& factor, const double& power);
    /// Convert the workspace units using TOF as an intermediate step in the conversion
    void convertViaTOF(Kernel::Unit_const_sptr fromUnit, API::MatrixWorkspace_sptr outputWS);

    // Calls Rebin as a Child Algorithm to align the bins of the output workspace
    API::MatrixWorkspace_sptr alignBins(const API::MatrixWorkspace_sptr workspace);
    const std::vector<double> calculateRebinParams(const API::MatrixWorkspace_const_sptr workspace) const;

    /// Reverses the workspace if X values are in descending order
    void reverse(API::MatrixWorkspace_sptr workspace);

    /// For conversions to energy transfer, removes bins corresponding to inaccessible values
    API::MatrixWorkspace_sptr removeUnphysicalBins(const API::MatrixWorkspace_const_sptr workspace);

    std::size_t m_numberOfSpectra;     ///< The number of spectra in the input workspace
    bool m_distribution;       ///< Whether input is a distribution. Only applies to histogram workspaces.
    bool m_inputEvents;        ///< Flag indicating whether input workspace is an EventWorkspace
    Kernel::Unit_const_sptr m_inputUnit;  ///< The unit of the input workspace
    Kernel::Unit_sptr m_outputUnit;       ///< The unit we're going to

  };


} // namespace Algorithms
} // namespace Mantid

#endif  /* MANTID_ALGORITHMS_CONVERTUNITSUSINGDETECTORTABLE_H_ */
