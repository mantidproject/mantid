#ifndef MANTID_CURVEFITTING_CALCULATEGAMMABACKGROUND_H_
#define MANTID_CURVEFITTING_CALCULATEGAMMABACKGROUND_H_

#include "MantidAPI/Algorithm.h"

namespace Mantid
{
  namespace CurveFitting
  {

    /**

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
    class DLLExport CalculateGammaBackground : public API::Algorithm
    {
    public:
      CalculateGammaBackground();

    private:

      const std::string name() const;
      int version() const;
      const std::string category() const;

      void initDocs();
      void init();
      void exec();

      /// Check and store appropriate input data
      void retrieveInputs();
      /// Create the output workspaces
      void createOutputWorkspaces();
      /// Calculate & correct the given index of the input workspace
      void applyCorrection(const size_t index);

      API::MatrixWorkspace_const_sptr m_inputWS;
      size_t m_npeaks;
      std::vector<double> m_masses;
      std::vector<double> m_amplitudes;
      std::vector<double> m_widths;

      API::MatrixWorkspace_sptr m_backgroundWS;
      API::MatrixWorkspace_sptr m_correctedWS;

    };


  } // namespace CurveFitting
} // namespace Mantid

#endif  /* MANTID_CURVEFITTING_CALCULATEGAMMABACKGROUND_H_ */
