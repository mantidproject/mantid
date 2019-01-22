// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_MDALGORITHMS_CONVTOMDEVENTSWSINDEXINGTEST_H_
#define MANTID_MDALGORITHMS_CONVTOMDEVENTSWSINDEXINGTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidMDAlgorithms/ConvToMDEventsWSIndexing.h"
#include <stdexcept>
#include <ostream>

/**
 * Class that stores the fixed 3d tree structure with
 * split parameter = 2 for every dim and the depth = 3:
 * level    box indexes range
 * 0        [0, 0]
 * 1        [1, 8]
 * 2        [9, 72]
 * 3        [73, 584]
 *
 *To calculate the <index> [0, 8] of ith child of the Box with <id>:
 * index = 8*id + 1 + index
 */
class FullTree3D3L {
  static constexpr size_t nodesCount{585};
  static constexpr uint8_t level{3};
public:
  struct Box {
    std::array<double, 3> lowerLeft;
    std::array<double, 3> upperRight;
    std::array<double, 3> center() const {
      return std::array<double, 3>{
             (lowerLeft[0] + upperRight[0])/2,
             (lowerLeft[1] + upperRight[1])/2,
             (lowerLeft[2] + upperRight[2])/2};
    }

    Box() {}
    Box(const std::array<double, 3>& ll, const std::array<double, 3>& ur) :
    lowerLeft(ll), upperRight(ur) {}

    template<typename T>
    bool contains(const std::array<T, 3>& pt) const {
      return (lowerLeft[0] <= pt[0] && pt[0] <= upperRight[0])
          && (lowerLeft[1] <= pt[1] && pt[1] <= upperRight[1])
          && (lowerLeft[2] <= pt[2] && pt[2] <= upperRight[2]);
    }
    friend std::ostream &operator<<(std::ostream &os, const Box &box) {
      os << "lowerLeft: ";
      for(uint8_t i = 0; i < 3; ++i)
        os << box.lowerLeft[i] << " ";
      os << "; upperRight: ";
      for(uint8_t i = 0; i < 3; ++i)
        os << box.upperRight[i] << " ";
      return os;
    }
  };
public:
  FullTree3D3L(const std::array<double, 3>& ll,
               const std::array<double, 3>& ur) {
    store[0].lowerLeft = ll;
    store[0].upperRight = ur;
    createBoxes(1, 0, ll, ur);
  }

  static void print3d(const std::array<double, 3> arr) {
    std::cout << arr[0] << "; " << arr[1] << "; " << arr[2];
  }

  static size_t getChildIdx(size_t parent, size_t child) { return 8*parent + child + 1; }

  const Box& getChild(size_t parent, size_t child) const {
    if(child > 7)
      throw std::logic_error(std::string(__PRETTY_FUNCTION__) + " node has only 7 hilds.");
    size_t idx = 8*parent + child + 1;
    if(idx >= nodesCount)
      throw std::logic_error(std::string(__PRETTY_FUNCTION__) + " no childs for "
      + std::to_string(parent) + " node.");
    return store[idx];
  }

  const Box& getBox(size_t id) const {return store[id];}

