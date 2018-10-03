// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_API_CATALOGSESSION_H_
#define MANTID_API_CATALOGSESSION_H_

#include "MantidAPI/DllConfig.h"
#include "boost/shared_ptr.hpp"
#include <string>

namespace Mantid {
namespace API {
/**
 This class is a responsible for storing session information for a specific
 catalog.

 @author Jay Rainey, ISIS Rutherford Appleton Laboratory
 @date 27/02/2014
*/
class MANTID_API_DLL CatalogSession {
public:
  CatalogSession(const std::string &sessionID, const std::string &facility,
                 const std::string &endpoint);
  std::string getSessionId() const;
  void setSessionId(const std::string &sessionID);
  const std::string &getSoapEndpoint() const;
  const std::string &getFacility() const;

private:
  std::string m_sessionID;
  std::string m_facility;
  std::string m_endpoint;
};

using CatalogSession_sptr = boost::shared_ptr<CatalogSession>;
using CatalogSession_const_sptr = boost::shared_ptr<const CatalogSession>;
} // namespace API
} // namespace Mantid

#endif /* MANTID_API_CATALOGSESSION_H_ */
