#ifndef MANTID_MDALGORITHMS_FITRESOLUTIONCONVOLVEDMODEL_H_
#define MANTID_MDALGORITHMS_FITRESOLUTIONCONVOLVEDMODEL_H_

#include "MantidAPI/Algorithm.h"

namespace Mantid
{
  namespace MDAlgorithms
  {
    /**
      Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
    class DLLExport FitResolutionConvolvedModel : public API::Algorithm
    {
    public:
      const std::string name() const;
      int version() const;
      const std::string category() const;
      void initDocs();

    protected:
      /// Returns the number of iterations that should be performed
      virtual unsigned int niterations() const;
      /// Returns the name of the max iterations property
      std::string maxIterationsPropertyName() const;

      void init();
      void exec();

    private:
      /// Create the fitting sub algorithm
      API::IAlgorithm_sptr createFittingAlgorithm();
      /// Create the function string required by fit
      std::string createFunctionString() const;
    };


  } // namespace MDAlgorithms
} // namespace Mantid

#endif  /* MANTID_MDALGORITHMS_FITRESOLUTIONCONVOLVEDMODEL_H_ */
