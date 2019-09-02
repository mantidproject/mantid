// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef ALGORITHMFACTORYOBSERVERTEST_H_
#define ALGORITHMFACTORYOBSERVERTEST_H_

#include <cxxtest/TestSuite.h>

#include "FakeAlgorithms.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/AlgorithmFactoryObserver.h"
#include "MantidKernel/Instantiator.h"

using namespace Mantid::API;

class FakeAlgorithmFactoryObserver
    : public Mantid::API::AlgorithmFactoryObserver {

public:
  FakeAlgorithmFactoryObserver() : m_updateHandleCalled(false) {}

  ~FakeAlgorithmFactoryObserver() { this->observeUpdate(false); }

  void updateHandle() override { m_updateHandleCalled = true; }

public:
  bool m_updateHandleCalled;
};

class AlgorithmFactoryObserverTest : public CxxTest::TestSuite {
private:
  AlgorithmFactoryImpl &af;
  std::unique_ptr<FakeAlgorithmFactoryObserver> m_mockInheritingClass;

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static AlgorithmFactoryObserverTest *createSuite() {
    return new AlgorithmFactoryObserverTest();
  }
  static void destroySuite(AlgorithmFactoryObserverTest *suite) {
    delete suite;
  }

  AlgorithmFactoryObserverTest()
      : af(AlgorithmFactory::Instance()),
        m_mockInheritingClass(
            std::make_unique<FakeAlgorithmFactoryObserver>()) {}

  void setUp() override {
    af.unsubscribe("ToyAlgorithm", 1);
    m_mockInheritingClass = std::make_unique<FakeAlgorithmFactoryObserver>();
  }

  void test_updateHandle_is_not_called_on_update_by_default() {
    // Notifications turned off in AlgorithmFactory by default
    m_mockInheritingClass->observeUpdate();

    std::unique_ptr<Mantid::Kernel::AbstractInstantiator<Algorithm>> newAlg =
        std::make_unique<
            Mantid::Kernel::Instantiator<ToyAlgorithm, Algorithm>>();
    af.subscribe(std::move(newAlg));

    TS_ASSERT(!m_mockInheritingClass->m_updateHandleCalled)
  }

  void test_update_handle_is_called_on_update() {
    m_mockInheritingClass->observeUpdate();

    af.enableNotifications();

    std::unique_ptr<Mantid::Kernel::AbstractInstantiator<Algorithm>> newAlg =
        std::make_unique<
            Mantid::Kernel::Instantiator<ToyAlgorithm, Algorithm>>();
    af.subscribe(std::move(newAlg));

    TS_ASSERT(m_mockInheritingClass->m_updateHandleCalled)
  }
};

#endif /* ALGORITHMFACTORYOBSERVERTEST_H_ */