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

using OptionalString = boost::optional<std::string>;
using OptionalCharStar = boost::optional<const char *>;
using OptionalVectorString = boost::optional<std::vector<std::string>>;

namespace {
OptionalString
convertOptionalCharStarToString(const OptionalCharStar &charStar) {
  if (charStar)
    return boost::optional<std::string>(std::string(charStar.get()));
  else
    return boost::none;
}
} // namespace

class MANTID_API_DLL Citation {
public:
  Citation();
  Citation(const OptionalString &doi,
           const OptionalString &bibtex = boost::none,
           const OptionalString &endnote = boost::none,
           const OptionalString &url = boost::none,
           const OptionalString &description = boost::none);

  Citation(const OptionalCharStar &doi,
           const OptionalCharStar &bibtex = boost::none,
           const OptionalCharStar &endnote = boost::none,
           const OptionalCharStar &url = boost::none,
           const OptionalCharStar &description = boost::none)
      : Citation(convertOptionalCharStarToString(doi),
                 convertOptionalCharStarToString(bibtex),
                 convertOptionalCharStarToString(endnote),
                 convertOptionalCharStarToString(url),
                 convertOptionalCharStarToString(description)) {}

  // Needed for future Set constructiom
  bool operator==(const Citation &rhs) const;

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

Citation getCitation(const BaseCitation &cite);

} // namespace API
} // namespace Mantid

#endif /* MANTID_API_CITATION_CITATION_H_ */