  bool isLeaf(size_t ind) { return ind > 72; }
  using PtDistr = std::array<std::vector<std::array<Mantid::coord_t, 3>>, nodesCount>;
  PtDistr distribute(const std::vector<std::array<Mantid::coord_t, 3>>& points, size_t threshold) {
    PtDistr res;

    // set all points to leaf nodes
    for(const auto& pt: points) {
      for (uint16_t i = 73; i < nodesCount; ++i) {
        if (store[i].contains(pt))
          res[i].emplace_back(pt);
      }
    }

    //accamulate points in the nodes of level 2 if
    //number of events <= threshold
    for(uint16_t i = 9; i < 73; ++i) {
      size_t count{0};
      auto& vect = res[i];
      for(uint16_t j = 1; j <= 8; ++j)
        count += res[i*8+j].size();
      if(count <= threshold)
        for(uint16_t j = 1; j <= 8; ++j) {
          vect.insert(vect.end(), res[i * 8 + j].begin(), res[i * 8 + j].end());
          res[i * 8 + j].clear();
        }
    }

    //accamulate points in the nodes of level 1 if
    //number of events <= threshold
    for(uint16_t i = 1; i < 9; ++i) {
      size_t count{0};
      auto& vect = res[i];
      for(uint16_t j = 1; j <= 8; ++j)
        count += res[i*8+j].size();
      if(count <= threshold)
        for(uint16_t j = 1; j <= 8; ++j) {
          vect.insert(vect.end(), res[i * 8 + j].begin(), res[i * 8 + j].end());
          res[i * 8 + j].clear();
        }
    }

    //accamulate points in the node of level 0 if
    //number of events <= threshold
    size_t i = 0;
    size_t count{0};
    auto& vect = res[i];
    for(uint16_t j = 1; j <= 8; ++j)
      count += res[i*8+j].size();
    if(count <= threshold)
      for(uint16_t j = 1; j <= 8; ++j) {
        vect.insert(vect.end(), res[i * 8 + j].begin(), res[i * 8 + j].end());
        res[i * 8 + j].clear();
      }
    return res;
  }

private:
  static double nextBigger(const double& in) {
    return std::nextafter(in, std::numeric_limits<double>::max());
  }
  void putChilds(const size_t& beforeStart, const std::array<double, 3>& ll,
                 const std::array<double, 3>& ur) {
    auto curIdx = beforeStart;
    std::array<double, 3> ctrUp = Box(ll, ur).center();
    std::array<double, 3> ctrLow{nextBigger(ctrUp[0]), nextBigger(ctrUp[1]), nextBigger(ctrUp[2])};
    store[++curIdx] = Box{{ll[0], ll[1], ll[2]},
                          {ctrUp[0], ctrUp[1], ctrUp[2]}};
    store[++curIdx] = Box{{ctrLow[0], ll[1], ll[2]},
                          {ur[0], ctrUp[1], ctrUp[2]}};
    store[++curIdx] = Box{{ll[0], ctrLow[1], ll[2]},
                          {ctrUp[0], ur[1], ctrUp[2]}};
    store[++curIdx] = Box{{ll[0], ll[1], ctrLow[2]},
                          {ctrUp[0], ctrUp[1], ur[2]}};
    store[++curIdx] = Box{{ctrLow[0], ctrLow[1], ll[2]},
                          {ur[0], ur[1], ctrUp[2]}};
    store[++curIdx] = Box{{ll[0], ctrLow[1], ctrLow[2]},
                          {ctrUp[0], ur[1], ur[2]}};
    store[++curIdx] = Box{{ctrLow[0], ll[1], ctrLow[2]},
                          {ur[0], ctrUp[1], ur[2]}};
    store[++curIdx] = Box{{ctrLow[0], ctrLow[1], ctrLow[2]},
                          {ur[0], ur[1], ur[2]}};
    std::sort(store.begin() + curIdx - 7, store.begin() + 1 + curIdx,
              [](Box& a, Box& b) {
                unsigned i = 3; while(i-->0) {
                const auto& ac = a.lowerLeft[i];
                const auto& bc = b.lowerLeft[i];
                if (ac < bc) return true;
                if (ac > bc) return false;
              }
                return true;
              });
  }

  void createBoxes(uint32_t lvl, size_t id, const std::array<double, 3>& ll,
                   const std::array<double, 3>& ur) {
    if(lvl > level)
      return;
    putChilds(id, ll, ur);
    for(uint8_t i = 1; i <= 8; ++i) {
      createBoxes(lvl + 1, (id + i) * 8, store[id+i].lowerLeft, store[id+i].upperRight);
    }
  }
private:
  std::array<Box, nodesCount> store;
};

