// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

#ifndef MANTID_API_CITATION_CITATION_H_
#define MANTID_API_CITATION_CITATION_H_

#include "MantidAPI/CitationConstructorHelpers.h"
#include "MantidAPI/DllConfig.h"

#include <string>
#include <vector>

#include <boost/optional/optional.hpp>

namespace Mantid {
namespace API {

class MANTID_API_DLL Citation {
public:
  Citation();
  Citation(const std::string &doi = "", const std::string &bibtex = "",
           const std::string &endnote = "", const std::string &url = "",
           const std::string &description = "");

  // Needed for future Set constructiom
  bool operator==(const Citation &rhs) const;

  const std::string &description() const;
  const std::string &url() const;
  const std::string &doi() const;
  const std::string &bibtex() const;
  const std::string &endnote() const;

  void loadFromNexus(const std::string &filename);
  void saveToNexus(const std::string &filename);

private:
  // Optional
  std::string m_description;

  // If this is set doi, bibtex or endote are provided then url must be.
  std::string m_url;

  std::string m_doi;
  std::string m_bibtex;
  std::string m_endnote;
};

Citation MANTID_API_DLL getCitation(const BaseCitation &cite);

} // namespace API
} // namespace Mantid

#endif /* MANTID_API_CITATION_CITATION_H_ */