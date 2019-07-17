// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

#ifndef MANTID_API_CITATION_CITATION_H_
#define MANTID_API_CITATION_CITATION_H_

#include <string>
#include <vector>

#include <boost/optional/optional.hpp>

namespace Mantid {
namespace API {
namespace Citation {

using OptionalString = boost::optional<std::string>;
using OptionalVectorString = boost::optional<std::vector<std::string>>;

class Citation {
public:
  Citation(const OptionalString &doi = boost::none,
           const OptionalString &bibtex = boost::none,
           const OptionalString &endnote = boost::none,
           const OptionalString &url = boost::none,
           const OptionalString &description = boost::none);

  // Needed for future Set constructiom
  bool operator==(const Citation &rhs) const;

private:
  // Optional
  std::string m_description;

  // If this is set doi, bibtex or endote are provided then url must be.
  std::string m_url;

  std::string m_doi;
  std::string m_bibtex;
  std::string m_endnote;
};

Citation fromNexus(const std::string &filename);
void addToNexus(const std::string &filename, const Citation &cite);

Citation getCitation(const BaseCitation &cite);

} // namespace Citation
} // namespace API
} // namespace Mantid

#endif /* MANTID_API_CITATION_CITATION_H_ */