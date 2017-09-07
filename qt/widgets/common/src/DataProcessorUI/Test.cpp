#include "MantidKernel/make_unique.h"
#include <iostream>
#include <memory>
#include <vector>

class Abstract {
public:
  Abstract() : m_things() {}
  virtual ~Abstract() {}
  virtual int pvc() const = 0;
  virtual int pv() = 0;
  virtual int v() { return 10; }
  std::vector<std::unique_ptr<Abstract>> m_things;
};

class Implementor : public Abstract {
public:
  int pvc() const override { return 12; }
  int pv() override { return 11; }
};

std::unique_ptr<Abstract> make() {
  return Mantid::Kernel::make_unique<Implementor>();
}

int my_function() {
  std::vector<std::unique_ptr<Abstract>> things;
  things.push_back(Mantid::Kernel::make_unique<Implementor>());
  things.push_back(make());
  for (auto& thing : things) {
    std::cout << "pvc: " << thing->pvc() << '\n';
    std::cout << "pv: " << thing->pv() << '\n';
    std::cout << "v: " << thing->v() << '\n';
  }
  std::cout << std::endl;
  return things[0]->pvc();
}