using Mantid::MDAlgorithms::ConvToMDEventsWSIndexing;

class MDEventTreeBuilderTest : public CxxTest::TestSuite {
  static constexpr size_t ND = 3;
  using Point = std::array<Mantid::coord_t, ND>;
  using Points = std::vector<Point>;
  template <size_t nd>
  using MDEventTml = typename Mantid::DataObjects::MDLeanEvent<nd>;
  using MDEvent = MDEventTml<ND>;
  using MDNode = Mantid::API::IMDNode;
  using MDEventStore = std::vector<MDEvent>;
  using MDEventIterator = MDEventStore ::iterator;
  using TreeBuilder = Mantid::MDAlgorithms::MDEventTreeBuilder<ND, MDEventTml, MDEventIterator>;

  const std::array<double, 3> lowerLeft {0, 0, 0};
  const std::array<double, 3> upperRight {8, 8, 8};
  const size_t splitTreshold = 10;

  /**
   * interface class for generators of test input
   */
  class InputGenerator{
  public:
    virtual Points generate() const = 0;
    virtual std::string description() const = 0;
  };

  class SimpleInput : public InputGenerator {
  public:
    SimpleInput(size_t N) : n(N) {}
    std::string description() const override final {
      return "Generates " + std::to_string(n) + " of points with all"
      "coordinates equal to 0.5. Make sense to check "
      "correctness of splitting and not splitting.";
    }

    Points generate() const override final {
      Points points;
      for(size_t i = 0; i < n; ++i)
        points.emplace_back(std::array<Mantid::coord_t, ND>{0.5, 0.5, 0.5});
      return points;
    }
  private:
    size_t n;
  };

  class CheckBasicSplitting : public InputGenerator {
  public:
    CheckBasicSplitting(size_t N,
        const std::array<double, 3>& ll,
        const std::array<double, 3>& ur) :
        nPerLeaf(N), lowerLeft(ll), upperRight(ur) {}

    std::string description() const override {
      return "Generates " + std::to_string(nPerLeaf) +
      " points with all for every leaf box in the center "
      "of the box.";
    }
    Points generate() const override {
      FullTree3D3L justForBoxes(lowerLeft, upperRight);
      Points points;
      for(size_t i = 73 ; i < 585; ++i)
        for(size_t _ = 0; _ < nPerLeaf; ++_) {
          auto ctr = justForBoxes.getBox(i).center();
          points.emplace_back(Point{static_cast<float>(ctr[0]),
                                    static_cast<float>(ctr[1]),
                                    static_cast<float>(ctr[2])});
        }
      return points;
    }
  protected:
    size_t nPerLeaf;
    const std::array<double, 3>& lowerLeft;
    const std::array<double, 3>& upperRight;
  };

  class CheckPreciseSplitting : public CheckBasicSplitting {
  public:
    CheckPreciseSplitting(size_t N,
        const std::array<double, 3>& ll,
        const std::array<double, 3>& ur, double e) :
        CheckBasicSplitting(N, ll, ur), eps(e) {}

    std::string description() const override final {
      return "Generates " + std::to_string(nPerLeaf) +
          " points with all for every leaf box close to"
          "float bounds of the box (eps) to check accuracy.";
    }
    Points generate() const override final {
      FullTree3D3L justForBoxes(lowerLeft, upperRight);
      Points points;
      for(size_t i = 73 ; i < 585; ++i) {
        const auto& bx = justForBoxes.getBox(i);
        for (size_t j = 0; j < nPerLeaf; ++j) {
          Point lower, upper;
          for(int d = 0; d < 3; ++d) {
            if(fabs(bx.upperRight[d] - bx.lowerLeft[d]) < 2*eps) {
              lower[d] = (smallerClosestFloat(biggerClosestFloat(bx.lowerLeft[d]) + eps));
              upper[d] = (biggerClosestFloat(smallerClosestFloat(bx.upperRight[d]) - eps));
            } else {
              lower[d] = static_cast<float>(bx.upperRight[d] + bx.lowerLeft[d]) / 2;
              upper[d] = lower[d];
            }
          }
          points.emplace_back(j%2==0 ? lower : upper);
        }
      }
      return points;
    }
  private:
    float biggerClosestFloat(const double& d) const {
      float res = static_cast<float>(d);
      if(res < d)
        return std::nextafter(res, std::numeric_limits<float>::max());
      else
        return res;
    }
    float smallerClosestFloat(const double& d) const {
      float res = static_cast<float>(d);
      if(res > d)
        return std::nextafter(res, std::numeric_limits<float>::min());
      else
        return res;
    }
  private:
    double eps;
  };

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static MDEventTreeBuilderTest *createSuite() { return new MDEventTreeBuilderTest(); }
  static void destroySuite( MDEventTreeBuilderTest *suite ) { delete suite; }


