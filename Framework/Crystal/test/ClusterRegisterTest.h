// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidCrystal/Cluster.h"
#include "MantidCrystal/ClusterRegister.h"
#include "MantidCrystal/CompositeCluster.h"
#include "MockObjects.h"
#include <cxxtest/TestSuite.h>
#include <memory>

using namespace Mantid::Crystal;
using namespace testing;

class ClusterRegisterTest : public CxxTest::TestSuite {

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ClusterRegisterTest *createSuite() { return new ClusterRegisterTest(); }
  static void destroySuite(ClusterRegisterTest *suite) { delete suite; }

  void test_addClusters() {
    ClusterRegister cRegister;
    cRegister.add(1, std::make_shared<MockICluster>());
    cRegister.add(2, std::make_shared<MockICluster>());

    auto clusters = cRegister.clusters();
    TS_ASSERT_EQUALS(2, clusters.size());
  }

  void test_try_addClusters_with_duplicate_keys() {
    ClusterRegister cRegister;
    cRegister.add(1, std::make_shared<MockICluster>());
    cRegister.add(1, std::make_shared<MockICluster>());

    auto clusters = cRegister.clusters();
    TS_ASSERT_EQUALS(1, clusters.size());
  }

  void test_simple_merge() {
    ClusterRegister cRegister;
    auto a = std::make_shared<Cluster>(1);
    auto b = std::make_shared<Cluster>(2);
    auto c = std::make_shared<Cluster>(3);
    a->addIndex(0);
    b->addIndex(0);
    c->addIndex(0);
    cRegister.add(1, a);
    cRegister.add(2, b);
    cRegister.add(3, c);
    cRegister.merge(DisjointElement(2),
                    DisjointElement(3)); // Merge clusters 2 and 3

    auto combined = cRegister.clusters();
    TS_ASSERT_EQUALS(2, combined.size());
    TS_ASSERT(combined.find(1) != combined.end());
    TS_ASSERT(combined.find(2) != combined.end());
    TS_ASSERT(std::dynamic_pointer_cast<ICluster>(combined[1]));
    TS_ASSERT(std::dynamic_pointer_cast<CompositeCluster>(combined[2]));
  }

  void test_simple_merge_repeat() {
    ClusterRegister cRegister;
    auto a = std::make_shared<Cluster>(1);
    auto b = std::make_shared<Cluster>(2);
    auto c = std::make_shared<Cluster>(3);
    a->addIndex(0);
    b->addIndex(0);
    c->addIndex(0);
    cRegister.add(1, a);
    cRegister.add(2, b);
    cRegister.add(3, c);
    cRegister.merge(DisjointElement(2),
                    DisjointElement(3)); // Merge clusters 2 and 3
    cRegister.merge(DisjointElement(3),
                    DisjointElement(2)); // This is a duplicate call that should be ignored.

    auto combined = cRegister.clusters();
    TS_ASSERT_EQUALS(2, combined.size());
    TS_ASSERT(combined.find(1) != combined.end());
    TS_ASSERT(combined.find(2) != combined.end());
    TS_ASSERT(std::dynamic_pointer_cast<ICluster>(combined[1]));
    TS_ASSERT(std::dynamic_pointer_cast<CompositeCluster>(combined[2]));
  }

  void test_multi_merge() {
    ClusterRegister cRegister;
    auto a = std::make_shared<Cluster>(1);
    auto b = std::make_shared<Cluster>(2);
    auto c = std::make_shared<Cluster>(3);
    a->addIndex(0);
    b->addIndex(0);
    c->addIndex(0);
    cRegister.add(1, a);
    cRegister.add(2, b);
    cRegister.add(3, c);
    cRegister.merge(DisjointElement(2),
                    DisjointElement(3)); // Merge clusters 2 and 3
    cRegister.merge(DisjointElement(1),
                    DisjointElement(2)); // Merge clusters 1 and 2

    auto combined = cRegister.clusters();
    TS_ASSERT_EQUALS(1, combined.size());
    TSM_ASSERT("Combined all clusters, so should have a single Composite "
               "cluster. Composite should be labelled with the lowest label.",
               std::dynamic_pointer_cast<CompositeCluster>(combined[1]));
  }

  void test_complex_merge() {
    // Merge (1,2)  (3,4) then (2, 3), we should get one big cluster at the end.

    auto one = std::make_shared<Cluster>(1);
    auto two = std::make_shared<Cluster>(2);
    auto three = std::make_shared<Cluster>(3);
    auto four = std::make_shared<Cluster>(4);

    one->addIndex(0);
    two->addIndex(0);
    three->addIndex(0);
    four->addIndex(0);

    ClusterRegister cRegister;
    cRegister.add(1, one);
    cRegister.add(2, two);
    cRegister.add(3, three);
    cRegister.add(4, four);

    cRegister.merge(DisjointElement(1), DisjointElement(2));
    cRegister.merge(DisjointElement(3), DisjointElement(4));
    cRegister.merge(DisjointElement(2), DisjointElement(3));

    auto clusters = cRegister.clusters();

    TSM_ASSERT_EQUALS("One big cluster", clusters.size(), 1);
    TSM_ASSERT_EQUALS("All four Clusters registered under big composite.", clusters[1]->size(), 4);

    auto label = clusters[1]->getLabel();
    TSM_ASSERT_EQUALS("Entire clustere labeled as minimum (1)", label, 1);
  }
};
