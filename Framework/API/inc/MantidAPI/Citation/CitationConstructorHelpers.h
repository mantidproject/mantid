#ifndef MANTID_API_CITATION_CITATIONCONSTRUCTORHELPERS_H_
#define MANTID_API_CITATION_CITATIONCONSTRUCTORHELPERS_H_

#include "MantidAPI/Citation/Citation.h"

#include <boost/optional.hpp>
#include <vector>

namespace Mantid {
namespace API {
namespace Citation {

using OptionalString = boost::optional<std::string>;
using OptionalVectorString = boost::optional<std::vector<std::string>>;

struct BaseCitation {
  BaseCitation();
  BaseCitation(const OptionalString &doi, const OptionalString &description);
  const OptionalString m_doi = boost::none;
  const OptionalString m_description = boost::none;

  virtual std::string toEndNote() const = 0;
  virtual std::string toBibTex() const = 0;
};

struct ArticleCitation : BaseCitation {
  ArticleCitation(const std::vector<std::string> &authors,
                  const std::string &title, const std::string &journal,
                  const std::string &year,
                  const OptionalString &volume = boost::none,
                  const OptionalString &number = boost::none,
                  const OptionalString &pages = boost::none,
                  const OptionalString &month = boost::none,
                  const OptionalString &description = boost::none,
                  const OptionalString &doi = boost::none);

  std::string toEndNote() const override;
  std::string toBibTex() const override;

  const std::vector<std::string> m_authors;
  const std::string m_title, m_journal, m_year;

  const OptionalString m_volume, m_number, m_pages, m_month;
};

struct BookCitation : BaseCitation {
  BookCitation(const std::vector<std::string> &authors,
               const std::string &title, const std::string &publisher,
               const std::string &year,
               const OptionalString &volume = boost::none,
               const OptionalString &series = boost::none,
               const OptionalString &address = boost::none,
               const OptionalString &edition = boost::none,
               const OptionalString &month = boost::none,
               const OptionalString &description = boost::none,
               const OptionalString &doi = boost::none);
  std::string toEndNote() const override;
  std::string toBibTex() const override;

  const std::vector<std::string> m_authors;
  const std::string m_title, m_publisher, m_year;

  const OptionalString m_volume, m_series, m_address, m_edition, m_month;
};

struct BookletCitation : BaseCitation {
  BookletCitation(const std::string &title,
                  const OptionalVectorString &author = boost::none,
                  const OptionalString &howPublished = boost::none,
                  const OptionalString &address = boost::none,
                  const OptionalString &month = boost::none,
                  const OptionalString &year = boost::none,
                  const OptionalString &description = boost::none,
                  const OptionalString &doi = boost::none);
  std::string toEndNote() const override;
  std::string toBibTex() const override;

  const std::string m_title;

  const OptionalVectorString m_author;
  const OptionalString m_howPublished, m_address, m_month, m_year;
};

// Pages assumes more than one would have a - seperator.
struct InBookCitation : BaseCitation {
  InBookCitation(const std::vector<std::string> &authors,
                 const std::string &title, const std::string &publisher,
                 const std::string &year, const std::string &pages,
                 const OptionalString &volume = boost::none,
                 const OptionalString &series = boost::none,
                 const OptionalString &type = boost::none,
                 const OptionalString &address = boost::none,
                 const OptionalString &edition = boost::none,
                 const OptionalString &month = boost::none,
                 const OptionalString &doi = boost::none,
                 const OptionalString &description = boost::none);
  std::string toEndNote() const override;
  std::string toBibTex() const override;

  const std::vector<std::string> m_authors;
  const std::string m_title, m_publisher, m_year, m_pages;

  const OptionalString m_volume, m_series, m_type, m_address, m_edition,
      m_month;
};

// Pages assumes more than one would have a - seperator.
struct InCollectionCitation : BaseCitation {
  InCollectionCitation(const std::vector<std::string> &authors,
                       const std::string &title, const std::string &booktitle,
                       const std::string &publisher, const std::string &year,
                       const OptionalString &volume = boost::none,
                       const OptionalString &series = boost::none,
                       const OptionalString &type = boost::none,
                       const OptionalString &chapter = boost::none,
                       const OptionalString &pages = boost::none,
                       const OptionalString &address = boost::none,
                       const OptionalString &edition = boost::none,
                       const OptionalString &month = boost::none,
                       const OptionalString &doi = boost::none,
                       const OptionalString &description = boost::none);
  std::string toEndNote() const override;
  std::string toBibTex() const override;

  const std::vector<std::string> m_authors;
  const std::string m_title, m_booktitle, m_publisher, m_year;

  const OptionalString m_volume, m_series, m_type, m_chapter, m_pages,
      m_address, m_edition, m_month;
};

// Pages assumes more than one would have a - seperator.
struct InProceedingsCitation : BaseCitation {
  InProceedingsCitation(const std::vector<std::string> &authors,
                        const std::string &title, const std::string &booktitle,
                        const std::string &year,
                        const OptionalString &editor = boost::none,
                        const OptionalString &volume = boost::none,
                        const OptionalString &series = boost::none,
                        const OptionalString &pages = boost::none,
                        const OptionalString &address = boost::none,
                        const OptionalString &month = boost::none,
                        const OptionalString &organization = boost::none,
                        const OptionalString &publisher = boost::none,
                        const OptionalString &doi = boost::none,
                        const OptionalString &description = boost::none);
  std::string toEndNote() const override;
  std::string toBibTex() const override;

