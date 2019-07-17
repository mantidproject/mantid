// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidAPI/Citation/CitationConstructorHelpers.h"

namespace Mantid {
namespace API {
namespace Citation {

namespace {
template <typename T> std::string join(T list...) {
  std::string joinedString{""};
  for (const auto &elem : list) {
    joinedString += elem;
  }
}

std::string addToBibTex(const std::string &type, const std::string &data) {
  return type + "{" + data + "},\n";
}

std::string addToBibTex(const std::string &type, const OptionalString &data) {
  if (data)
    return addToBibTex(type, data.get());
  else
    return "";
}

std::string authorStringGenerator(const std::vector<std::string> &authors) {
  std::string authorString = "";
  if (authors.size() == 1)
    authorString = authors[0];
  else {
    for (const auto &author : authors)
      authorString += author + ", ";
    // Pop off the last space and comma from authorString
    authorString.pop_back();
    authorString.pop_back();
  }
  return authorString;
}

std::string authorStringGenerator(const OptionalVectorString &authors) {
  if (authors)
    return authorStringGenerator(authors.get());
  else
    return "";
}

std::string addToEndNote(const std::string &type, const std::string &data) {
  return type + data + "\n";
}

std::string addToEndNote(const std::string &type, const OptionalString &data) {
  if (data)
    return addToEndNote(type, data.get());
  else
    return "";
}

std::string addAuthorsToEndNote(const std::string &type,
                                const std::vector<std::string> &authors) {
  if (authors.size() == 1)
    return addToEndNote(EndNote::AUTHOR, authors[0]);
  std::string endNote = "";
  for (const auto &author : authors) {
    endNote += addToEndNote(EndNote::AUTHOR, author);
  }
  return endNote;
}

std::string addAuthorsToEndNote(const std::string &type,
                                const OptionalVectorString &authors) {
  if (authors)
    return addAuthorsToEndNote(type, authors.get());
  else
    return "";
}

std::string makeEndNoteDate(const std::string &year, const std::string &month) {
  return year + "/" + month;
}

std::tuple<std::string, std::string>
makeEndNotePageNumbers(const std::string &pages) {
  std::vector<std::string> strs;
  boost::split(strs, pages, boost::is_any_of("-"));
  if (strs.size() > 1)
    return {strs[0], strs.pop_back()};
  else
    return {pages, pages};
}

} // namespace

namespace BibTex {
const std::string AUTHOR = "author=";
const std::string TITLE = "title=";
const std::string JOURNAL = "journal=";
const std::string YEAR = "year=";
const std::string MONTH = "month=";
const std::string VOLUME = "volume=";
const std::string NUMBER = "number=";
const std::string PAGES = "pages=";
const std::string DESCRIPTION = "note=";
const std::string PUBLISHER = "publisher=";
const std::string SERIES = "series=";
const std::string ADDRESS = "address=";
const std::string HOWPUBLISHED = "howpublished=";
const std::string EDITOR = "editor=";
const std::string CHAPTER = "chapter=";
const std::string TYPE = "type=";
const std::string BOOKTITLE = "booktitle=";
const std::string ORGANIZATION = "organization=";
const std::string SCHOOL = "school=";
const std::string INSTITUION = "institution=";
const std::string EDITION = "edition=";
const std::string DOI = "doi=";
const std::string ENDING = "\n}";
} // namespace BibTex

namespace EndNote {
const std::string AUTHOR = "AU  - ";
const std::string TITLE = "T1  - ";
const std::string JOURNAL = "T2  - ";
const std::string YEAR = "PY  - ";
// const std::string MONTH Replaced with Date
const std::string DATE = "DA  - ";
const std::string VOLUME = "VL  - ";
const std::string NUMBER = "IS  - ";
const std::string STARTPAGE = "SP  - ";
const std::string ENDPAGE = "EP  - ";
// const std::string PAGES = ""; Replaced with start and end pages
const std::string DESCRIPTION = "N1  - ";
const std::string PUBLISHER = "PB  - ";
const std::string SERIES = "C1  - ";
const std::string ADDRESS = "AD  - ";
const std::string HOWPUBLISHED = "BT  - ";
const std::string CHAPTER = "SE  - ";
const std::string TYPE = "M3  - ";
const std::string BOOKTITLE = "T2  - ";
const std::string ORGANIZATION = "PP  - ";
const std::string SCHOOL = "PP  - ";
const std::string INSTITUION = "PP  - ";
const std::string DOI = "DO  - ";
const std::string EDITION = "ET  - ";
const std::string EDITOR = "ED  - ";
const std::string ENDING = "EP  - \n";
} // namespace EndNote

BaseCitation::BaseCitation(const OptionalString &doi,
                           const OptionalString &description)
    : m_doi(doi), m_description(description) {}

ArticleCitation::ArticleCitation(
    const std::vector<std::string> &authors, const std::string &title,
    const std::string &journal, const std::string &year,
    const OptionalString &volume, const OptionalString &number,
    const OptionalString &pages, const OptionalString &month,
    const OptionalString &description, const OptionalString &doi)
    : BaseCitation(doi, description), m_authors(authors), m_title(title),
      m_journal(journal), m_volume(volume), m_number(number), m_pages(pages),
      m_month(month) {}

std::string ArticleCitation::toBibTex() const {
  using namespace BibTex;
  std::string bibTex = "@article{refference,\n";

  bibTex += addToBibTex(AUTHOR, authorStringGenerator(m_authors));
  bibTex += addToBibTex(TITLE, m_title);
  bibTex += addToBibTex(JOURNAL, m_journal);
  bibTex += addToBibTex(YEAR, m_year);
  bibTex += addToBibTex(VOLUME, m_volume);
  bibTex += addToBibTex(NUMBER, m_number);
  bibTex += addToBibTex(PAGES, m_pages);
  bibTex += addToBibTex(MONTH, m_month);
  bibTex += addToBibTex(DESCRIPTION, m_description);
  bibTex += addToBibTex(DOI, m_doi);

  // Remove a extra comma
  bibTex.pop_back();
  bibTex += ENDING;
  return bibTex;
}

std::string ArticleCitation::toEndNote() const {
  using namespace EndNote;
  std::string endNote = "TY  - JOUR";

  endNote += addAuthorsToEndNote(m_authors);
  endNote += addToEndNote(TITLE, m_title);
  endNote += addToEndNote(JOURNAL, m_journal);
  endNote += addToEndNote(VOLUME, m_volume);
  endNote += addToEndNote(NUMBER, m_number);
  auto [start, end] = makeEndNotePageNumbers(pages);
  endNote += addToEndNote(STARTPAGE, start);
  endNote += addToEndNote(ENDPAGE, end);
  if (m_month)
    endNote += addToEndNote(DATE, makeEndNoteDate(m_year, m_month));
  else
    endNote += addToEndNote(YEAR, m_year);
  endNote += addToEndNote(DESCRIPTION, m_description);
  endNote += addToEndNote(DOI, m_doi);
  endNote += ENDING;
  return endNote;
}

BookCitation::BookCitation(
    const std::vector<std::string> &authors, const std::string &title,
    const std::string &publisher, const std::string &year,
    const OptionalString &volume, const OptionalString &series,
    const OptionalString &address, const OptionalString &edition,
    const OptionalString &month, const OptionalString &description,
    const OptionalString &doi)
    : BaseCitation(doi, description), m_authors(authors), m_title(title),
      m_publisher(publisher), m_year(year), m_volume(volume), m_series(series),
      m_address(address), m_edition(edition), m_month(month) {}

std::string BookCitation::toBibTex() const {
  using namespace BibTex;
  std::string bibTex = "@book{refference,\n";

  bibTex += addToBibTex(AUTHOR, authorStringGenerator(m_authors));
  bibTex += addToBibTex(TITLE, m_title);
  bibTex += addToBibTex(PUBLISHER, m_publisher);
  bibTex += addToBibTex(YEAR, m_year);
  bibTex += addToBibTex(VOLUME, m_volume);
  bibTex += addToBibTex(SERIES, m_series);
  bibTex += addToBibTex(ADDRESS, m_address);
  bibTex += addToBibTex(EDITION, m_edition);
  bibTex += addToBibTex(MONTH, m_month);
  bibTex += addToBibTex(DESCRIPTION, m_description);
  bibTex += addToBibTex(DOI, m_doi);

  // Remove a extra comma
  bibTex.pop_back();
  bibTex += ENDING;
  return bibTex;
}

std::string BookCitation::toEndNote() const {
  using namespace EndNote;
  std::string endNote = "TY  - BOOK";

  endNote += addAuthorsToEndNote(m_authors);
  endNote += addToEndNote(TITLE, m_title);
  endNote += addToEndNote(PUBLISHER, m_publisher);
  endNote += addToEndNote(SERIES, m_series);
  endNote += addToEndNote(VOLUME, m_volume);
  endNote += addToEndNote(ADDRESS, m_address);
  endNote += addToEndNote(EDITION, m_edition);
  if (month)
    endNote += addToEndNote(DATE, makeEndNoteDate(m_year, m_month));
  else
    endNote += addToEndNote(YEAR, m_year);
  endNote += addToEndNote(DESCRIPTION, m_description);
  endNote += addToEndNote(DOI, m_doi);
  endNote += ENDING;
  return endNote;
}

BookletCitation::BookletCitation(
    const std::string &title, const OptionalVectorString &author,
    const OptionalString &howPublished, const OptionalString &address,
    const OptionalString &month, const OptionalString &year,
    const OptionalString &description, const OptionalString &doi)
    : BaseCitation(doi, description), m_title(title), m_author(author),
      m_howPublished(howPublished), m_address(address), m_month(month),
      m_year(year) {}

std::string BookletCitation::toBibTex() const {
  using namespace BibTex;
  std::string bibTex = "@booklet{refference,\n";

  bibTex += addToBibTex(AUTHOR, authorStringGenerator(m_author));
  bibTex += addToBibTex(TITLE, m_title);
  bibTex += addToBibTex(HOWPUBLISHED, m_howPublished);
  bibTex += addToBibTex(ADDRESS, m_address);
  bibTex += addToBibTex(MONTH, m_month);
  bibTex += addToBibTex(YEAR, m_year);
  bibTex += addToBibTex(DESCRIPTION, m_description);
  bibTex += addToBibTex(DOI, m_doi);

  // Remove a extra comma
  bibTex.pop_back();
  bibTex += ENDING;
  return bibTex;
}

std::string BookletCitation::toEndNote() const {
  using namespace EndNote;
  std::string endNote = "TY  - PAMP";

  endNote += addAuthorsToEndNote(m_author);
  endNote += addToEndNote(TITLE, m_title);
  endNote += addToEndNote(HOWPUBLISHED, m_howPublished);
  endNote += addToEndNote(ADDRESS, m_address);
  if (year)
    if (month)
      endNote += addToEndNote(DATE, makeEndNoteDate(m_year, m_month));
    else
      endNote += addToEndNote(YEAR, m_year);
  endNote += addToEndNote(DESCRIPTION, m_description);
  endNote += addToEndNote(DOI, m_doi);
  endNote += ENDING;
  return endNote;
}

InBookCitation::InBookCitation(
    const std::vector<std::string> &authors, const std::string &title,
    const std::string &publisher, const std::string &year,
    const std::string &pages, const OptionalString &volume,
    const OptionalString &series, const OptionalString &type,
    const OptionalString &address, const OptionalString &edition,
    const OptionalString &month, const OptionalString &doi,
    const OptionalString &description)
    : BaseCitation(doi, description), m_authors(authors), m_title(title),
      m_publisher(publisher), m_year(year), m_pages(pages), m_volume(volume),
      m_series(series), m_type(type), m_address(address), m_edition(edition),
      m_month(month) {}

std::string InBookCitation::toBibTex() const {
  using namespace BibTex;
  std::string bibTex = "@inbook{refference,\n";

  bibTex += addToBibTex(AUTHOR, authorStringGenerator(m_authors));
  bibTex += addToBibTex(TITLE, m_title);
  bibTex += addToBibTex(PUBLISHER, m_publisher);
  bibTex += addToBibTex(YEAR, m_year);
  bibTex += addToBibTex(PAGES, m_pages);
  bibTex += addToBibTex(VOLUME, m_volume);
  bibTex += addToBibTex(SERIES, m_series);
  bibTex += addToBibTex(TYPE, m_type);
  bibTex += addToBibTex(ADDRESS, m_address);
  bibTex += addToBibTex(EDITION, m_edition);
  bibTex += addToBibTex(MONTH, m_month);
  bibTex += addToBibTex(DESCRIPTION, m_description);
  bibTex += addToBibTex(DOI, m_doi);

  // Remove a extra comma
  bibTex.pop_back();
  bibTex += ENDING;
  return bibTex;
}

std::string InBookCitation::toEndNote() const {
  using namespace EndNote;
  std::string endNote = "TY  - BOOK";

  endNote += addAuthorsToEndNote(m_authors);
  endNote += addToEndNote(TITLE, m_title);
  endNote += addToEndNote(PUBLISHER, m_publisher);
  endNote += addToEndNote(SERIES, m_series);
  endNote += addToEndNote(VOLUME, m_volume);
  endNote += addToEndNote(ADDRESS, m_address);
  endNote += addToEndNote(EDITION, m_edition);
  endNote += addToEndNote(TYPE, m_type);
  if (month)
    endNote += addToEndNote(DATE, makeEndNoteDate(m_year, m_month));
  else
    endNote += addToEndNote(YEAR, m_year);
  endNote += addToEndNote(DESCRIPTION, m_description);
  endNote += addToEndNote(DOI, m_doi);
  endNote += ENDING;
  return endNote;
}

InCollectionCitation::InCollectionCitation(
    const std::vector<std::string> &authors, const std::string &title,
    const std::string &booktitle, const std::string &publisher,
    const std::string &year, const OptionalString &volume,
    const OptionalString &series, const OptionalString &type,
    const OptionalString &chapter, const OptionalString &pages,
    const OptionalString &address, const OptionalString &edition,
    const OptionalString &month, const OptionalString &doi,
    const OptionalString &description)
    : BaseCitation(doi, description), m_authors(authors), m_title(title),
      m_booktitle(booktitle), m_publisher(publisher), m_year(year),
      m_volume(volume), m_series(series), m_type(type), m_chapter(chapter),
      m_pages(pages), m_address(address), m_edition(edition), m_month(month) {}

std::string InCollectionCitation::toBibTex() const {
  using namespace BibTex;
  std::string bibTex = "@incollection{refference,\n";

  bibTex += addToBibTex(AUTHOR, authorStringGenerator(m_authors));
  bibTex += addToBibTex(TITLE, m_title);
  bibTex += addToBibTex(BOOKTITLE, m_booktitle);
  bibTex += addToBibTex(PUBLISHER, m_publisher);
  bibTex += addToBibTex(YEAR, m_year);
  bibTex += addToBibTex(VOLUME, m_volume);
  bibTex += addToBibTex(SERIES, m_series);
  bibTex += addToBibTex(TYPE, m_type);
  bibTex += addToBibTex(CHAPTER, m_chapter);
  bibTex += addToBibTex(PAGES, m_pages);
  bibTex += addToBibTex(ADDRESS, m_address);
  bibTex += addToBibTex(EDITION, m_edition);
  bibTex += addToBibTex(MONTH, m_month);
  bibTex += addToBibTex(DESCRIPTION, m_description);
  bibTex += addToBibTex(DOI, m_doi);

  // Remove a extra comma
  bibTex.pop_back();
  bibTex += ENDING;
  return bibTex;
}

std::string InCollectionCitation::toEndNote() const {
  using namespace EndNote;
  std::string endNote = "TY  - GEN\n";

  endNote += addAuthorsToEndNote(m_authors);
  endNote += addToEndNote(TITLE, m_title);
  endNote += addToEndNote(BOOKTITLE, m_booktitle);
  endNote += addToEndNote(PUBLISHER, m_publisher);
  endNote += addToEndNote(YEAR, m_year);
  endNote += addToEndNote(VOLUME, m_volume);
  endNote += addToEndNote(SERIES, m_series);
  endNote += addToEndNote(TYPE, m_type);
  endNote += addToEndNote(CHAPTER, m_chapter);
  auto [start, end] = makeEndNotePageNumbers(pages);
  endNote += addToEndNote(STARTPAGE, start);
  endNote += addToEndNote(ENDPAGE, end);
  endNote += addToEndNote(ADDRESS, m_address);
  endNote += addToEndNote(EDITION, m_edition);
  if (month)
    endNote += addToEndNote(DATE, makeEndNoteDate(m_year, m_month));
  else
    endNote += addToEndNote(YEAR, m_year);
  endNote += addToEndNote(DESCRIPTION, m_description);
  endNote += addToEndNote(DOI, m_doi);
  endNote += ENDING;
  return endNote;
}

InProceedingsCitation::InProceedingsCitation(
    const std::vector<std::string> &authors, const std::string &title,
    const std::string &booktitle, const std::string &year,
    const OptionalString &editor, const OptionalString &volume,
    const OptionalString &series, const OptionalString &pages,
    const OptionalString &address, const OptionalString &month,
    const OptionalString &organization, const OptionalString &publisher,
    const OptionalString &doi, const OptionalString &description)
    : BaseCitation(doi, description), m_authors(authors), m_title(title),
      m_booktitle(booktitle), m_year(year), m_editor(editor), m_volume(volume),
      m_series(series), m_pages(pages), m_address(address), m_month(month),
      m_organization(organization), m_publisher(publisher) {}

std::string InProceedingsCitation::toBibTex() const {
  using namespace BibTex;
  std::string bibTex = "@inproceedings{refference,\n";

  bibTex += addToBibTex(AUTHOR, authorStringGenerator(m_authors));
  bibTex += addToBibTex(TITLE, m_title);
  bibTex += addToBibTex(BOOKTITLE, m_booktitle);
  bibTex += addToBibTex(YEAR, m_year);
  bibTex += addToBibTex(EDITOR, m_editor);
  bibTex += addToBibTex(VOLUME, m_volume);
  bibTex += addToBibTex(SERIES, m_series);
  bibTex += addToBibTex(PAGES, m_pages);
  bibTex += addToBibTex(ADDRESS, m_address);
  bibTex += addToBibTex(MONTH, m_month);
  bibTex += addToBibTex(ORGANIZATION, m_organization);
  bibTex += addToBibTex(PUBLISHER, m_publisher);
  bibTex += addToBibTex(DESCRIPTION, m_description);
  bibTex += addToBibTex(DOI, m_doi);

  // Remove a extra comma
  bibTex.pop_back();
  bibTex += ENDING;
  return bibTex;
}

std::string InProceedingsCitation::toEndNote() const {
  using namespace EndNote;
  std::string endNote = "TY  - CONF\n";

  endNote += addAuthorsToEndNote(m_authors);
  endNote += addToEndNote(TITLE, m_title);
  endNote += addToEndNote(BOOKTITLE, m_booktitle);
  endNote += addToEndNote(YEAR, m_year);
  endNote += addToEndNote(EDITOR, m_editor);
  endNote += addToEndNote(VOLUME, m_volume);
  endNote += addToEndNote(SERIES, m_series);
  auto [start, end] = makeEndNotePageNumbers(pages);
  endNote += addToEndNote(STARTPAGE, start);
  endNote += addToEndNote(ENDPAGE, end);
  endNote += addToEndNote(ADDRESS, m_address);
  if (month)
    endNote += addToEndNote(DATE, makeEndNoteDate(m_year, m_month));
  else
    endNote += addToEndNote(YEAR, m_year);
  endNote += addToEndNote(ORGANIZATION, m_organization);
  endNote += addToEndNote(PUBLISHER, m_publisher);
  endNote += addToEndNote(DESCRIPTION, m_description);
  endNote += addToEndNote(DOI, m_doi);
  endNote += ENDING;
  return endNote;
}

ManualCitation::ManualCitation(
    const std::string &title, const OptionalVectorString &authors,
    const OptionalString &organization, const OptionalString &address,
    const OptionalString &edition, const OptionalString &month,
    const OptionalString &year, const OptionalString &doi,
    const OptionalString &description)
    : BaseCitation(doi, description), m_title(title), m_authors(authors),
      m_organization(organization), m_address(address), m_edition(edition),
      m_month(month), m_year(year) {}

std::string ManualCitation::toBibTex() const {
  using namespace BibTex;
  std::string bibTex = "@manual{refference,\n";

  bibTex += addToBibTex(AUTHOR, authorStringGenerator(m_authors));
  bibTex += addToBibTex(TITLE, m_title);
  bibTex += addToBibTex(YEAR, m_year);
  bibTex += addToBibTex(ADDRESS, m_address);
  bibTex += addToBibTex(MONTH, m_month);
  bibTex += addToBibTex(ORGANIZATION, m_organization);
  bibTex += addToBibTex(EDITION, m_edition);
  bibTex += addToBibTex(DESCRIPTION, m_description);
  bibTex += addToBibTex(DOI, m_doi);

  // Remove a extra comma
  bibTex.pop_back();
  bibTex += ENDING;
  return bibTex;
}

std::string ManualCitation::toEndNote() const {
  using namespace EndNote;
  std::string endNote = "TY  - GEN\n";

  endNote += addAuthorsToEndNote(m_authors);
  endNote += addToEndNote(TITLE, m_title);
  endNote += addToEndNote(ADDRESS, m_address);
  if (year)
    if (month)
      endNote += addToEndNote(DATE, makeEndNoteDate(m_year, m_month));
    else
      endNote += addToEndNote(YEAR, m_year);
  endNote += addToEndNote(ORGANIZATION, m_organization);
  endNote += addToEndNote(EDITION, m_edition);
  endNote += addToEndNote(DESCRIPTION, m_description);
  endNote += addToEndNote(DOI, m_doi);
  endNote += ENDING;
  return endNote;
}

MastersThesisCitation::MastersThesisCitation(
    const std::vector<std::string> &authors, const std::string &title,
    const std::string &school, const std::string &year,
    const OptionalString &type, const OptionalString &address,
    const OptionalString &month, const OptionalString &doi,
    const OptionalString &description)
    : BaseCitation(doi, description), m_authors(authors), m_title(title),
      m_school(school), m_year(year), m_type(type), m_address(address),
      m_month(month) {}

std::string MastersThesisCitation::toBibTex() const {
  using namespace BibTex;
  std::string bibTex = "@mastersthesis{refference,\n";

  bibTex += addToBibTex(AUTHOR, authorStringGenerator(m_authors));
  bibTex += addToBibTex(TITLE, m_title);
  bibTex += addToBibTex(SCHOOL, m_school);
  bibTex += addToBibTex(TYPE, m_type);
  bibTex += addToBibTex(YEAR, m_year);
  bibTex += addToBibTex(ADDRESS, m_address);
  bibTex += addToBibTex(MONTH, m_month);
  bibTex += addToBibTex(DESCRIPTION, m_description);
  bibTex += addToBibTex(DOI, m_doi);

  // Remove a extra comma
  bibTex.pop_back();
  bibTex += ENDING;
  return bibTex;
}

std::string MastersThesisCitation::toEndNote() const {
  using namespace EndNote;
  std::string endNote = "TY  - THES\n";

  endNote += addAuthorsToEndNote(m_authors);
  endNote += addToEndNote(TITLE, m_title);
  endNote += addToEndNote(SCHOOL, m_school);
  endNote += addToEndNote(TYPE, m_type);
  endNote += addToEndNote(ADDRESS, m_address);
  if (year)
    if (month)
      endNote += addToEndNote(DATE, makeEndNoteDate(m_year, m_month));
    else
      endNote += addToEndNote(YEAR, m_year);
  endNote += addToEndNote(DESCRIPTION, m_description);
  endNote += addToEndNote(DOI, m_doi);
  endNote += ENDING;
  return endNote;
}

MiscCitation::MiscCitation(const OptionalVectorString &authors,
                           const OptionalString &title,
                           const OptionalString &howpublished,
                           const OptionalString &month,
                           const OptionalString &year,
                           const OptionalString &doi,
                           const OptionalString &description)
    : BaseCitation(doi, description), m_authors(authors), m_title(title),
      m_howpublished(howpublished), m_month(month), m_year(year) {}

std::string MiscCitation::toBibTex() const {
  using namespace BibTex;
  std::string bibTex = "@misc{refference,\n";

  bibTex += addToBibTex(AUTHOR, authorStringGenerator(m_authors));
  bibTex += addToBibTex(TITLE, m_title);
  bibTex += addToBibTex(HOWPUBLISHED, m_howpublished);
  bibTex += addToBibTex(YEAR, m_year);
  bibTex += addToBibTex(MONTH, m_month);
  bibTex += addToBibTex(DESCRIPTION, m_description);
  bibTex += addToBibTex(DOI, m_doi);

  // Remove a extra comma
  bibTex.pop_back();
  bibTex += ENDING;
  return bibTex;
}

std::string MiscCitation::toEndNote() const {
  using namespace EndNote;
  std::string endNote = "TY  - GEN\n";

  endNote += addAuthorsToEndNote(m_authors);
  endNote += addToEndNote(TITLE, m_title);
  endNote += addToEndNote(HOWPUBLISHED, m_howpublished);
  if (year)
    if (month)
      endNote += addToEndNote(DATE, makeEndNoteDate(m_year, m_month));
    else
      endNote += addToEndNote(YEAR, m_year);
  endNote += addToEndNote(DESCRIPTION, m_description);
  endNote += addToEndNote(DOI, m_doi);
  endNote += ENDING;
  return endNote;
}

PHDThesisCitation::PHDThesisCitation(
    const std::vector<std::string> &authors, const std::string &title,
    const std::string &school, const std::string &year,
    const OptionalString &type, const OptionalString &address,
    const OptionalString &month, const OptionalString &doi,
    const OptionalString &description)
    : BaseCitation(doi, description), m_authors(authors), m_title(title),
      m_school(school), m_year(year), m_type(type), m_address(address),
      m_month(month) {}

std::string PHDThesisCitation::toBibTex() const {
  using namespace BibTex;
  std::string bibTex = "@phdthesis{refference,\n";

  bibTex += addToBibTex(AUTHOR, authorStringGenerator(m_authors));
  bibTex += addToBibTex(TITLE, m_title);
  bibTex += addToBibTex(SCHOOL, m_school);
  bibTex += addToBibTex(TYPE, m_type);
  bibTex += addToBibTex(YEAR, m_year);
  bibTex += addToBibTex(ADDRESS, m_address);
  bibTex += addToBibTex(MONTH, m_month);
  bibTex += addToBibTex(DESCRIPTION, m_description);
  bibTex += addToBibTex(DOI, m_doi);

  // Remove a extra comma
  bibTex.pop_back();
  bibTex += ENDING;
  return bibTex;
}

std::string PHDThesisCitation::toEndNote() const {
  using namespace EndNote;
  std::string endNote = "TY  - THES\n";

  endNote += addAuthorsToEndNote(m_authors);
  endNote += addToEndNote(TITLE, m_title);
  endNote += addToEndNote(SCHOOL, m_school);
  endNote += addToEndNote(TYPE, m_type);
  endNote += addToEndNote(ADDRESS, m_address);
  if (year)
    if (month)
      endNote += addToEndNote(DATE, makeEndNoteDate(m_year, m_month));
    else
      endNote += addToEndNote(YEAR, m_year);
  endNote += addToEndNote(DESCRIPTION, m_description);
  endNote += addToEndNote(DOI, m_doi);
  endNote += ENDING;
  return endNote;
}

ProceedingsCitation::ProceedingsCitation(
    const std::string &title, const std::string &year,
    const OptionalString &editor, const OptionalString &volume,
    const OptionalString &series, const OptionalString &address,
    const OptionalString &month, const OptionalString &organization,
    const OptionalString &publisher, const OptionalString &doi,
    const OptionalString &description)
    : BaseCitation(doi, description), m_title(title), m_year(year),
      m_editor(editor), m_volume(volume), m_series(series), m_address(address),
      m_month(month), m_organization(organization), m_publisher(publisher) {}

std::string ProceedingsCitation::toBibTex() const {
  using namespace BibTex;
  std::string bibTex = "@proceedings{refference,\n";

  bibTex += addToBibTex(TITLE, m_title);
  bibTex += addToBibTex(YEAR, m_year);
  bibTex += addToBibTex(EDITOR, m_editor);
  bibTex += addToBibTex(VOLUME, m_volume);
  bibTex += addToBibTex(SERIES, m_series);
  bibTex += addToBibTex(ADDRESS, m_address);
  bibTex += addToBibTex(ORGANIZATION, m_organization);
  bibTex += addToBibTex(PUBLISHER, m_publisher);
  bibTex += addToBibTex(MONTH, m_month);
  bibTex += addToBibTex(DESCRIPTION, m_description);
  bibTex += addToBibTex(DOI, m_doi);

  // Remove a extra comma
  bibTex.pop_back();
  bibTex += ENDING;
  return bibTex;
}

std::string ProceedingsCitation::toEndNote() const {
  using namespace EndNote;
  std::string endNote = "TY  - CONF\n";

  endNote += addToEndNote(TITLE, m_title);
  endNote += addToEndNote(EDITOR, m_editor);
  endNote += addToEndNote(VOLUME, m_volume);
  endNote += addToEndNote(SERIES, m_series);
  endNote += addToEndNote(ADDRESS, m_address);
  endNote += addToEndNote(ORGANIZATION, m_organization);
  endNote += addToEndNote(PUBLISHER, m_publisher);
  if (month)
    endNote += addToEndNote(DATE, makeEndNoteDate(m_year, m_month));
  else
    endNote += addToEndNote(YEAR, m_year);
  endNote += addToEndNote(DESCRIPTION, m_description);
  endNote += addToEndNote(DOI, m_doi);
  endNote += ENDING;
  return endNote;
}

TechReportCitation::TechReportCitation(
    const std::vector<std::string> &authors, const std::string &title,
    const std::string &institution, const std::string &year,
    const OptionalString &type, const OptionalString &number,
    const OptionalString &address, const OptionalString &month,
    const OptionalString &doi, const OptionalString &description)
    : BaseCitation(doi, description), m_authors(authors), m_title(title),
      m_institution(institution), m_year(year), m_type(type), m_number(number),
      m_address(address), m_month(month) {}

std::string TechReportCitation::toBibTex() const {
  using namespace BibTex;
  std::string bibTex = "@techreport{refference,\n";

  bibTex += addToBibTex(AUTHORS, authorStringGenerator(m_authors));
  bibTex += addToBibTex(TITLE, m_title);
  bibTex += addToBibTex(INSTITUTION, m_institution);
  bibTex += addToBibTex(YEAR, m_year);
  bibTex += addToBibTex(TYPE, m_type);
  bibTex += addToBibTex(NUMBER, m_number);
  bibTex += addToBibTex(ADDRESS, m_address);
  bibTex += addToBibTex(MONTH, m_month);
  bibTex += addToBibTex(DESCRIPTION, m_description);
  bibTex += addToBibTex(DOI, m_doi);

  // Remove a extra comma
  bibTex.pop_back();
  bibTex += ENDING;
  return bibTex;
}

std::string TechReportCitation::toEndNote() const {
  using namespace EndNote;
  std::string endNote = "TY  - RPRT\n";

  endNote += addAuthorsToEndNote(m_authors);
  endNote += addToEndNote(TITLE, m_title);
  endNote += addToEndNote(INSTITUION, m_institution);
  endNote += addToEndNote(TYPE, m_type);
  endNote += addToEndNote(NUMBER, m_number);
  endNote += addToEndNote(ADDRESS, m_address);
  if (month)
    endNote += addToEndNote(DATE, makeEndNoteDate(m_year, m_month));
  else
    endNote += addToEndNote(YEAR, m_year);
  endNote += addToEndNote(DESCRIPTION, m_description);
  endNote += addToEndNote(DOI, m_doi);
  endNote += ENDING;
  return endNote;
}

UnPublishedCitation::UnPublishedCitation(
    const std::vector<std::string> &authors, const std::string &title,
    const std::string &description, const OptionalString &month,
    const OptionalString &year, const OptionalString &doi)
    : BaseCitation(doi, description), m_authors(authors), m_title(title),
      m_month(month), m_year(year) {}

std::string UnPublishedCitation::toBibTex() const {
  using namespace BibTex;
  std::string bibTex = "@unpublished{refference,\n";
  bibTex += addToBibTex(AUTHORS, authorStringGenerator(m_authors));
  bibTex += addToBibTex(TITLE, m_title);
  bibTex += addToBibTex(DESCRIPTION, m_description);
  bibTex += addToBibTex(MONTH, m_month);
  bibTex += addToBibTex(YEAR, m_year);
  bibTex += addToBibTex(DOI, m_doi);
  // Remove a extra comma
  bibTex.pop_back();
  bibTex += ENDING;
  return bibTex;
}

std::string UnPublishedCitation::toEndNote() const {
  using namespace EndNote;
  std::string endNote = "TY  - UNPB\n";

  endNote += addAuthorsToEndNote(m_authors);
  endNote += addToEndNote(TITLE, m_title);
  if (month)
    endNote += addToEndNote(DATE, makeEndNoteDate(m_year, m_month));
  else
    endNote += addToEndNote(YEAR, m_year);
  endNote += addToEndNote(DESCRIPTION, m_description);
  endNote += addToEndNote(DOI, m_doi);
  endNote += ENDING;
  return endNote;
}
} // namespace Citation
} // namespace API
} // namespace Mantid