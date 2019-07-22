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
        ArticleCitation({"author1, author2"}, "title", "journal", "year", "",
                        "number", "pages", "month", "description", "", "url");
    TS_ASSERT_EQUALS(citation.toEndNote(), "");
  }

  void test_article_citation_bibtex() {
    auto citation =
        ArticleCitation({"author1, author2"}, "title", "journal", "year", "",
                        "number", "pages", "month", "description", "", "url");
    TS_ASSERT_EQUALS(citation.toBibTex(), "");
  }

  void test_book_citation_endnote() {
    auto citation = BookCitation({"author1", "author2"}, "title", "publisher",
                                 "year", "", "series", "", "edition", "month",
                                 "description", "doi", "");
    TS_ASSERT_EQUALS(citation.toEndNote(), "");
  }

  void test_book_citation_bibtex() {
    auto citation = BookCitation({"author1", "author2"}, "title", "publisher",
                                 "year", "", "series", "", "edition", "month",
                                 "description", "doi", "");
    TS_ASSERT_EQUALS(citation.toBibTex(), "");
  }

  void test_booklet_citation_endnote() {
    auto citation =
        BookletCitation("title", {}, "how published", "address", "month",
                        "year", "description", "doi", "url");
    TS_ASSERT_EQUALS(citation.toEndNote(), "");
  }

  void test_booklet_citation_bibtex() {
    auto citation =
        BookletCitation("title", {}, "how published", "address", "month",
                        "year", "description", "doi", "url");
    TS_ASSERT_EQUALS(citation.toBibTex(), "");
  }

  void test_inbook_citation_endnote() {}

  void test_inbook_citation_bibtex() {}

  void test_incollection_citation_endnote() {}

  void test_incollection_citation_bibtex() {}

  void test_inproceedings_citation_endnote() {}

  void test_inproceedings_citation_bibtex() {}

  void test_manual_citation_endnote() {}

  void test_manual_citation_bibtex() {}

  void test_masters_thesis_citation_endnote() {}

  void test_masters_thesis_citation_bibtex() {}

  void test_misc_citation_endnote() {}

  void test_misc_citation_bibtex() {}

  void test_PHD_thesis_citation_endnote() {}

  void test_PHD_thesis_citation_bibtex() {}

  void test_proceedings_citation_endnote() {}

  void test_proceedings_citation_bibtex() {}

  void test_tech_report_citation_endnote() {}

  void test_tech_report_citation_bibtex() {}

  void test_unpublished_citation_endnote() {}

  void test_unpublished_citation_bibtex() {}
};

#endif /* CITATIONCONSTRUCTORHELPERSTEST_H_ */