  const std::vector<std::string> m_authors;
  const std::string m_title, m_booktitle, m_year;

  const OptionalString m_editor, m_volume, m_series, m_pages, m_address,
      m_month, m_organization, m_publisher;
};

struct ManualCitation : BaseCitation {
  ManualCitation(const std::string &title,
                 const OptionalVectorString &authors = boost::none,
                 const OptionalString &organization = boost::none,
                 const OptionalString &address = boost::none,
                 const OptionalString &edition = boost::none,
                 const OptionalString &month = boost::none,
                 const OptionalString &year = boost::none,
                 const OptionalString &doi = boost::none,
                 const OptionalString &description = boost::none);
  std::string toEndNote() const override;
  std::string toBibTex() const override;

  const std::string m_title;

  const OptionalVectorString m_authors;
  const OptionalString m_organization, m_address, m_edition, m_month, m_year;
};

struct MastersThesisCitation : BaseCitation {
  MastersThesisCitation(const std::vector<std::string> &authors,
                        const std::string &title, const std::string &school,
                        const std::string &year,
                        const OptionalString &type = boost::none,
                        const OptionalString &address = boost::none,
                        const OptionalString &month = boost::none,
                        const OptionalString &doi = boost::none,
                        const OptionalString &description = boost::none);
  std::string toEndNote() const override;
  std::string toBibTex() const override;

  const std::vector<std::string> m_authors;
  const std::string m_title, m_school, m_year;

  const OptionalString m_type, m_address, m_month;
};

struct MiscCitation : BaseCitation {
  MiscCitation(const OptionalVectorString &authors = boost::none,
               const OptionalString &title = boost::none,
               const OptionalString &howpublished = boost::none,
               const OptionalString &month = boost::none,
               const OptionalString &year = boost::none,
               const OptionalString &doi = boost::none,
               const OptionalString &description = boost::none);
  std::string toEndNote() const override;
  std::string toBibTex() const override;

  const OptionalVectorString m_authors;
  const OptionalString m_title, m_howpublished, m_month, m_year;
};

struct PHDThesisCitation : BaseCitation {
  PHDThesisCitation(const std::vector<std::string> &authors,
                    const std::string &title, const std::string &school,
                    const std::string &year,
                    const OptionalString &type = boost::none,
                    const OptionalString &address = boost::none,
                    const OptionalString &month = boost::none,
                    const OptionalString &doi = boost::none,
                    const OptionalString &description = boost::none);
  std::string toEndNote() const override;
  std::string toBibTex() const override;

  const std::vector<std::string> m_authors;
  const std::string m_title, m_school, m_year;

  const OptionalString m_type, m_address, m_month;
};

struct ProceedingsCitation : BaseCitation {
  ProceedingsCitation(const std::string &title, const std::string &year,
                      const OptionalString &editor = boost::none,
                      const OptionalString &volume = boost::none,
                      const OptionalString &series = boost::none,
                      const OptionalString &address = boost::none,
                      const OptionalString &month = boost::none,
                      const OptionalString &organization = boost::none,
                      const OptionalString &publisher = boost::none,
                      const OptionalString &doi = boost::none,
                      const OptionalString &description = boost::none);
  std::string toEndNote() const override;
  std::string toBibTex() const override;

  const std::string m_title, m_year;

  const OptionalString m_editor, m_volume, m_series, m_address, m_month,
      m_organization, m_publisher;
};

struct TechReportCitation : BaseCitation {
  TechReportCitation(const std::vector<std::string> &authors,
                     const std::string &title, const std::string &instituion,
                     const std::string &year,
                     const OptionalString &type = boost::none,
                     const OptionalString &number = boost::none,
                     const OptionalString &address = boost::none,
                     const OptionalString &month = boost::none,
                     const OptionalString &doi = boost::none,
                     const OptionalString &description = boost::none);
  std::string toEndNote() const override;
  std::string toBibTex() const override;

  const std::vector<std::string> m_authors;
  const std::string m_title, m_institution, m_year;

  const OptionalString m_type, m_number, m_address, m_month;
};

struct UnPublishedCitation : BaseCitation {
  UnPublishedCitation(const std::vector<std::string> &authors,
                      const std::string &title, const std::string &description,
                      const OptionalString &month = boost::none,
                      const OptionalString &year = boost::none,
                      const OptionalString &doi = boost::none);
  std::string toEndNote() const override;
  std::string toBibTex() const override;

  const std::vector<std::string> m_authors;
  const std::string m_title;

  const OptionalString m_month, m_year;
};

} // namespace Citation
} // namespace API
} // namespace Mantid
#endif /* MANTID_API_CITATION_CITATION_H_ */