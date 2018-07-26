/*
 * MockObjects.h
 *
 *  Created on: Apr 15, 2014
 *      Author: spu92482
 */

#ifndef MOCKOBJECTS_H_
#define MOCKOBJECTS_H_

#include "MantidCrystal/BackgroundStrategy.h"
#include "MantidCrystal/ICluster.h"
#include "MantidKernel/WarningSuppressions.h"
#include <gmock/gmock.h>
#include <stdexcept>

namespace Mantid {
namespace Crystal {

GNU_DIAG_OFF_SUGGEST_OVERRIDE

// Mock Background strategy
class MockBackgroundStrategy : public BackgroundStrategy {
public:
  MOCK_CONST_METHOD1(configureIterator, void(Mantid::API::IMDIterator *const));
  MOCK_CONST_METHOD1(isBackground, bool(Mantid::API::IMDIterator *const));
  MockBackgroundStrategy *clone() const override {
    throw std::runtime_error("Cannot clone the mock object");
  }
  ~MockBackgroundStrategy() override {}
};

class MockICluster : public ICluster {
public:
  MOCK_CONST_METHOD1(integrate, ClusterIntegratedValues(boost::shared_ptr<
                                    const Mantid::API::IMDHistoWorkspace> ws));
  MOCK_CONST_METHOD1(
      writeTo, void(boost::shared_ptr<Mantid::API::IMDHistoWorkspace> ws));
  MOCK_CONST_METHOD0(getLabel, size_t());
  MOCK_CONST_METHOD0(getOriginalLabel, size_t());
  MOCK_CONST_METHOD0(size, size_t());
  MOCK_METHOD1(addIndex, void(const size_t &index));
  MOCK_METHOD1(toUniformMinimum,
               void(std::vector<DisjointElement> &disjointSet));
  MOCK_METHOD1(setRootCluster, void(ICluster const *));
  MOCK_CONST_METHOD0(getRepresentitiveIndex, size_t());
  bool containsLabel(const size_t &label) const override {
    return this->getLabel() == label;
  }
};

GNU_DIAG_ON_SUGGEST_OVERRIDE
}
}

#endif /* MOCKOBJECTS_H_ */
