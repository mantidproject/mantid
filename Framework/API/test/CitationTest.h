// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Citation.h"
#include "MantidFrameworkTestHelpers/NexusTestHelper.h"
#include "MantidNexus/NeXusFile.hpp"

#include <cxxtest/TestSuite.h>

class CitationTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static CitationTest *createSuite() { return new CitationTest(); }
  static void destroySuite(CitationTest *suite) { delete suite; }

  void test_citation_constructor() {
    TS_ASSERT_THROWS_NOTHING(Mantid::API::Citation("doi", "bibtex", "endnote", "url", "description"));
  }

  void test_citation_constructor_accepts_none_for_some_variables() {
    TS_ASSERT_THROWS_NOTHING(Mantid::API::Citation("doi", "bibtex", "endnote", "url", ""));
  }

  void test_citation_constructor_throws_when_bibtex_given_but_endnote_isnt() {
    TS_ASSERT_THROWS(Mantid::API::Citation("", "bibtex"), const std::invalid_argument &)
  }

  void test_citation_constructor_throws_when_endnote_given_but_bibtex_isnt() {
    TS_ASSERT_THROWS(Mantid::API::Citation("", "", "endnote"), const std::invalid_argument &);
  }

  void test_citation_constructor_throws_when_doi_is_given_but_endnote_isnt() {
    TS_ASSERT_THROWS(Mantid::API::Citation("doi", "bibtex", "", "url"), const std::invalid_argument &);
  }

  void test_citation_constructor_throws_when_doi_is_given_but_bibtex_isnt() {
    TS_ASSERT_THROWS(Mantid::API::Citation("doi", "", "endnote", "url"), const std::invalid_argument &);
  }

  void test_citation_constructor_throws_when_doi_is_given_but_endnote_and_bibtex_isnt() {
    TS_ASSERT_THROWS(Mantid::API::Citation("doi", "", "", "url"), const std::invalid_argument &);
  }

  void test_citation_constructor_throws_when_doi_is_given_but_url_isnt() {
    TS_ASSERT_THROWS(Mantid::API::Citation("doi", "bibtex", "endnote", ""), const std::invalid_argument &);
  }

  void test_citation_constructor_throws_when_url_is_not_given_when_bibtex_endnote_and_doi_is_not_given() {
    TS_ASSERT_THROWS(Mantid::API::Citation("", "", "", "", ""), const std::invalid_argument &);
  }

  void test_citation_constructor_doesnt_throw_when_url_is_given_when_bibtex_endnote_and_doi_is_not_given() {
    TS_ASSERT_THROWS_NOTHING(Mantid::API::Citation("", "", "", "url"));
  }

  void test_citation_equivelancy_operator_is_true_on_equal() {
    auto cite1 = Mantid::API::Citation("doi", "bibtex", "endnote", "url", "description");
    auto cite2 = Mantid::API::Citation("doi", "bibtex", "endnote", "url", "description");
    TS_ASSERT(cite1 == cite2);
  }

  void test_citation_equivelancy_operator_is_false_on_not_equal() {
    auto cite1 = Mantid::API::Citation("doi", "bibtex", "endnote", "url", "description");
    auto cite2 = Mantid::API::Citation("doi", "bibtex", "endnote", "url", "not description");
    TS_ASSERT(!(cite1 == cite2));
  }

  // Make sure this will cleanup
  void test_save_nexus_doesnt_throw() {
    const std::string filename = "saveNexusCitation1.nxs";
    NexusTestHelper th(true);
    th.createFile(filename);
    auto cite = Mantid::API::Citation("doi", "bibtex", "endnote", "url", "description");

    TS_ASSERT_THROWS_NOTHING(cite.saveNexus(th.file.get(), "group"))
  }

  // Make sure this will cleanup
  void test_save_and_load_nexus() {
    const std::string filename = "loadNexusCitation1.nxs";
    const std::string group = "group";
    NexusTestHelper th(true);
    th.createFile(filename);
    auto cite1 = Mantid::API::Citation("doi", "bibtex", "endnote", "url", "description");

    cite1.saveNexus(th.file.get(), group);

    Mantid::API::Citation cite2(th.file.get(), group);
    TS_ASSERT_EQUALS(cite2.doi(), "doi");
    TS_ASSERT_EQUALS(cite2.description(), "description")
    TS_ASSERT_EQUALS(cite2.url(), "url")
    TS_ASSERT_EQUALS(cite2.endnote(), "endnote")
    TS_ASSERT_EQUALS(cite2.bibtex(), "bibtex")
  }
};
