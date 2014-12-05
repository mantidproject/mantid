#ifndef MANTID_WORKFLOWALGORITHMS_DGSPROCESSDETECTORVANADIUM_H_
#define MANTID_WORKFLOWALGORITHMS_DGSPROCESSDETECTORVANADIUM_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"

namespace Mantid
{
  namespace WorkflowAlgorithms
  {

    /** DgsProcessDetectorVanadium : This is the algorithm responsible for processing the
    detector vanadium into the form needed for the normalisation of sample data in the
    convert to energy transfer process.

    @date 2012-07-25

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
    class DLLExport DgsProcessDetectorVanadium  : public API::Algorithm
    {
    public:
      DgsProcessDetectorVanadium();
      virtual ~DgsProcessDetectorVanadium();

      virtual const std::string name() const;
      ///Summary of algorithms purpose
      virtual const std::string summary() const {return "Algorithm to process detector vanadium.";}

      virtual int version() const;
      virtual const std::string category() const;

    private:
      void init();
      void exec();
    };

  } // namespace WorkflowAlgorithms
} // namespace Mantid

#endif  /* MANTID_WORKFLOWALGORITHMS_DGSPROCESSDETECTORVANADIUM_H_ */
