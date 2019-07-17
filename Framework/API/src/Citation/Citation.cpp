// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidAPI/Citation/Citation.h"

namespace Mantid {
namespace API {
namespace Citation {

/**
  This function is designed for construction and validation for loading, however
  will inherently work with getCitation.

  - description is always optional (this isn't needed for citation, but gives
  insight as to why this citation is relevant)
  - if bibtex is provided endnote must also be provided, and vice-versa (BibTex
  and Endnote contain essentially the same information, they can both be created
  if one can be. BibTex and Endnote do not imply a DOI is minted)
   - if doi is provided, url, bibtex and endnote must all be provided (BibTex
  and Endnote can be generated from DOIs)
    - if none of doi, bibtex or endnote are provided, url must be provided
  (there must be something there, even if this isn't citable a URL is better
  than nothing)
 */

Citation::Citation(const OptionalString &doi, const OptionalString &bibtex,
                   const OptionalString &endnote, const OptionalString &url,
                   const OptionalString &description) {
  if (!doi && !bibtex && !endnote && !url && !description)
    throw std::invalid_argument("No arguements were given!");

  // This is an initial implementation that expects it but it should be possible
  // to generate one from the other
  if ((bibtex && !endnote) || (!bibtex && endnote))
    throw std::invalid_argument(
        "If bibtex is provided, endnote must also be provided and vice-versa");

  // If doi is provided then url, bibtex and endnote must be.
  if (doi && (!bibtex || !endnote || !url))
    throw std::invalid_argument(
        "If doi is provided then url, bibtex and endnote must be");

  if (!doi && !bibtex && !endnote)
    if (!url)
      throw std::invalid_argument(
          "If none of doi, bibtex, or endnote is provided, then url must be");

  if (doi)
    m_doi = doi.get();

  if (bibtex)
    m_bibtex = doi.get();

  if (endnote)
    m_endnote = endnote.get();

  if (url)
    m_url = url.get();
}
} // namespace Citation
} // namespace API
} // namespace Mantid