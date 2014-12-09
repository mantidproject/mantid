#ifndef MANTID_MDALGORITHMS_FITRESOLUTIONCONVOLVEDMODEL_H_
#define MANTID_MDALGORITHMS_FITRESOLUTIONCONVOLVEDMODEL_H_

#include "MantidAPI/Algorithm.h"

namespace Mantid
{
  namespace MDAlgorithms
  {
    /**
      Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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
    class DLLExport FitResolutionConvolvedModel : public API::Algorithm
    {
    public:
      const std::string name() const;
    ///Summary of algorithms purpose
    virtual const std::string summary() const {return "Fits a cuts/slices from an MDEventWorkspace using a resolution function convolved with a foreground model";}

      int version() const;
      const std::string category() const;

    protected:
      /// Returns the number of iterations that should be performed
      virtual int niterations() const;
      /// Returns the name of the max iterations property
      std::string maxIterationsPropertyName() const;
      /// Returns the name of the output parameters property
      std::string outputParsPropertyName() const;
      /// Returns the name of the covariance matrix property
      std::string covMatrixPropertyName() const;
      /// Create the function string required by fit
      std::string createFunctionString() const;

      void init();
      void exec();

    private:
      /// Create the fitting Child Algorithm
      API::IAlgorithm_sptr createFittingAlgorithm();
    };


  } // namespace MDAlgorithms
} // namespace Mantid

#endif  /* MANTID_MDALGORITHMS_FITRESOLUTIONCONVOLVEDMODEL_H_ */
