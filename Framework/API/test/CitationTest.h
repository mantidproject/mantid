// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

#ifndef CITATIONTEST_H_
#define CITATIONEST_H_

#include "MantidAPI/Citation.h"
#include "MantidAPI/CitationConstructorHelpers.h"
#include <cxxtest/TestSuite.h>

class CitationTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static CitationTest *createSuite() { return new CitationTest(); }
  static void destroySuite(CitationTest *suite) { delete suite; }

  void test_citation_constructor() {
    TS_ASSERT_THROWS_NOTHING(Mantid::API::Citation("doi", "bibtex", "endnote",
                                                   "url", "description"));
  }

  void test_citation_constructor_accepts_none_for_some_variables() {
    TS_ASSERT_THROWS_NOTHING(
        Mantid::API::Citation("doi", "bibtex", "endnote", "url", ""));
  }

  void test_citation_constructor_throws_when_bibtex_given_but_endnote_isnt() {
    TS_ASSERT_THROWS(Mantid::API::Citation("", "bibtex"),
                     const std::invalid_argument &)
  }

  void test_citation_constructor_throws_when_endnote_given_but_bibtex_isnt() {
    TS_ASSERT_THROWS(Mantid::API::Citation("", "", "endnote"),
                     const std::invalid_argument &);
  }

  void test_citation_constructor_throws_when_doi_is_given_but_endnote_isnt() {
    TS_ASSERT_THROWS(Mantid::API::Citation("doi", "bibtex", "", "url"),
                     const std::invalid_argument &);
  }

  void test_citation_constructor_throws_when_doi_is_given_but_bibtex_isnt() {
    TS_ASSERT_THROWS(Mantid::API::Citation("doi", "", "endnote", "url"),
                     const std::invalid_argument &);
  }

  void
  test_citation_constructor_throws_when_doi_is_given_but_endnote_and_bibtex_isnt() {
    TS_ASSERT_THROWS(Mantid::API::Citation("doi", "", "", "url"),
                     const std::invalid_argument &);
  }

  void test_citation_constructor_throws_when_doi_is_given_but_url_isnt() {
    TS_ASSERT_THROWS(Mantid::API::Citation("doi", "bibtex", "endnote", ""),
                     const std::invalid_argument &);
  }

  void
  test_citation_constructor_throws_when_url_is_not_given_when_bibtex_endnote_and_doi_is_not_given() {
    TS_ASSERT_THROWS(Mantid::API::Citation("", "", "", "", ""),
                     const std::invalid_argument &);
  }

  void
  test_citation_constructor_doesnt_throw_when_url_is_given_when_bibtex_endnote_and_doi_is_not_given() {
    TS_ASSERT_THROWS_NOTHING(Mantid::API::Citation("", "", "", "url"));
  }

  void test_citation_equivelancy_operator_is_true_on_equal() {
    auto cite1 =
        Mantid::API::Citation("doi", "bibtex", "endnote", "url", "description");
    auto cite2 =
        Mantid::API::Citation("doi", "bibtex", "endnote", "url", "description");
    TS_ASSERT(cite1 == cite2);
  }

  void test_citation_equivelancy_operator_is_false_on_not_equal() {
    auto cite1 =
        Mantid::API::Citation("doi", "bibtex", "endnote", "url", "description");
    auto cite2 = Mantid::API::Citation("doi", "bibtex", "endnote", "url",
                                       "not description");
    TS_ASSERT(!(cite1 == cite2));
  }

  void test_getCitation_with_a_citation_struct() {
    auto cite = Mantid::API::getCitation(Mantid::API::ArticleCitation(
        {"author1", "author2"}, "title", "journal", "year", "", "number",
        "pages", "month", "description", "", "url"));
    TS_ASSERT_EQUALS(cite.doi(), "");
    TS_ASSERT_EQUALS(cite.description(), "description")
    TS_ASSERT_EQUALS(cite.url(), "url")
    TS_ASSERT_EQUALS(cite.endnote(),
                     "TY  - JOUR\nAU  - author1\nAU  - author2\nT1  - "
                     "title\nT2  - journal\nIS  - number\nSP  - pages\nEP  - "
                     "pages\nDA  - year/month\nN1  - description\nEP  - \n")
    TS_ASSERT_EQUALS(
        cite.bibtex(),
        "@article{ref,\nauthor={author1 and "
        "author2},\ntitle={title},\njournal={journal},\nyear={year},"
        "\nnumber={"
        "number},\npages={pages},\nmonth={month},\nnote={description}\n}")
  }
};

#endif