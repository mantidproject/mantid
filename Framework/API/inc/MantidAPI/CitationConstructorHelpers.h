#ifndef MANTID_API_CITATION_CITATIONCONSTRUCTORHELPERS_H_
#define MANTID_API_CITATION_CITATIONCONSTRUCTORHELPERS_H_

#include "DllConfig.h"

#include <boost/optional.hpp>
#include <vector>

namespace Mantid {
namespace API {

struct MANTID_API_DLL BaseCitation {
  BaseCitation() = delete;
  BaseCitation(const std::string &doi = "", const std::string &description = "",
               const std::string &url = "");
  const std::string m_doi, m_description, m_url;

  virtual std::string toEndNote() const = 0;
  virtual std::string toBibTex() const = 0;
};

struct MANTID_API_DLL ArticleCitation : BaseCitation {
  ArticleCitation(const std::vector<std::string> &authors,
                  const std::string &title, const std::string &journal,
                  const std::string &year, const std::string &volume = "",
                  const std::string &number = "", const std::string &pages = "",
                  const std::string &month = "",
                  const std::string &description = "",
                  const std::string &doi = "", const std::string &url = "");

  std::string toEndNote() const override;
  std::string toBibTex() const override;

  const std::vector<std::string> m_authors;
  const std::string m_title, m_journal, m_year, m_volume, m_number, m_pages,
      m_month;
};

struct MANTID_API_DLL BookCitation : BaseCitation {
  BookCitation(const std::vector<std::string> &authors,
               const std::string &title, const std::string &publisher,
               const std::string &year, const std::string &volume = "",
               const std::string &series = "", const std::string &address = "",
               const std::string &edition = "", const std::string &month = "",
               const std::string &description = "", const std::string &doi = "",
               const std::string &url = "");
  std::string toEndNote() const override;
  std::string toBibTex() const override;

  const std::vector<std::string> m_authors;
  const std::string m_title, m_publisher, m_year, m_volume, m_series, m_address,
      m_edition, m_month;
};

struct MANTID_API_DLL BookletCitation : BaseCitation {
  BookletCitation(const std::string &title,
                  const std::vector<std::string> &author = {},
                  const std::string &howPublished = "",
                  const std::string &address = "",
                  const std::string &month = "", const std::string &year = "",
                  const std::string &description = "",
                  const std::string &doi = "", const std::string &url = "");
  std::string toEndNote() const override;
  std::string toBibTex() const override;

  const std::string m_title;
  const std::vector<std::string> m_author;
  const std::string m_howPublished, m_address, m_month, m_year;
};

// Pages assumes more than one would have a - seperator.
struct MANTID_API_DLL InBookCitation : BaseCitation {
  InBookCitation(const std::vector<std::string> &authors,
                 const std::string &title, const std::string &publisher,
                 const std::string &year, const std::string &pages,
                 const std::string &volume = "", const std::string &series = "",
                 const std::string &type = "", const std::string &address = "",
                 const std::string &edition = "", const std::string &month = "",
                 const std::string &doi = "",
                 const std::string &description = "",
                 const std::string &url = "");
  std::string toEndNote() const override;
  std::string toBibTex() const override;

  const std::vector<std::string> m_authors;
  const std::string m_title, m_publisher, m_year, m_pages, m_volume, m_series,
      m_type, m_address, m_edition, m_month;
};

// Pages assumes more than one would have a - seperator.
struct MANTID_API_DLL InCollectionCitation : BaseCitation {
  InCollectionCitation(
      const std::vector<std::string> &authors, const std::string &title,
      const std::string &booktitle, const std::string &publisher,
      const std::string &year, const std::string &volume = "",
      const std::string &series = "", const std::string &type = "",
      const std::string &chapter = "", const std::string &pages = "",
      const std::string &address = "", const std::string &edition = "",
      const std::string &month = "", const std::string &doi = "",
      const std::string &description = "", const std::string &url = "");
  std::string toEndNote() const override;
  std::string toBibTex() const override;

  // A function so that in python

  const std::vector<std::string> m_authors;
  const std::string m_title, m_booktitle, m_publisher, m_year, m_volume,
      m_series, m_type, m_chapter, m_pages, m_address, m_edition, m_month;
};

// Pages assumes more than one would have a - seperator.
struct MANTID_API_DLL InProceedingsCitation : BaseCitation {
  InProceedingsCitation(
      const std::vector<std::string> &authors, const std::string &title,
      const std::string &booktitle, const std::string &year,
      const std::string &editor = "", const std::string &volume = "",
      const std::string &series = "", const std::string &pages = "",
      const std::string &address = "", const std::string &month = "",
      const std::string &organization = "", const std::string &publisher = "",
      const std::string &doi = "", const std::string &description = "",
      const std::string &url = "");
  std::string toEndNote() const override;
  std::string toBibTex() const override;