  void test_sructure() {

    static std::vector<std::unique_ptr<InputGenerator>> generators;
    //All points in one child node
    generators.emplace_back(std::make_unique<SimpleInput>(11));
    //All points in top level node
    generators.emplace_back(std::make_unique<SimpleInput>(5));
    // Every leaf has 2 points in it
    generators.emplace_back(std::make_unique<CheckBasicSplitting>(2, lowerLeft, upperRight));
    // Every leaf has 2 points close to boundaries in it
    generators.emplace_back(std::make_unique<CheckPreciseSplitting>(4, lowerLeft, upperRight, 0.0001));

    for(auto& gen: generators)
      TSM_ASSERT_EQUALS(gen->description().c_str(),
          checkStructure(gen->generate(), lowerLeft, upperRight, splitTreshold), true);
  }
private:
  bool compare(FullTree3D3L::PtDistr& distr, size_t id, Mantid::API::IMDNode* nd) {
    bool res = (distr[id].empty());
    if(nd->isLeaf()) {
      res = (distr[id].size() == nd->getNPoints());
    } else {
      for (int i = 0; i < 8; ++i)
        res &= compare(distr, FullTree3D3L::getChildIdx(id, i), nd->getChild(i));
    }

    return res;
  }
  bool compareTrees(FullTree3D3L::PtDistr& distr, Mantid::API::IMDNode* root) {
    return compare(distr, 0, root);
  }

  bool checkStructure(const Points& points,
                      const std::array<double, 3>& ll,   //lower left bound of global space
                      const std::array<double, 3>& ur,
                      size_t splitTreshold) { //upper right bound of global space
    FullTree3D3L t3d(ll, ur);
    Mantid::API::BoxController_sptr bc =
        boost::shared_ptr<Mantid::API::BoxController>(new Mantid::API::BoxController(ND));
    bc->setMaxDepth(3);
    bc->setSplitInto(2);
    bc->setSplitThreshold(splitTreshold);
    MDSpaceBounds<ND> bds{};
    bds(0,0) = static_cast<Mantid::coord_t>(ll[0]);
    bds(0,1) = static_cast<Mantid::coord_t>(ur[0]);
    bds(1,0) = static_cast<Mantid::coord_t>(ll[1]);
    bds(1,1) = static_cast<Mantid::coord_t>(ur[1]);
    bds(2,0) = static_cast<Mantid::coord_t>(ll[2]);
    bds(2,1) = static_cast<Mantid::coord_t>(ur[2]);
    TreeBuilder tb(1, 0, bc, bds);

    auto res = t3d.distribute(points, splitTreshold);

    MDEventStore mdEvents;
    mdEvents.reserve(points.size());
    for(const auto& pt: points)
      mdEvents.emplace_back(.0f ,.0f , pt.data());

    auto topNode = tb.distribute(mdEvents);

    return compareTrees(res, topNode), true;
  }

};


#endif /* MANTID_MDALGORITHMS_CONVTOMDEVENTSWSINDEXINGTEST_H_ */