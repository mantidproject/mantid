// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

#ifndef CITATIONTEST_H_
#define CITATIONEST_H_

#include "MantidAPI/Citation.h"
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
        Mantid::API::Citation("doi", "bibtex", "endnote", "url", boost::none));
  }

  void test_citation_constructor_throws_when_bibtex_given_but_endnote_isnt() {
    TS_ASSERT_THROWS(Mantid::API::Citation(boost::none, "bibtex"),
                     const std::invalid_argument &)
  }

  void test_citation_constructor_throws_when_endnote_given_but_bibtex_isnt() {
    TS_ASSERT_THROWS(Mantid::API::Citation(boost::none, boost::none, "endnote"),
                     const std::invalid_argument &);
  }

  void test_citation_constructor_throws_when_doi_is_given_but_endnote_isnt() {
    TS_ASSERT_THROWS(Mantid::API::Citation("doi", "bibtex", boost::none, "url"),
                     const std::invalid_argument &);
  }

  void test_citation_constructor_throws_when_doi_is_given_but_bibtex_isnt() {
    TS_ASSERT_THROWS(
        Mantid::API::Citation("doi", boost::none, "endnote", "url"),
        const std::invalid_argument &);
  }

  void
  test_citation_constructor_throws_when_doi_is_given_but_endnote_and_bibtex_isnt() {
    TS_ASSERT_THROWS(
        Mantid::API::Citation("doi", boost::none, boost::none, "url"),
        const std::invalid_argument &);
  }

  void test_citation_constructor_throws_when_doi_is_given_but_url_isnt() {
    TS_ASSERT_THROWS(
        Mantid::API::Citation("doi", "bibtex", "endnote", boost::none),
        const std::invalid_argument &);
  }

  void
  test_citation_constructor_throws_when_url_is_not_given_when_bibtex_endnote_and_doi_is_not_given() {
    TS_ASSERT_THROWS(Mantid::API::Citation(boost::none, boost::none,
                                           boost::none, boost::none, ""),
                     const std::invalid_argument &);
  }

  void
  test_citation_constructor_doesnt_throw_when_url_is_given_when_bibtex_endnote_and_doi_is_not_given() {
    TS_ASSERT_THROWS_NOTHING(
        Mantid::API::Citation(boost::none, boost::none, boost::none, "url"));
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
};

#endif