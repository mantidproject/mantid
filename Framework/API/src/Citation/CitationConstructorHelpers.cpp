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
    return type + "{" + data.get() + "},\n";
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
const std::string AUTHOR = "A1  - ";
const std::string AUTHOR2 = "A2  - ";
const std::string AUTHOR3 = "A3  - ";
const std::string AUTHOR4 = "A4  - ";
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

ManualCitation::ManualCitation(
    const std::string &title, const OptionalVectorString &authors,
    const OptionalString &organization, const OptionalString &address,
    const OptionalString &edition, const OptionalString &month,
    const OptionalString &year, const OptionalString &doi,
    const OptionalString &description)
    : BaseCitation(doi, description), m_title(title), m_authors(authors),
      m_organization(organization), m_address(address), m_edition(edition),
      m_month(month), m_year(year) {}

MastersThesisCitation::MastersThesisCitation(
    const std::vector<std::string> &authors, const std::string &title,
    const std::string &school, const std::string &year,
    const OptionalString &type, const OptionalString &address,
    const OptionalString &month, const OptionalString &doi,
    const OptionalString &description)
    : BaseCitation(doi, description), m_authors(authors), m_title(title),
      m_school(school), m_year(year), m_type(type), m_address(address),
      m_month(month) {}

MiscCitation::MiscCitation(const OptionalVectorString &authors,
                           const OptionalString &title,
                           const OptionalString &howpublished,
                           const OptionalString &month,
                           const OptionalString &year,
                           const OptionalString &doi,
                           const OptionalString &description)
    : BaseCitation(doi, description), m_authors(authors), m_title(title),
      m_howpublished(howpublished), m_month(month), m_year(year) {}

PHDThesisCitation::PHDThesisCitation(
    const std::vector<std::string> &authors, const std::string &title,
    const std::string &school, const std::string &year,
    const OptionalString &type, const OptionalString &address,
    const OptionalString &month, const OptionalString &doi,
    const OptionalString &description)
    : BaseCitation(doi, description), m_authors(authors), m_title(title),
      m_school(school), m_year(year), m_type(type), m_address(address),
      m_month(month) {}

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

TechReportCitation::TechReportCitation(
    const std::vector<std::string> &authors, const std::string &title,
    const std::string &instituion, const std::string &year,
    const OptionalString &type, const OptionalString &number,
    const OptionalString &address, const OptionalString &month,
    const OptionalString &doi, const OptionalString &description)
    : BaseCitation(doi, description), m_authors(authors), m_title(title),
      m_institution(instituion), m_year(year), m_type(type), m_number(number),
      m_address(address), m_month(month) {}

UnPublishedCitation::UnPublishedCitation(
    const std::vector<std::string> &authors, const std::string &title,
    const std::string &description, const OptionalString &month,
    const OptionalString &year, const OptionalString &doi)
    : BaseCitation(doi, description), m_authors(authors), m_title(title),
      m_month(month), m_year(year) {}
} // namespace Citation
} // namespace API
} // namespace Mantid