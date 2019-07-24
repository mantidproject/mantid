// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

#ifndef CITATIONCONSTRUCTORHELPERSTEST_H_
#define CITATIONCONSTRUCTORHELPERSTEST_H_

#include "MantidAPI/CitationConstructorHelpers.h"

#include <cxxtest/TestSuite.h>

using namespace Mantid::API;

class CitationConstructorHelpersTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static CitationConstructorHelpersTest *createSuite() {
    return new CitationConstructorHelpersTest();
  }
  static void destroySuite(CitationConstructorHelpersTest *suite) {
    delete suite;
  }

  void test_article_citation_endnote() {
    auto citation =
        ArticleCitation({"author1", "author2"}, "title", "journal", "year", "",
                        "number", "pages", "month", "description", "", "url");
    TS_ASSERT_EQUALS(citation.toEndNote(),
                     "TY  - JOUR\nAU  - author1\nAU  - author2\nT1  - "
                     "title\nT2  - journal\nIS  - number\nSP  - pages\nEP  - "
                     "pages\nDA  - year/month\nN1  - description\nEP  - \n");
  }

  void test_article_citation_bibtex() {
    auto citation =
        ArticleCitation({"author1", "author2"}, "title", "journal", "year", "",
                        "number", "pages", "month", "description", "", "url");
    TS_ASSERT_EQUALS(
        citation.toBibTex(),
        "@article{ref,\nauthor={author1 and "
        "author2},\ntitle={title},\njournal={journal},\nyear={year},"
        "\nnumber={"
        "number},\npages={pages},\nmonth={month},\nnote={description}\n}");
  }

  void test_book_citation_endnote() {
    auto citation = BookCitation({"author1", "author2"}, "title", "publisher",
                                 "year", "", "series", "", "edition", "month",
                                 "description", "doi", "");
    TS_ASSERT_EQUALS(citation.toEndNote(),
                     "TY  - BOOK\nAU  - author1\nAU  - author2\nT1  - "
                     "title\nPB  - publisher\nC1  - series\nET  - edition\nDA  "
                     "- year/month\nN1  - description\nDO  - doi\nEP  - \n");
  }

  void test_book_citation_bibtex() {
    auto citation = BookCitation({"author1", "author2"}, "title", "publisher",
                                 "year", "", "series", "", "edition", "month",
                                 "description", "doi", "");
    TS_ASSERT_EQUALS(citation.toBibTex(),
                     "@book{ref,\nauthor={author1 and "
                     "author2},\ntitle={title},\npublisher={publisher},\nyear={"
                     "year},\nseries={series},\nedition={edition},\nmonth={"
                     "month},\nnote={description},\ndoi={doi}\n}");
  }

  void test_booklet_citation_endnote() {
    auto citation =
        BookletCitation("title", {}, "how published", "address", "month",
                        "year", "description", "doi", "url");
    TS_ASSERT_EQUALS(
        citation.toEndNote(),
        "TY  - PAMP\nT1  - title\nBT  - how published\nAD  - address\nDA  - "
        "year/month\nN1  - description\nDO  - doi\nEP  - \n");
  }

  void test_booklet_citation_bibtex() {
    auto citation =
        BookletCitation("title", {}, "how published", "address", "month",
                        "year", "description", "doi", "url");
    TS_ASSERT_EQUALS(citation.toBibTex(),
                     "@booklet{ref,\ntitle={title},\nhowpublished={how "
                     "published},\naddress={address},\nmonth={month},\nyear={"
                     "year},\nnote={description},\ndoi={doi}\n}");
  }

  void test_inbook_citation_endnote() {
    auto citation =
        InBookCitation({"author1", "author2"}, "title", "publisher", "year",
                       "11-12", "", "", "", "address", "edition", "month");
    TS_ASSERT_EQUALS(citation.toEndNote(),
                     "TY  - BOOK\nAU  - author1\nAU  - author2\nT1  - "
                     "title\nPB  - publisher\nSP  - 11\nEP  - 12\nAD  - "
                     "address\nET  - edition\nDA  - year/month\nEP  - \n")
  }

  void test_inbook_citation_bibtex() {
    auto citation =
        InBookCitation({"author1", "author2"}, "title", "publisher", "year",
                       "11-12", "", "", "", "address", "edition", "month");
    TS_ASSERT_EQUALS(citation.toBibTex(),
                     "@inbook{ref,\nauthor={author1 and "
                     "author2},\ntitle={title},\npublisher={publisher},\nyear={"
                     "year},\npages={11-12},\naddress={address},\nedition={"
                     "edition},\nmonth={month}\n}")
  }

  void test_incollection_citation_endnote() {
    auto citation = InCollectionCitation(
        {"author1", "Maximilliam the third of Southwestbrooke"}, "title",
        "booktitle", "publisher", "year", "", "", "type", "chapter", "pages",
        "address", "", "month", "doi", "description", "https://www.url.com");
    TS_ASSERT_EQUALS(
        citation.toEndNote(),
        "TY  - GEN\nAU  - author1\nAU  - Maximilliam the third of "
        "Southwestbrooke\nT1  - title\nT2  - booktitle\nPB  - publisher\nPY  - "
        "year\nM3  - type\nSE  - chapter\nSP  - pages\nEP  - pages\nAD  - "
        "address\nDA  - year/month\nN1  - description\nDO  - doi\nEP  - \n")
  }

  void test_incollection_citation_bibtex() {
    auto citation = InCollectionCitation(
        {"author1", "Maximilliam the third of Southwestbrooke"}, "title",
        "booktitle", "publisher", "year", "", "", "type", "chapter", "pages",
        "address", "", "month", "doi", "description", "https://www.url.com");
    TS_ASSERT_EQUALS(
        citation.toBibTex(),
        "@incollection{ref,\nauthor={author1 and Maximilliam the third of "
        "Southwestbrooke},\ntitle={title},\nbooktitle={booktitle},\npublisher={"
        "publisher},\nyear={year},\ntype={type},\nchapter={chapter},\npages={"
        "pages},\naddress={address},\nmonth={month},\nnote={description},\ndoi="
        "{doi}\n}")
  }

  void test_inproceedings_citation_endnote() {
    auto citation = InProceedingsCitation(
        {"Borris", "Phil"}, "title", "booktitle", "year", "editor", "", "",
        "pages", "", "", "", "publisher", "doi", "big proceeding");
    TS_ASSERT_EQUALS(
        citation.toEndNote(),
        "TY  - CONF\nAU  - Borris\nAU  - Phil\nT1  - title\nT2  - "
        "booktitle\nPY  - year\nED  - editor\nSP  - pages\nEP  - pages\nPY  - "
        "year\nPB  - publisher\nN1  - big proceeding\nDO  - doi\nEP  - \n")
  }

  void test_inproceedings_citation_bibtex() {
    auto citation = InProceedingsCitation(
        {"Borris", "Phil"}, "title", "booktitle", "year", "editor", "", "",
        "pages", "", "", "", "publisher", "doi", "big proceeding");
    TS_ASSERT_EQUALS(citation.toBibTex(),
                     "@inproceedings{ref,\nauthor={Borris and "
                     "Phil},\ntitle={title},\nbooktitle={booktitle},\nyear={"
                     "year},\neditor={editor},\npages={pages},\npublisher={"
                     "publisher},\nnote={big proceeding},\ndoi={doi}\n}")
  }

  void test_manual_citation_endnote() {
    auto citation =
        ManualCitation("title", {}, "organisation", "", "edition", "month",
                       "year", "doi", "description", "URL");
    TS_ASSERT_EQUALS(
        citation.toEndNote(),
        "TY  - GEN\nT1  - title\nDA  - year/month\nPP  - organisation\nET  - "
        "edition\nN1  - description\nDO  - doi\nEP  - \n")
  }

  void test_manual_citation_bibtex() {
    auto citation =
        ManualCitation("title", {}, "organisation", "", "edition", "month",
                       "year", "doi", "description", "URL");
    TS_ASSERT_EQUALS(citation.toBibTex(),
                     "@manual{ref,\ntitle={title},\nyear={year},\nmonth={month}"
                     ",\norganization={organisation},\nedition={edition},"
                     "\nnote={description},\ndoi={doi}\n}")
  }

  void test_masters_thesis_citation_endnote() {
    auto citation =
        MastersThesisCitation({"author"}, "title", "school", "year", "type",
                              "address", "month", "doi", "description", "url");
    TS_ASSERT_EQUALS(
        citation.toEndNote(),
        "TY  - THES\nAU  - author\nT1  - title\nPP  - school\nM3  - type\nAD  "
        "- address\nDA  - year/month\nN1  - description\nDO  - doi\nEP  - \n")
  }

  void test_masters_thesis_citation_bibtex() {
    auto citation =
        MastersThesisCitation({"author"}, "title", "school", "year", "type",
                              "address", "month", "doi", "description", "url");
    TS_ASSERT_EQUALS(
        citation.toBibTex(),
        "@mastersthesis{ref,\nauthor={author},\ntitle={title},\nschool={school}"
        ",\ntype={type},\nyear={year},\naddress={address},\nmonth={month},"
        "\nnote={description},\ndoi={doi}\n}")
  }

  void test_misc_citation_endnote() {
    auto citation = MiscCitation({}, "title", "published this way", "month", "",
                                 "", "", "");
    TS_ASSERT_EQUALS(
        citation.toEndNote(),
        "TY  - GEN\nT1  - title\nBT  - published this way\nEP  - \n")
  }

  void test_misc_citation_bibtex() {
    auto citation = MiscCitation({}, "title", "published this way", "month", "",
                                 "", "", "");
    TS_ASSERT_EQUALS(citation.toBibTex(),
                     "@misc{ref,\ntitle={title},\nhowpublished={published this "
                     "way},\nmonth={month}\n}")
  }

  void test_PHD_thesis_citation_endnote() {
    auto citation =
        PHDThesisCitation({"author"}, "title", "school", "year", "type",
                          "address", "month", "doi", "description", "url");
    TS_ASSERT_EQUALS(
        citation.toEndNote(),
        "TY  - THES\nAU  - author\nT1  - title\nPP  - school\nM3  - type\nAD  "
        "- address\nDA  - year/month\nN1  - description\nDO  - doi\nEP  - \n")
  }

  void test_PHD_thesis_citation_bibtex() {
    auto citation =
        PHDThesisCitation({"author"}, "title", "school", "year", "type",
                          "address", "month", "doi", "description", "url");
    TS_ASSERT_EQUALS(
        citation.toBibTex(),
        "@phdthesis{ref,\nauthor={author},\ntitle={title},\nschool={school},"
        "\ntype={type},\nyear={year},\naddress={address},\nmonth={month},"
        "\nnote={description},\ndoi={doi}\n}")
  }

  void test_proceedings_citation_endnote() {
    auto citation = ProceedingsCitation(
        "title", "year", "editor", "volume", "series", "address", "month",
        "organization", "publisher", "doi", "description", "url");
    TS_ASSERT_EQUALS(
        citation.toEndNote(),
        "TY  - CONF\nT1  - title\nED  - editor\nVL  - volume\nC1  - series\nAD "
        " - address\nPP  - organization\nPB  - publisher\nDA  - year/month\nN1 "
        " - description\nDO  - doi\nEP  - \n")
  }

  void test_proceedings_citation_bibtex() {
    auto citation = ProceedingsCitation(
        "title", "year", "editor", "volume", "series", "address", "month",
        "organization", "publisher", "doi", "description", "url");
    TS_ASSERT_EQUALS(
        citation.toBibTex(),
        "@proceedings{ref,\ntitle={title},\nyear={year},\neditor={editor},"
        "\nvolume={volume},\nseries={series},\naddress={address},"
        "\norganization={organization},\npublisher={publisher},\nmonth={month},"
        "\nnote={description},\ndoi={doi}\n}")
  }

  void test_tech_report_citation_endnote() {
    auto citation =
        TechReportCitation({"author1"}, "title", "institution", "year", "type",
                           "", "", "", "", "description", "url");
    TS_ASSERT_EQUALS(
        citation.toEndNote(),
        "TY  - RPRT\nAU  - author1\nT1  - title\nPP  - institution\nM3  - "
        "type\nPY  - year\nN1  - description\nEP  - \n")
  }

  void test_tech_report_citation_bibtex() {
    auto citation =
        TechReportCitation({"author1"}, "title", "institution", "year", "type",
                           "", "", "", "", "description", "url");
    TS_ASSERT_EQUALS(
        citation.toBibTex(),
        "@techreport{ref,\nauthor={author1},\ntitle={title},\ninstitution={"
        "institution},\nyear={year},\ntype={type},\nnote={description}\n}")
  }

  void test_unpublished_citation_endnote() {
    auto citation = UnPublishedCitation({"author1"}, "title", "description",
                                        "month", "", "doi", "url");
    TS_ASSERT_EQUALS(citation.toEndNote(), "TY  - UNPB\nAU  - author1\nT1  -"
                                           " title\nN1  - description\nDO  -"
                                           " doi\nEP  - \n");
  }

  void test_unpublished_citation_bibtex() {
    auto citation = UnPublishedCitation({"author1"}, "title", "description",
                                        "month", "", "doi", "url");
    TS_ASSERT_EQUALS(citation.toBibTex(),
                     "@unpublished{ref,\nauthor={author1},\ntitle={title},"
                     "\nnote={description},\nmonth={month},\ndoi={doi}\n}");
  }
};

#endif /* CITATIONCONSTRUCTORHELPERSTEST_H_ */