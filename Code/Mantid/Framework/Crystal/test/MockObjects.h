/*
 * MockObjects.h
 *
 *  Created on: Apr 15, 2014
 *      Author: spu92482
 */

#ifndef MOCKOBJECTS_H_
#define MOCKOBJECTS_H_

#include <gmock/gmock.h>
#include "MantidCrystal/ICluster.h"

namespace Mantid
{
  namespace Crystal
  {

  class MockICluster: public ICluster
  {
  public:
    MOCK_CONST_METHOD1(integrate,
        ClusterIntegratedValues(boost::shared_ptr<const Mantid::API::IMDHistoWorkspace> ws));
    MOCK_CONST_METHOD1(writeTo,
        void(boost::shared_ptr<Mantid::API::IMDHistoWorkspace> ws));
    MOCK_CONST_METHOD0(getLabel,
        size_t());
    MOCK_CONST_METHOD0(getOriginalLabel,
        size_t());
    MOCK_CONST_METHOD0(size,
        size_t());
    MOCK_METHOD1(addIndex,
        void(const size_t& index));
    MOCK_METHOD1(toUniformMinimum,
        void(std::vector<DisjointElement>& disjointSet));
    MOCK_METHOD1(setRootCluster, void(ICluster const*));
    MOCK_CONST_METHOD0(getRepresentitiveIndex, size_t());
    virtual bool containsLabel(const size_t& label) const
    {
      return this->getLabel() == label;
    }
  };
  }
}


#endif /* MOCKOBJECTS_H_ */
