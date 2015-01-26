#ifndef MANTID_VATES_METADATAEXTRACTORUTILS_H_
#define MANTID_VATES_METADATAEXTRACTORUTILS_H_

#include "MantidAPI/IMDWorkspace.h"
#include "MantidAPI/IMDIterator.h"
#include <qwt_double_interval.h>
#include <string>


/**
 * Class with utility methdos to extract meta data information from a IMDWorkspace.
 *
 * @date November 21, 2014
 *
 * Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory
 *
 * This file is part of Mantid.
 *
 * Mantid is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * Mantid is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * File change history is stored at: <https://github.com/mantidproject/mantid>
 * Code Documentation is available at: <http://doxygen.mantidproject.org>
 */
namespace Mantid
{
  namespace VATES
  {

    class DLLExport MetaDataExtractorUtils
    {
      public:

        MetaDataExtractorUtils();

        ~MetaDataExtractorUtils();

        /**
          * Get the minimum, maximum pair from the workspace
          * @param workspace A pointer to the workspace
          * @return A pair of minimum and maximum values.
          */
        QwtDoubleInterval getMinAndMax(Mantid::API::IMDWorkspace_sptr workspace);

        /**
          * Extracts the instrument from the workspace.
          * @param A pointer to a workspace.
          * @returns The instrument. 
          */
        std::string extractInstrument(Mantid::API::IMDWorkspace_sptr workspace);

      private:
          /**
          * Get the range of data values from an MD iterator
          * @param it Iterator for a general MD workspace.
          * @retunrs A maximum and minimum pair.
          */
        QwtDoubleInterval getRange(Mantid::API::IMDIterator* it);

        double defaultMin;
        double defaultMax;
    };
  }
}
#endif