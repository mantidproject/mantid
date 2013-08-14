#ifndef FAKEALGORITHMS_H_
#define FAKEALGORITHMS_H_

#include "MantidAPI/Algorithm.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/RebinParamsValidator.h"

using namespace Mantid::Kernel; 
using namespace Mantid::API;

class ToyAlgorithm : public Algorithm
{
public:
  ToyAlgorithm() : Algorithm() {}
  virtual ~ToyAlgorithm() {}
  const std::string name() const { return "ToyAlgorithm";} ///< Algorithm's name for identification
  int version() const  { return 1;} ///< Algorithm's version for identification
  const std::string category() const { return "Cat";} ///< Algorithm's category for identification
  const std::string alias() const { return "Dog";}

  void init()
  { 
    declareProperty("prop1","value");
    declareProperty("prop2",1);
  }
  void exec() {}
  
  bool existsProperty( const std::string &name ) const
  {
    return PropertyManagerOwner::existsProperty(name);
  }
  const std::vector< Property* >& getProperties() const
  {
    return PropertyManagerOwner::getProperties();
  }
};

class ToyAlgorithmTwo : public Algorithm
{
public:
  ToyAlgorithmTwo() : Algorithm() {}
  virtual ~ToyAlgorithmTwo() {}

  const std::string name() const { return "ToyAlgorithm";} ///< Algorithm's name for identification
  int version() const  { return 2;} ///< Algorithm's version for identification
  const std::string category() const { return "Cat,Leopard,Mink";} 
  const std::string categorySeparator() const { return ",";} ///< testing the ability to change the seperator
  const std::string alias() const { return "Dog";}
  void init()
  { 
    declareProperty("prop1","value");
    declareProperty("prop2",1);   
    declareProperty("prop3",10.5);   
    std::vector<double> binning;
    binning.push_back(1.0);
    binning.push_back(0.1);
    binning.push_back(2.0);
    declareProperty(new ArrayProperty<double>("Binning", binning,
        boost::make_shared<RebinParamsValidator>()));
  }
  void exec() {}
};

class ToyAlgorithmThree : public Algorithm
{
public:
  ToyAlgorithmThree() : Algorithm() {}
  virtual ~ToyAlgorithmThree() {}

  const std::string name() const { return "ToyAlgorithm";} ///< Algorithm's name for identification
  int version() const  { return 2;} ///< Algorithm's version for identification
  const std::string category() const { return "Cat;Leopard;Mink";} 
  const std::string alias() const { return "Dog";}
  void init()
  { 
    declareProperty("prop1","value");
    declareProperty("prop2",1);   
    declareProperty("prop3",10.5);   
  }
  void exec() {}
};

class CategoryAlgorithm : public Algorithm
{
public:
  CategoryAlgorithm() : Algorithm() {}
  virtual ~CategoryAlgorithm() {}

  const std::string name() const { return "CategoryAlgorithm";} ///< Algorithm's name for identification
  int version() const  { return 1;} ///< Algorithm's version for identification
  const std::string category() const { return "Fake";} 
  const std::string alias() const { return "CategoryTester";}
  void init()
  { 
    declareProperty("prop1","value");
    declareProperty("prop2",1);   
    declareProperty("prop3",10.5);   
  }
  void exec() {}
};
#endif