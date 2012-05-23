#ifndef MANTID_MDALGORITHMS_SIMULATERESOLUTIONCONVOLVEDMODEL_H_
#define MANTID_MDALGORITHMS_SIMULATERESOLUTIONCONVOLVEDMODEL_H_

#include "MantidMDAlgorithms/Quantification/FitResolutionConvolvedModel.h"

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
    class DLLExport SimulateResolutionConvolvedModel : public FitResolutionConvolvedModel
    {
    public:
      virtual const std::string name() const;
      virtual int version() const;

    private:
      virtual void initDocs();

      /// Returns the number of iterations that should be performed
      virtual unsigned int niterations() const;

      void init();
    };


  } // namespace MDAlgorithms
} // namespace Mantid

#endif  /* MANTID_MDALGORITHMS_SIMULATERESOLUTIONCONVOLVEDMODEL_H_ */
