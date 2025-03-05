// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/DllConfig.h"
#include "MantidNexus/NeXusFile.hpp"

#include <string>
#include <vector>

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
   * There are some strict parameters that the function arguments must adhere
   * to:
   * - description is always optional (this isn't needed for citation, but gives
   *   insight as to why this citation is relevant)
   * - if bibtex is provided endnote must also be provided, and vice-versa
   *   (BibTex and Endnote contain essentially the same information, they can
   *   both be created if one can be. BibTex and Endnote do not imply a DOI is
   *   minted)
   * - if doi is provided, url, bibtex and endnote must all be provided (BibTex
   *   and Endnote can be generated from DOIs)
   * - if none of doi, bibtex or endnote are provided, url must be provided
   *   (there must be something there, even if this isn't citable a URL is
   *   better than nothing)
   *
   * @param doi - https://www.baylor.edu/lib/electrres/index.php?id=49231
   * @param bibtex - https://www.economics.utoronto.ca/osborne/latex/BIBTEX.HTM
   * @param endnote - https://en.wikipedia.org/wiki/RIS_(file_format)
   * @param url - The website url for which the citation refers
   * @param description - The description of the thing that is being cited
   */
  Citation(const std::string &doi = "", const std::string &bibtex = "", const std::string &endnote = "",
           const std::string &url = "", const std::string &description = "");

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