  const std::vector<std::string> m_authors;
  const std::string m_title, m_booktitle, m_year, m_editor, m_volume, m_series,
      m_pages, m_address, m_month, m_organization, m_publisher;
};

struct MANTID_API_DLL ManualCitation : BaseCitation {
  ManualCitation(const std::string &title,
                 const std::vector<std::string> &authors = {},
                 const std::string &organization = "",
                 const std::string &address = "",
                 const std::string &edition = "", const std::string &month = "",
                 const std::string &year = "", const std::string &doi = "",
                 const std::string &description = "",
                 const std::string &url = "");
  std::string toEndNote() const override;
  std::string toBibTex() const override;

  const std::string m_title;
  const std::vector<std::string> m_authors;
  const std::string m_organization, m_address, m_edition, m_month, m_year;
};

struct MANTID_API_DLL MastersThesisCitation : BaseCitation {
  MastersThesisCitation(const std::vector<std::string> &authors,
                        const std::string &title, const std::string &school,
                        const std::string &year, const std::string &type = "",
                        const std::string &address = "",
                        const std::string &month = "",
                        const std::string &doi = "",
                        const std::string &description = "",
                        const std::string &url = "");
  std::string toEndNote() const override;
  std::string toBibTex() const override;

  const std::vector<std::string> m_authors;
  const std::string m_title, m_school, m_year, m_type, m_address, m_month;
};

struct MANTID_API_DLL MiscCitation : BaseCitation {
  MiscCitation(const std::vector<std::string> &authors = {},
               const std::string &title = "",
               const std::string &howpublished = "",
               const std::string &month = "", const std::string &year = "",
               const std::string &doi = "", const std::string &description = "",
               const std::string &url = "");
  std::string toEndNote() const override;
  std::string toBibTex() const override;

  const std::vector<std::string> m_authors;
  const std::string m_title, m_howpublished, m_month, m_year;
};

struct MANTID_API_DLL PHDThesisCitation : BaseCitation {
  PHDThesisCitation(const std::vector<std::string> &authors,
                    const std::string &title, const std::string &school,
                    const std::string &year, const std::string &type = "",
                    const std::string &address = "",
                    const std::string &month = "", const std::string &doi = "",
                    const std::string &description = "",
                    const std::string &url = "");
  std::string toEndNote() const override;
  std::string toBibTex() const override;

  const std::vector<std::string> m_authors;
  const std::string m_title, m_school, m_year, m_type, m_address, m_month;
};

struct MANTID_API_DLL ProceedingsCitation : BaseCitation {
  ProceedingsCitation(
      const std::string &title, const std::string &year,
      const std::string &editor = "", const std::string &volume = "",
      const std::string &series = "", const std::string &address = "",
      const std::string &month = "", const std::string &organization = "",
      const std::string &publisher = "", const std::string &doi = "",
      const std::string &description = "", const std::string &url = "");
  std::string toEndNote() const override;
  std::string toBibTex() const override;

  const std::string m_title, m_year, m_editor, m_volume, m_series, m_address,
      m_month, m_organization, m_publisher;
};

struct MANTID_API_DLL TechReportCitation : BaseCitation {
  TechReportCitation(const std::vector<std::string> &authors,
                     const std::string &title, const std::string &instituion,
                     const std::string &year, const std::string &type = "",
                     const std::string &number = "",
                     const std::string &address = "",
                     const std::string &month = "", const std::string &doi = "",
                     const std::string &description = "",
                     const std::string &url = "");
  std::string toEndNote() const override;
  std::string toBibTex() const override;

  const std::vector<std::string> m_authors;
  const std::string m_title, m_institution, m_year, m_type, m_number, m_address,
      m_month;
};

struct MANTID_API_DLL UnPublishedCitation : BaseCitation {
  UnPublishedCitation(const std::vector<std::string> &authors,
                      const std::string &title, const std::string &description,
                      const std::string &month = "",
                      const std::string &year = "", const std::string &doi = "",
                      const std::string &url = "");
  std::string toEndNote() const override;
  std::string toBibTex() const override;

  const std::vector<std::string> m_authors;
  const std::string m_title, m_month, m_year;
};

} // namespace API
} // namespace Mantid
#endif /* MANTID_API_CITATION_CITATION_H_ */