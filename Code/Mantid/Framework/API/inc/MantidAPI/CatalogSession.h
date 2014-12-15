#ifndef MANTID_API_CATALOGSESSION_H_
#define MANTID_API_CATALOGSESSION_H_

#include "MantidAPI/DllConfig.h"
#include "boost/shared_ptr.hpp"
#include <string>

namespace Mantid
{
  namespace API
  {
    /**
     This class is a responsible for storing session information for a specific catalog.

     @author Jay Rainey, ISIS Rutherford Appleton Laboratory
     @date 27/02/2014
     Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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

     File change history is stored at: <https://github.com/mantidproject/mantid>.
     Code Documentation is available at: <http://doxygen.mantidproject.org>
    */
    class MANTID_API_DLL CatalogSession
    {
      public:
        CatalogSession(const std::string &sessionID, const std::string &facility, const std::string &endpoint);
        std::string getSessionId() const;
        void setSessionId(const std::string &sessionID);
        const std::string& getSoapEndpoint() const;
        const std::string& getFacility() const;

      private:
        std::string m_sessionID;
        std::string m_facility;
        std::string m_endpoint;
    };

    typedef boost::shared_ptr<CatalogSession> CatalogSession_sptr;
    typedef boost::shared_ptr<const CatalogSession> CatalogSession_const_sptr;
  }
}

#endif /* MANTID_API_CATALOGSESSION_H_ */
