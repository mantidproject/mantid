// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef DATAHANDLING_XMLHANDLER_H_
#define DATAHANDLING_XMLHANDLER_H_

#include "DllConfig.h"

#include <Poco/AutoPtr.h>
#include <Poco/DOM/Document.h>

#include <map>
#include <string>
#include <vector>

namespace Mantid {
namespace DataHandling {

class MANTID_DATAHANDLING_DLL XmlHandler {
public:
  XmlHandler() = default;
  XmlHandler(std::string);

  std::map<std::string, std::string>
  get_metadata(const std::vector<std::string> &tags_to_ignore);
  std::string get_text_from_tag(const std::string &);
  std::map<std::string, std::string>
  get_attributes_from_tag(const std::string &);
  std::vector<std::string> get_subnodes(const std::string &);

private:
  Poco::AutoPtr<Poco::XML::Document> pDoc;
};
} // namespace DataHandling
} // namespace Mantid

#endif /* DATAHANDLING_XMLHANDLER_H_ */
