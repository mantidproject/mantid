// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

#ifndef MANTID_API_CITATION_CITATION_H_
#define MANTID_API_CITATION_CITATION_H_

#include "MantidAPI/DllConfig.h"

#include <string>
#include <vector>

#include <nexus/NeXusFile.hpp>

namespace Mantid {
namespace API {

class MANTID_API_DLL Citation {
public:
  /**
   * This constructor will load the data from the given file using the given
   * group as the NeXus Group in which the NXCite is contained.
   */
  Citation(::NeXus::File *file, const std::string &group);

  /**
   * An example Constructor will look like this:
   * Citation citation("doi", "@misc{Nobody06,\nauthor = "Nobody Jr",\ntitle =
   * "My Article",\nyear = "2006" }", "TY  - JOUR\nAU  - Shannon, Claude E.\nPY
   * - 1948/07//\nTI  - A Mathematical Theory of Communication\nT2  - Bell
   * System Technical Journal\nSP  - 379\nEP  - 423\nVL  - 27\nER  -", "url",
   * "description")
   */
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

  void saveNexus(::NeXus::File *file, const std::string &group);

private:
  void loadNexus(::NeXus::File *file, const std::string &group);

  std::string m_doi;
  std::string m_bibtex;
  std::string m_endnote;
  std::string m_url;
  std::string m_description;
};
} // namespace API
} // namespace Mantid

#endif /* MANTID_API_CITATION_CITATION_H_ */
