#include "MantidGeometry/Objects/Object.h"
#include "MantidKernel/Strings.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/MultiThreaded.h"
#include "MantidGeometry/Objects/Rules.h"
#include "MantidGeometry/Objects/Track.h"

#include "MantidGeometry/Surfaces/Surface.h"
#include "MantidGeometry/Surfaces/LineIntersectVisit.h"
#include "MantidGeometry/Surfaces/Cylinder.h"
#include "MantidGeometry/Surfaces/Cone.h"

#include "MantidGeometry/Rendering/GeometryHandler.h"
#include "MantidGeometry/Rendering/CacheGeometryHandler.h"
#include "MantidGeometry/Rendering/vtkGeometryCacheReader.h"
#include "MantidGeometry/Rendering/vtkGeometryCacheWriter.h"
#include "MantidKernel/RegexStrings.h"
#include "MantidKernel/Tolerance.h"
#include <deque>
#include <iostream>
#include <stack>

namespace Mantid {
namespace Geometry {

using Kernel::V3D;
using Kernel::Quat;

/**
*  Default constuctor
*/
Object::Object()
    : ObjName(0), TopRule(0), m_boundingBox(), AABBxMax(0), AABByMax(0),
      AABBzMax(0), AABBxMin(0), AABByMin(0), AABBzMin(0), boolBounded(false),
      handle(), bGeometryCaching(false),
      vtkCacheReader(boost::shared_ptr<vtkGeometryCacheReader>()),
      vtkCacheWriter(boost::shared_ptr<vtkGeometryCacheWriter>()),
      m_material() // empty by default
{
  handle = boost::shared_ptr<GeometryHandler>(new CacheGeometryHandler(this));
}

/**
*  Construct with original shape xml knowledge.
*  @param shapeXML : string with original shape xml.
*/
Object::Object(const std::string &shapeXML)
    : ObjName(0), TopRule(0), m_boundingBox(), AABBxMax(0), AABByMax(0),
      AABBzMax(0), AABBxMin(0), AABByMin(0), AABBzMin(0), boolBounded(false),
      handle(), bGeometryCaching(false),
      vtkCacheReader(boost::shared_ptr<vtkGeometryCacheReader>()),
      vtkCacheWriter(boost::shared_ptr<vtkGeometryCacheWriter>()),
      m_shapeXML(shapeXML), m_material() // empty by default
{
  handle = boost::shared_ptr<GeometryHandler>(new CacheGeometryHandler(this));
}

/**
* Copy constructor
* @param A :: The object to initialise this copy from
*/
Object::Object(const Object &A)
    : ObjName(A.ObjName), TopRule((A.TopRule) ? A.TopRule->clone() : NULL),
      m_boundingBox(A.m_boundingBox), AABBxMax(A.AABBxMax),
      AABByMax(A.AABByMax), AABBzMax(A.AABBzMax), AABBxMin(A.AABBxMin),
      AABByMin(A.AABByMin), AABBzMin(A.AABBzMin), boolBounded(A.boolBounded),
      handle(A.handle->clone()), bGeometryCaching(A.bGeometryCaching),
      vtkCacheReader(A.vtkCacheReader), vtkCacheWriter(A.vtkCacheWriter),
      m_shapeXML(A.m_shapeXML), m_material(A.m_material) {
  if (TopRule)
    createSurfaceList();
}

/**
* Assignment operator
* @param A :: Object to copy
* @return *this
*/
Object &Object::operator=(const Object &A) {
  if (this != &A) {
    ObjName = A.ObjName;
    delete TopRule;
    TopRule = (A.TopRule) ? A.TopRule->clone() : 0;
    AABBxMax = A.AABBxMax;
    AABByMax = A.AABByMax;
    AABBzMax = A.AABBzMax;
    AABBxMin = A.AABBxMin;
    AABByMin = A.AABByMin;
    AABBzMin = A.AABBzMin;
    boolBounded = A.boolBounded;
    handle = A.handle->clone();
    bGeometryCaching = A.bGeometryCaching;
    vtkCacheReader = A.vtkCacheReader;
    vtkCacheWriter = A.vtkCacheWriter;
    m_shapeXML = A.m_shapeXML;
    m_material = A.m_material;

    if (TopRule)
      createSurfaceList();
  }
  return *this;
}

/**
* Destructor
* Deletes the rule
*/
Object::~Object() { delete TopRule; }

/**
 * @param material The new Material that the object is composed from
 */
void Object::setMaterial(const Kernel::Material &material) {
  m_material = material;
}

/**
 * @return The Material that the object is composed from
 */
const Kernel::Material &Object::material() const { return m_material; }

/**
* Returns whether this object has a valid shape
* @returns True if the surface list is populated and there is a
* defined TopRule, false otherwise.
*/
bool Object::hasValidShape() const {
  // Assume invalid shape if object has no 'TopRule' or surfaces
  return (TopRule != NULL && !SurList.empty());
}

/**
* Object line ==  cell
* @param ON :: Object name
* @param Ln :: Input string must be :  {rules}
* @returns 1 on success and zero on failure
*/
int Object::setObject(const int ON, const std::string &Ln) {
  // Split line
  const boost::regex letters("[a-zA-Z]"); // Does the string now contain junk...
  if (Mantid::Kernel::Strings::StrLook(Ln, letters))
    return 0;

  if (procString(Ln)) // this currently does not fail:
  {
    SurList.clear();
    ObjName = ON;
    return 1;
  }

  // failure
  return 0;
}

/**
* Returns just the cell string object
* @param MList :: List of indexable Hulls
* @return Cell String (from TopRule)
* @todo Break infinite recusion
*/
void Object::convertComplement(const std::map<int, Object> &MList)

{
  this->procString(this->cellStr(MList));
  return;
}

/**
* Returns just the cell string object
* @param MList :: List of indexable Hulls
* @return Cell String (from TopRule)
* @todo Break infinite recusion
*/
std::string Object::cellStr(const std::map<int, Object> &MList) const {
  std::string TopStr = this->topRule()->display();
  std::string::size_type pos = TopStr.find('#');
  std::ostringstream cx;
  while (pos != std::string::npos) {
    pos++;
    cx << TopStr.substr(0, pos); // Everything including the #
    int cN(0);
    const int nLen =
        Mantid::Kernel::Strings::convPartNum(TopStr.substr(pos), cN);
    if (nLen > 0) {
      cx << "(";
      std::map<int, Object>::const_iterator vc = MList.find(cN);
      if (vc == MList.end())
        throw Kernel::Exception::NotFoundError(
            "Not found in the list of indexable hulls (Object::cellStr)", cN);
      // Not the recusion :: This will cause no end of problems
      // if there is an infinite loop.
      cx << vc->second.cellStr(MList);
      cx << ") ";
      pos += nLen;
    }
    TopStr.erase(0, pos);
    pos = TopStr.find('#');
  }
  cx << TopStr;
  return cx.str();
}

/*
* Calcluate if there are any complementary components in
* the object. That is lines with #(....)
* @throw ColErr::ExBase :: Error with processing
* @param Ln :: Input string must:  ID Mat {Density}  {rules}
* @param Cnum :: Number for cell since we don't have one
* @retval 0 on no work to do
* @retval 1 :: A (maybe there are many) #(...) object found
*/
int Object::complementaryObject(const int Cnum, std::string &Ln) {
  std::string::size_type posA = Ln.find("#(");
  // No work to do ?
  if (posA == std::string::npos)
    return 0;
  posA += 2;

  // First get the area to be removed
  int brackCnt;
  std::string::size_type posB;
  posB = Ln.find_first_of("()", posA);
  if (posB == std::string::npos)
    throw std::runtime_error("Object::complemenet :: " + Ln);

  brackCnt = (Ln[posB] == '(') ? 1 : 0;
  while (posB != std::string::npos && brackCnt) {
    posB = Ln.find_first_of("()", posB);
    brackCnt += (Ln[posB] == '(') ? 1 : -1;
    posB++;
  }

  std::string Part = Ln.substr(posA, posB - (posA + 1));

  ObjName = Cnum;
  if (procString(Part)) {
    SurList.clear();
    Ln.erase(posA - 1, posB + 1); // Delete brackets ( Part ) .
    std::ostringstream CompCell;
    CompCell << Cnum << " ";
    Ln.insert(posA - 1, CompCell.str());
    return 1;
  }

  throw std::runtime_error("Object::complemenet :: " + Part);
  return 0;
}

/**
* Determine if the object has a complementary object
* @retval 1 :: true
* @retval 0 :: false
*/
int Object::hasComplement() const {

  if (TopRule)
    return TopRule->isComplementary();
  return 0;
}

/**
* Goes through the cell objects and adds the pointers
* to the SurfPoint keys (using their keyN)
* @param Smap :: Map of surface Keys and Surface Pointers
* @retval 1000+ keyNumber :: Error with keyNumber
* @retval 0 :: successfully populated all the whole Object.
*/
int Object::populate(const std::map<int, Surface *> &Smap) {
  std::deque<Rule *> Rst;
  Rst.push_back(TopRule);
  Rule *TA, *TB; // Tmp. for storage

  int Rcount(0);
  while (!Rst.empty()) {
    Rule *T1 = Rst.front();
    Rst.pop_front();
    if (T1) {
      // if an actual surface process :
      SurfPoint *KV = dynamic_cast<SurfPoint *>(T1);
      if (KV) {
        // Ensure that we have a it in the surface list:
        std::map<int, Surface *>::const_iterator mf = Smap.find(KV->getKeyN());
        if (mf != Smap.end()) {
          KV->setKey(mf->second);
          Rcount++;
        } else {
          throw Kernel::Exception::NotFoundError("Object::populate",
                                                 KV->getKeyN());
        }
      }
      // Not a surface : Determine leaves etc and add to stack:
      else {
        TA = T1->leaf(0);
        TB = T1->leaf(1);
        if (TA)
          Rst.push_back(TA);
        if (TB)
          Rst.push_back(TB);
      }
    }
  }
  createSurfaceList();
  return 0;
}

/**
* This takes a string Ln, finds the first two
* Rxxx function, determines their join type
* make the rule,  adds to vector then removes two old rules from
* the vector, updates string
* @param Ln :: String to porcess
* @param Rlist :: Map of rules (added to)
* @param compUnit :: Last computed unit
* @retval 0 :: No rule to find
* @retval 1 :: A rule has been combined
*/
int Object::procPair(std::string &Ln, std::map<int, Rule *> &Rlist,
                     int &compUnit) const

{
  unsigned int Rstart;
  unsigned int Rend;
  int Ra, Rb;

  for (Rstart = 0; Rstart < Ln.size() && Ln[Rstart] != 'R'; Rstart++)
    ;

  int type = 0; // intersection

  // plus 1 to skip 'R'
  if (Rstart == Ln.size() ||
      !Mantid::Kernel::Strings::convert(Ln.c_str() + Rstart + 1, Ra) ||
      Rlist.find(Ra) == Rlist.end())
    return 0;

  for (Rend = Rstart + 1; Rend < Ln.size() && Ln[Rend] != 'R'; Rend++) {
    if (Ln[Rend] == ':')
      type = 1; // make union
  }
  if (Rend == Ln.size() ||
      !Mantid::Kernel::Strings::convert(Ln.c_str() + Rend + 1, Rb) ||
      Rlist.find(Rb) == Rlist.end())
    return 0;

  // Get end of number (digital)
  for (Rend++; Rend < Ln.size() && Ln[Rend] >= '0' && Ln[Rend] <= '9'; Rend++)
    ;

  // Get rules
  Rule *RRA = Rlist[Ra];
  Rule *RRB = Rlist[Rb];
  Rule *Join = (type) ? static_cast<Rule *>(new Union(RRA, RRB))
                      : static_cast<Rule *>(new Intersection(RRA, RRB));
  Rlist[Ra] = Join;
  Rlist.erase(Rlist.find(Rb));

  // Remove space round pair
  int fb;
  for (fb = Rstart - 1; fb >= 0 && Ln[fb] == ' '; fb--)
    ;
  Rstart = (fb < 0) ? 0 : fb;
  for (fb = Rend; fb < static_cast<int>(Ln.size()) && Ln[fb] == ' '; fb++)
    ;
  Rend = fb;

  std::stringstream cx;
  cx << " R" << Ra << " ";
  Ln.replace(Rstart, Rend, cx.str());
  compUnit = Ra;
  return 1;
}

/**
* Takes a Rule item and makes it a complementary group
* @param RItem :: to encapsulate
* @returns the complementary group
*/
CompGrp *Object::procComp(Rule *RItem) const {
  if (!RItem)
    return new CompGrp();

  Rule *Pptr = RItem->getParent();
  CompGrp *CG = new CompGrp(Pptr, RItem);
  if (Pptr) {
    const int Ln = Pptr->findLeaf(RItem);
    Pptr->setLeaf(CG, Ln);
  }
  return CG;
}

/**
* - (a) Uses the Surface list to check those surface
* that the point is on.
* - (b) Creates a list of normals to the touching surfaces
* - (c) Checks if normals and "normal pair bisection vector" are contary.
* If any are found to be so the the point is
* on a surface.
* - (d) Return 1 / 0 depending on test (c)

* \todo This needs to be completed to deal with apex points
* In the case of a apex (e.g. top of a pyramid) you need
* to interate over all clusters of points on the Snorm
* ie. sum of 2, sum of 3 sum of 4. etc. to be certain
* to get a correct normal test.

* @param Pt :: Point to check
* @returns 1 if the point is on the surface
*/
bool Object::isOnSide(const Kernel::V3D &Pt) const {
  std::list<Kernel::V3D> Snorms; // Normals from the constact surface.

  std::vector<const Surface *>::const_iterator vc;
  for (vc = SurList.begin(); vc != SurList.end(); ++vc) {
    if ((*vc)->onSurface(Pt)) {
      Snorms.push_back((*vc)->surfaceNormal(Pt));
      // can check direct normal here since one success
      // means that we can return 1 and finish
      if (!checkSurfaceValid(Pt, Snorms.back()))
        return true;
    }
  }
  std::list<Kernel::V3D>::const_iterator xs, ys;
  Kernel::V3D NormPair;
  for (xs = Snorms.begin(); xs != Snorms.end(); ++xs)
    for (ys = xs, ++ys; ys != Snorms.end(); ++ys) {
      NormPair = (*ys) + (*xs);
      NormPair.normalize();
      if (!checkSurfaceValid(Pt, NormPair))
        return true;
    }
  // Ok everthing failed return 0;
  return false;
}

/**
* Determine if a point is valid by checking both
* directions of the normal away from the line
* A good point will have one valid and one invalid.
* @param C :: Point on a basic surface to check
* @param Nm :: Direction +/- to be checked
* @retval +1 ::  Point outlayer (ie not in object)
* @retval -1 :: Point included (e.g at convex intersection)
* @retval 0 :: success
*/
int Object::checkSurfaceValid(const Kernel::V3D &C,
                              const Kernel::V3D &Nm) const {
  int status(0);
  Kernel::V3D tmp = C + Nm * (Kernel::Tolerance * 5.0);
  status = (!isValid(tmp)) ? 1 : -1;
  tmp -= Nm * (Kernel::Tolerance * 10.0);
  status += (!isValid(tmp)) ? 1 : -1;
  return status / 2;
}

/**
* Determines is Pt is within the object or on the surface
* @param Pt :: Point to be tested
* @returns 1 if true and 0 if false
*/
bool Object::isValid(const Kernel::V3D &Pt) const {
  if (!TopRule)
    return false;
  return TopRule->isValid(Pt);
}

/**
* Determines is group of surface maps are valid
* @param SMap :: map of SurfaceNumber : status
* @returns 1 if true and 0 if false
*/
bool Object::isValid(const std::map<int, int> &SMap) const {
  if (!TopRule)
    return false;
  return TopRule->isValid(SMap);
}

/**
* Uses the topRule* to create a surface list
* by iterating throught the tree
* @param outFlag :: Sends output to standard error if true
* @return 1 (should be number of surfaces)
*/
int Object::createSurfaceList(const int outFlag) {
  SurList.clear();
  std::stack<const Rule *> TreeLine;
  TreeLine.push(TopRule);
  while (!TreeLine.empty()) {
    const Rule *tmpA = TreeLine.top();
    TreeLine.pop();
    const Rule *tmpB = tmpA->leaf(0);
    const Rule *tmpC = tmpA->leaf(1);
    if (tmpB || tmpC) {
      if (tmpB)
        TreeLine.push(tmpB);
      if (tmpC)
        TreeLine.push(tmpC);
    } else {
      const SurfPoint *SurX = dynamic_cast<const SurfPoint *>(tmpA);
      if (SurX) {
        SurList.push_back(SurX->getKey());
      }
    }
  }
  if (outFlag) {

    std::vector<const Surface *>::const_iterator vc;
    for (vc = SurList.begin(); vc != SurList.end(); ++vc) {
      std::cerr << "Point == " << reinterpret_cast<long int>(*vc) << std::endl;
      std::cerr << (*vc)->getName() << std::endl;
    }
  }
  return 1;
}

/**
* Returns all of the numbers of surfaces
* @return Surface numbers
*/
std::vector<int> Object::getSurfaceIndex() const {
  std::vector<int> out;
  transform(SurList.begin(), SurList.end(),
            std::insert_iterator<std::vector<int>>(out, out.begin()),
            std::mem_fun(&Surface::getName));
  return out;
}

/**
* Removes a surface and then re-builds the
* cell. This could be done by just removing
* the surface from the object.
* @param SurfN :: Number for the surface
* @return number of surfaces removes
*/
int Object::removeSurface(const int SurfN) {
  if (!TopRule)
    return -1;
  const int cnt = Rule::removeItem(TopRule, SurfN);
  if (cnt)
    createSurfaceList();
  return cnt;
}

/**
* Removes a surface and then re-builds the cell.
* @param SurfN :: Number for the surface
* @param NsurfN :: New surface number
* @param SPtr :: Surface pointer for surface NsurfN
* @return number of surfaces substituted
*/
int Object::substituteSurf(const int SurfN, const int NsurfN, Surface *SPtr) {
  if (!TopRule)
    return 0;
  const int out = TopRule->substituteSurf(SurfN, NsurfN, SPtr);
  if (out)
    createSurfaceList();
  return out;
}

/**
* Prints almost everything
*/
void Object::print() const {
  std::deque<Rule *> Rst;
  std::vector<int> Cells;
  int Rcount(0);
  Rst.push_back(TopRule);
  Rule *TA, *TB; // Temp. for storage

  while (!Rst.empty()) {
    Rule *T1 = Rst.front();
    Rst.pop_front();
    if (T1) {
      Rcount++;
      SurfPoint *KV = dynamic_cast<SurfPoint *>(T1);
      if (KV)
        Cells.push_back(KV->getKeyN());
      else {
        TA = T1->leaf(0);
        TB = T1->leaf(1);
        if (TA)
          Rst.push_back(TA);
        if (TB)
          Rst.push_back(TB);
      }
    }
  }

  std::cout << "Name == " << ObjName << std::endl;
  std::cout << "Rules == " << Rcount << std::endl;
  std::vector<int>::const_iterator mc;
  std::cout << "Surface included == ";
  for (mc = Cells.begin(); mc < Cells.end(); ++mc) {
    std::cout << (*mc) << " ";
  }
  std::cout << std::endl;
  return;
}

/**
* Takes the complement of a group
*/
void Object::makeComplement() {
  Rule *NCG = procComp(TopRule);
  TopRule = NCG;
  return;
}

/**
* Displays the rule tree
*/
void Object::printTree() const {
  std::cout << "Name == " << ObjName << std::endl;
  std::cout << TopRule->display() << std::endl;
  return;
}

/**
* Write the object to a string.
* This includes only rules.
* @return Object Line
*/
std::string Object::cellCompStr() const {
  std::ostringstream cx;
  if (TopRule)
    cx << TopRule->display();
  return cx.str();
}

/**
* Write the object to a string.
* This includes the Name but not post-fix operators
* @return Object Line
*/
std::string Object::str() const {
  std::ostringstream cx;
  if (TopRule) {
    cx << ObjName << " ";
    cx << TopRule->display();
  }
  return cx.str();
}

/**
* Write the object to a standard stream
* in standard MCNPX output format.
* @param OX :: Output stream (required for multiple std::endl)
*/
void Object::write(std::ostream &OX) const {
  std::ostringstream cx;
  cx.precision(10);
  cx << str();
  Mantid::Kernel::Strings::writeMCNPX(cx.str(), OX);
  return;
}

/**
* Processes the cell string. This is an internal function
* to process a string with - String type has #( and ( )
* @param Line :: String value
* @returns 1 on success
*/
int Object::procString(const std::string &Line) {
  delete TopRule;
  TopRule = 0;
  std::map<int, Rule *> RuleList; // List for the rules
  int Ridx = 0; // Current index (not necessary size of RuleList
  // SURFACE REPLACEMENT
  // Now replace all free planes/Surfaces with appropiate Rxxx
  SurfPoint *TmpR(0); // Tempory Rule storage position
  CompObj *TmpO(0);   // Tempory Rule storage position

  std::string Ln = Line;
  // Remove all surfaces :
  std::ostringstream cx;
  const std::string::size_type length = Ln.length();
  for (size_t i = 0; i < length; i++) {
    if (isdigit(Ln[i]) || Ln[i] == '-') {
      int SN;
      int nLen = Mantid::Kernel::Strings::convPartNum(Ln.substr(i), SN);
      if (!nLen)
        throw std::invalid_argument(
            "Invalid surface string in Object::ProcString : " + Line);
      // Process #Number
      if (i != 0 && Ln[i - 1] == '#') {
        TmpO = new CompObj();
        TmpO->setObjN(SN);
        RuleList[Ridx] = TmpO;
      } else // Normal rule
      {
        TmpR = new SurfPoint();
        TmpR->setKeyN(SN);
        RuleList[Ridx] = TmpR;
      }
      cx << " R" << Ridx << " ";
      Ridx++;
      i += nLen;
    }
    if (i < length)
      cx << Ln[i];
  }
  Ln = cx.str();
  // PROCESS BRACKETS

  int brack_exists = 1;
  while (brack_exists) {
    std::string::size_type rbrack = Ln.find(')');
    std::string::size_type lbrack = Ln.rfind('(', rbrack);
    if (rbrack != std::string::npos && lbrack != std::string::npos) {
      std::string Lx = Ln.substr(lbrack + 1, rbrack - lbrack - 1);
      // Check to see if a #( unit
      int compUnit(0);
      while (procPair(Lx, RuleList, compUnit))
        ;
      Ln.replace(lbrack, 1 + rbrack - lbrack, Lx);
      // Search back and find if # ( exists.
      int hCnt;
      for (hCnt = static_cast<int>(lbrack) - 1; hCnt >= 0 && isspace(Ln[hCnt]);
           hCnt--)
        ;
      if (hCnt >= 0 && Ln[hCnt] == '#') {
        RuleList[compUnit] = procComp(RuleList[compUnit]);
        Ln.erase(hCnt, lbrack - hCnt);
      }
    } else
      brack_exists = 0;
  }
  // Do outside loop...
  int nullInt;
  while (procPair(Ln, RuleList, nullInt))
    ;

  if (RuleList.size() != 1) {
    std::cerr << "Map size not equal to 1 == " << RuleList.size() << std::endl;
    std::cerr << "Error Object::ProcString : " << Ln << std::endl;
    exit(1);
    return 0;
  }
  TopRule = (RuleList.begin())->second;
  return 1;
}

/**
* Given a track, fill the track with valid section
* @param UT :: Initial track
* @return Number of segments added
*/
int Object::interceptSurface(Geometry::Track &UT) const {
  int cnt = UT.count(); // Number of intersections original track
  // Loop over all the surfaces.
  LineIntersectVisit LI(UT.startPoint(), UT.direction());
  std::vector<const Surface *>::const_iterator vc;
  for (vc = SurList.begin(); vc != SurList.end(); ++vc) {
    (*vc)->acceptVisitor(LI);
  }
  const auto &IPts(LI.getPoints());
  const auto &dPts(LI.getDistance());

  auto ditr = dPts.begin();
  auto itrEnd = IPts.end();
  for (auto iitr = IPts.begin(); iitr != itrEnd; ++iitr, ++ditr) {
    if (*ditr > 0.0) // only interested in forward going points
    {
      // Is the point and enterance/exit Point
      const int flag = calcValidType(*iitr, UT.direction());
      UT.addPoint(flag, *iitr, *this);
    }
  }
  UT.buildLink();
  // Return number of track segments added
  return (UT.count() - cnt);
}

/**
* Calculate if a point PT is a valid point on the track
* @param Pt :: Point to calculate from.
* @param uVec :: Unit vector of the track
* @retval 0 :: Not valid / double valid
* @retval 1 :: Entry point
* @retval -1 :: Exit Point
*/
int Object::calcValidType(const Kernel::V3D &Pt,
                          const Kernel::V3D &uVec) const {
  const Kernel::V3D shift(uVec * Kernel::Tolerance * 25.0);
  const Kernel::V3D testA(Pt - shift);
  const Kernel::V3D testB(Pt + shift);
  const int flagA = isValid(testA);
  const int flagB = isValid(testB);
  if (!(flagA ^ flagB))
    return 0;
  return (flagA) ? -1 : 1;
}

/**
* Find soild angle of object wrt the observer. This interface routine calls
* either
* getTriangleSoldiAngle or getRayTraceSolidAngle. Choice made on number of
* triangles
* in the discete surface representation.
* @param observer :: point to measure solid angle from
* @return :: estimate of solid angle of object. Accuracy depends on object
* shape.
*/
double Object::solidAngle(const Kernel::V3D &observer) const {
  if (this->NumberOfTriangles() > 30000)
    return rayTraceSolidAngle(observer);
  return triangleSolidAngle(observer);
}

/**
* Find solid angle of object wrt the observer with a scaleFactor for the object.
* @param observer :: point to measure solid angle from
* @param scaleFactor :: V3D giving scaling of the object
* @return :: estimate of solid angle of object. Accuracy depends on
* triangulation quality.
*/
double Object::solidAngle(const Kernel::V3D &observer,
                          const Kernel::V3D &scaleFactor) const

{
  return triangleSolidAngle(observer, scaleFactor);
}

/**
* Given an observer position find the approximate solid angle of the object
* @param observer :: position of the observer (V3D)
* @return Solid angle in steradians (+/- 1% if accurate bounding box available)
*/
double Object::rayTraceSolidAngle(const Kernel::V3D &observer) const {
  // Calculation of solid angle as numerical double integral over all
  // angles. This could be optimised further e.g. by
  // using a light weight version of the interceptSurface method - this does
  // more work
  // than is necessary in this application.
  // Accuracy is of the order of 1% for objects with an accurate bounding box,
  // though
  // less in the case of high aspect ratios.
  //
  // resBB controls accuracy and cost - linear accuracy improvement with
  // increasing res,
  // but quadratic increase in run time. If no bounding box found, resNoBB used
  // instead.
  const int resNoBB = 200, resPhiMin = 10;
  int res = resNoBB, itheta, jphi, resPhi;
  double theta, phi, sum, dphi, dtheta;
  if (this->isValid(observer) && !this->isOnSide(observer))
    return 4 * M_PI; // internal point
  if (this->isOnSide(observer))
    return 2 * M_PI; // this is wrong if on an edge
  // Use BB if available, and if observer not within it
  const BoundingBox &boundingBox = getBoundingBox();
  double thetaMax = M_PI;
  bool useBB = false, usePt = false;
  Kernel::V3D ptInObject, axis;
  Quat zToPt;

  // Is the bounding box a reasonable one?
  if (boundingBox.isNonNull() && !boundingBox.isPointInside(observer)) {
    useBB = usePt = true;
    thetaMax = boundingBox.angularWidth(observer);
    ptInObject = boundingBox.centrePoint();
    const int resBB = 100;
    res = resBB;
  }
  // Try and find a point in the object if useful bounding box not found
  if (!useBB) {
    usePt = getPointInObject(ptInObject) == 1;
  }
  if (usePt) {
    // found point in object, now get rotation that maps z axis to this
    // direction from observer
    ptInObject -= observer;
    double theta0 = -180.0 / M_PI * acos(ptInObject.Z() / ptInObject.norm());
    Kernel::V3D zDir(0.0, 0.0, 1.0);
    axis = ptInObject.cross_prod(zDir);
    if (axis.nullVector())
      axis = Kernel::V3D(1.0, 0.0, 0.0);
    zToPt(theta0, axis);
  }
  dtheta = thetaMax / res;
  int count = 0, countPhi;
  sum = 0.;
  for (itheta = 1; itheta <= res; itheta++) {
    // itegrate theta from 0 to maximum from bounding box, or PI otherwise
    theta = thetaMax * (itheta - 0.5) / res;
    resPhi = static_cast<int>(res * sin(theta));
    if (resPhi < resPhiMin)
      resPhi = resPhiMin;
    dphi = 2 * M_PI / resPhi;
    countPhi = 0;
    for (jphi = 1; jphi <= resPhi; jphi++) {
      // integrate phi from 0 to 2*PI
      phi = 2.0 * M_PI * (jphi - 0.5) / resPhi;
      Kernel::V3D dir(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));
      if (usePt)
        zToPt.rotate(dir);
      if (!useBB || boundingBox.doesLineIntersect(observer, dir)) {
        Track tr(observer, dir);
        if (this->interceptSurface(tr) > 0) {
          sum += dtheta * dphi * sin(theta);
          countPhi++;
        }
      }
    }
    // this break (only used in no BB defined) may be wrong if object has hole
    // in middle
    if (!useBB && countPhi == 0)
      break;
    count += countPhi;
  }
  if (!useBB && count < resPhiMin + 1) {
    // case of no bound box defined and object has few if any points in sum
    // redo integration on finer scale
    thetaMax = thetaMax * (itheta - 0.5) / res;
    dtheta = thetaMax / res;
    sum = 0;
    for (itheta = 1; itheta <= res; itheta++) {
      theta = thetaMax * (itheta - 0.5) / res;
      resPhi = static_cast<int>(res * sin(theta));
      if (resPhi < resPhiMin)
        resPhi = resPhiMin;
      dphi = 2 * M_PI / resPhi;
      countPhi = 0;
      for (jphi = 1; jphi <= resPhi; jphi++) {
        phi = 2.0 * M_PI * (jphi - 0.5) / resPhi;
        Kernel::V3D dir(sin(theta) * cos(phi), sin(theta) * sin(phi),
                        cos(theta));
        if (usePt)
          zToPt.rotate(dir);
        Track tr(observer, dir);
        if (this->interceptSurface(tr) > 0) {
          sum += dtheta * dphi * sin(theta);
          countPhi++;
        }
      }
      if (countPhi == 0)
        break;
    }
  }

  return sum;
}

/**
* Find the solid angle of a triangle defined by vectors a,b,c from point
*"observer"
*
* formula (Oosterom) O=2atan([a,b,c]/(abc+(a.b)c+(a.c)b+(b.c)a))
*
* @param a :: first point of triangle
* @param b :: second point of triangle
* @param c :: third point of triangle
* @param observer :: point from which solid angle is required
* @return :: solid angle of triangle in Steradians.
*/
double Object::getTriangleSolidAngle(const V3D &a, const V3D &b, const V3D &c,
                                     const V3D &observer) const {
  const V3D ao = a - observer;
  const V3D bo = b - observer;
  const V3D co = c - observer;
  const double modao = ao.norm();
  const double modbo = bo.norm();
  const double modco = co.norm();
  const double aobo = ao.scalar_prod(bo);
  const double aoco = ao.scalar_prod(co);
  const double boco = bo.scalar_prod(co);
  const double scalTripProd = ao.scalar_prod(bo.cross_prod(co));
  const double denom =
      modao * modbo * modco + modco * aobo + modbo * aoco + modao * boco;
  if (denom != 0.0)
    return 2.0 * atan2(scalTripProd, denom);
  else
    return 0.0; // not certain this is correct
}

/**
* Find solid angle of object from point "observer" using the
* OC triangluation of the object, if it exists
*
* @param observer :: Point from which solid angle is required
* @return the solid angle
*/
double Object::triangleSolidAngle(const V3D &observer) const {
  //
  // Because the triangles from OC are not consistently ordered wrt their
  // outward normal
  // internal points give incorrect solid angle. Surface points are difficult to
  // get right
  // with the triangle based method. Hence catch these two (unlikely) cases.
  const BoundingBox &boundingBox = this->getBoundingBox();
  if (boundingBox.isNonNull() && boundingBox.isPointInside(observer)) {
    if (isValid(observer)) {
      if (isOnSide(observer))
        return (2.0 * M_PI);
      else
        return (4.0 * M_PI);
    }
  }

  // If the object is a simple shape use the special methods
  double height(0.0), radius(0.0);
  int type(0);
  std::vector<Mantid::Kernel::V3D> geometry_vectors;
  // Maximum of 4 vectors depending on the type
  geometry_vectors.reserve(4);
  this->GetObjectGeom(type, geometry_vectors, radius, height);
  int nTri = this->NumberOfTriangles();
  // Cylinders are by far the most frequently used
  if (type == 3)
    return CylinderSolidAngle(observer, geometry_vectors[0],
                              geometry_vectors[1], radius, height);
  else if (type == 1)
    return CuboidSolidAngle(observer, geometry_vectors);
  else if (type == 2)
    return SphereSolidAngle(observer, geometry_vectors, radius);
  else if (type == 4)
    return ConeSolidAngle(observer, geometry_vectors[0], geometry_vectors[1],
                          radius, height);
  else if (nTri == 0) // Fall back to raytracing if there are no triangles
  {
    return rayTraceSolidAngle(observer);
  }
  // Compute a generic shape that has been triangulated
  else {
    double *vertices = this->getTriangleVertices();
    int *faces = this->getTriangleFaces();
    double sangle(0.0), sneg(0.0);
    for (int i = 0; i < nTri; i++) {
      int p1 = faces[i * 3], p2 = faces[i * 3 + 1], p3 = faces[i * 3 + 2];
      V3D vp1 =
          V3D(vertices[3 * p1], vertices[3 * p1 + 1], vertices[3 * p1 + 2]);
      V3D vp2 =
          V3D(vertices[3 * p2], vertices[3 * p2 + 1], vertices[3 * p2 + 2]);
      V3D vp3 =
          V3D(vertices[3 * p3], vertices[3 * p3 + 1], vertices[3 * p3 + 2]);
      double sa = getTriangleSolidAngle(vp1, vp2, vp3, observer);
      if (sa > 0.0) {
        sangle += sa;
      } else {
        sneg += sa;
      }
    }
    return 0.5 * (sangle - sneg);
  }
}
/**
* Find solid angle of object from point "observer" using the
* OC triangluation of the object, if it exists. This method expects a
* scaling vector scaleFactor that scales the three axes.
*
* @param observer :: Point from which solid angle is required.
* @param scaleFactor :: V3D each component giving the scaling of the object only
*(not observer)
* @return the solid angle
*/
double Object::triangleSolidAngle(const V3D &observer,
                                  const V3D &scaleFactor) const {
  //
  // Because the triangles from OC are not consistently ordered wrt their
  // outward normal
  // internal points give incorrect solid angle. Surface points are difficult to
  // get right
  // with the triangle based method. Hence catch these two (unlikely) cases.
  const BoundingBox &boundingBox = this->getBoundingBox();
  double sx = scaleFactor[0], sy = scaleFactor[1], sz = scaleFactor[2];
  V3D sObserver = observer;
  if (boundingBox.isNonNull() && boundingBox.isPointInside(sObserver)) {
    if (isValid(sObserver)) {
      if (isOnSide(sObserver))
        return (2.0 * M_PI);
      else
        return (4.0 * M_PI);
    }
  }

  int nTri = this->NumberOfTriangles();
  //
  // If triangulation is not available fall back to ray tracing method, unless
  // object is a standard shape, currently Cuboid or Sphere. Should add Cylinder
  // and Cone cases as well.
  //
  if (nTri == 0) {
    double height = 0.0, radius(0.0);
    int type;
    std::vector<Kernel::V3D> vectors;
    this->GetObjectGeom(type, vectors, radius, height);
    if (type == 1) {
      for (size_t i = 0; i < vectors.size(); i++)
        vectors[i] *= scaleFactor;
      return CuboidSolidAngle(observer, vectors);
    } else if (type == 2) // this is wrong for scaled objects
      return SphereSolidAngle(observer, vectors, radius);
    //
    // No special case, do the ray trace.
    //
    return rayTraceSolidAngle(observer); // so is this
  }
  double *vertices = this->getTriangleVertices();
  int *faces = this->getTriangleFaces();
  double sangle(0.0), sneg(0.0);
  for (int i = 0; i < nTri; i++) {
    int p1 = faces[i * 3], p2 = faces[i * 3 + 1], p3 = faces[i * 3 + 2];
    // would be more efficient to pre-multiply the vertices (copy of) by these
    // factors beforehand
    V3D vp1 = V3D(sx * vertices[3 * p1], sy * vertices[3 * p1 + 1],
                  sz * vertices[3 * p1 + 2]);
    V3D vp2 = V3D(sx * vertices[3 * p2], sy * vertices[3 * p2 + 1],
                  sz * vertices[3 * p2 + 2]);
    V3D vp3 = V3D(sx * vertices[3 * p3], sy * vertices[3 * p3 + 1],
                  sz * vertices[3 * p3 + 2]);
    double sa = getTriangleSolidAngle(vp1, vp2, vp3, observer);
    if (sa > 0.0)
      sangle += sa;
    else
      sneg += sa;
    //    std::cout << vp1 << vp2 << vp2;
  }
  return (0.5 * (sangle - sneg));
}
/**
* Get the solid angle of a sphere defined by centre and radius using an analytic
* formula
* @param observer :: point from which solid angle required
* @param vectors :: vector of V3D - the only value is the sphere centre
* @param radius :: sphere radius
* @return :: solid angle of sphere
*/
double Object::SphereSolidAngle(const V3D observer,
                                const std::vector<Kernel::V3D> vectors,
                                const double radius) const {
  const double distance = (observer - vectors[0]).norm();
  const double tol = Kernel::Tolerance;
  if (distance > radius + tol) {
    const double sa = 2.0 * M_PI * (1.0 - cos(asin(radius / distance)));
    return sa;
  } else if (distance < radius - tol)
    return 4.0 * M_PI; // internal point
  else
    return 2.0 * M_PI; // surface point
}

/**
* Get the solid angle of a cuboid defined by 4 points. Simple use of triangle
* based soild angle
* calculation. Should work for parallel-piped as well.
* @param observer :: point from which solid angle required
* @param vectors :: vector of V3D - the values are the 4 points used to defined
* the cuboid
* @return :: solid angle of cuboid - good accuracy
*/
double Object::CuboidSolidAngle(const V3D observer,
                                const std::vector<Kernel::V3D> vectors) const {
  // Build bounding points, then set up map of 12 bounding
  // triangles defining the 6 surfaces of the bounding box. Using a consistent
  // ordering of points the "away facing" triangles give -ve contributions to
  // the
  // solid angle and hence are ignored.
  std::vector<V3D> pts;
  pts.reserve(8);
  Kernel::V3D dx = vectors[1] - vectors[0];
  Kernel::V3D dz = vectors[3] - vectors[0];
  pts.push_back(vectors[2]);
  pts.push_back(vectors[2] + dx);
  pts.push_back(vectors[1]);
  pts.push_back(vectors[0]);
  pts.push_back(vectors[2] + dz);
  pts.push_back(vectors[2] + dz + dx);
  pts.push_back(vectors[1] + dz);
  pts.push_back(vectors[0] + dz);

  const unsigned int ntriangles(12);
  std::vector<std::vector<int>> triMap(ntriangles, std::vector<int>(3, 0));
  triMap[0][0] = 1;
  triMap[0][1] = 4;
  triMap[0][2] = 3;
  triMap[1][0] = 3;
  triMap[1][1] = 2;
  triMap[1][2] = 1;
  triMap[2][0] = 5;
  triMap[2][1] = 6;
  triMap[2][2] = 7;
  triMap[3][0] = 7;
  triMap[3][1] = 8;
  triMap[3][2] = 5;
  triMap[4][0] = 1;
  triMap[4][1] = 2;
  triMap[4][2] = 6;
  triMap[5][0] = 6;
  triMap[5][1] = 5;
  triMap[5][2] = 1;
  triMap[6][0] = 2;
  triMap[6][1] = 3;
  triMap[6][2] = 7;
  triMap[7][0] = 7;
  triMap[7][1] = 6;
  triMap[7][2] = 2;
  triMap[8][0] = 3;
  triMap[8][1] = 4;
  triMap[8][2] = 8;
  triMap[9][0] = 8;
  triMap[9][1] = 7;
  triMap[9][2] = 3;
  triMap[10][0] = 1;
  triMap[10][1] = 5;
  triMap[10][2] = 8;
  triMap[11][0] = 8;
  triMap[11][1] = 4;
  triMap[11][2] = 1;
  double sangle = 0.0;
  for (unsigned int i = 0; i < ntriangles; i++) {
    double sa =
        getTriangleSolidAngle(pts[triMap[i][0] - 1], pts[triMap[i][1] - 1],
                              pts[triMap[i][2] - 1], observer);
    if (sa > 0)
      sangle += sa;
  }
  return (sangle);
}

/**
* Calculate the solid angle for a cylinder using triangulation EXCLUDING the end
* caps.
* @param observer :: The observer's point
* @param centre :: The centre vector
* @param axis :: The axis vector
* @param radius :: The radius
* @param height :: The height
* @returns The solid angle value
*/
double Object::CylinderSolidAngle(const V3D &observer,
                                  const Mantid::Kernel::V3D &centre,
                                  const Mantid::Kernel::V3D &axis,
                                  const double radius,
                                  const double height) const {
  // The cylinder is triangulated along its axis EXCLUDING the end caps so that
  // stacked cylinders
  // give the correct value of solid angle (i.e shadowing is losely taken into
  // account by this
  // method)
  // Any triangle that has a normal facing away from the observer gives a
  // negative solid
  // angle and is excluded
  // For simplicity the triangulation points are constructed such that the cone
  // axis
  // points up the +Z axis and then rotated into their final position
  Kernel::V3D axis_direction = axis;
  axis_direction.normalize();
  // Required rotation
  Kernel::V3D initial_axis = Kernel::V3D(0., 0., 1.0);
  Kernel::V3D final_axis = axis_direction;
  Kernel::Quat transform(initial_axis, final_axis);

  // Do the base cap which is a point at the centre and nslices points around it
  const int nslices(Mantid::Geometry::Cylinder::g_nslices);
  const double angle_step = 2 * M_PI / (double)nslices;

  const int nstacks(Mantid::Geometry::Cylinder::g_nstacks);
  const double z_step = height / nstacks;
  double z0(0.0), z1(z_step);
  double solid_angle(0.0);
  for (int st = 1; st <= nstacks; ++st) {
    if (st == nstacks)
      z1 = height;

    for (int sl = 0; sl < nslices; ++sl) {
      double x = radius * std::cos(angle_step * sl);
      double y = radius * std::sin(angle_step * sl);
      Kernel::V3D pt1 = Kernel::V3D(x, y, z0);
      Kernel::V3D pt2 = Kernel::V3D(x, y, z1);
      int vertex = (sl + 1) % nslices;
      x = radius * std::cos(angle_step * vertex);
      y = radius * std::sin(angle_step * vertex);
      Kernel::V3D pt3 = Kernel::V3D(x, y, z0);
      Kernel::V3D pt4 = Kernel::V3D(x, y, z1);
      // Rotations
      transform.rotate(pt1);
      transform.rotate(pt3);
      transform.rotate(pt2);
      transform.rotate(pt4);

      pt1 += centre;
      pt2 += centre;
      pt3 += centre;
      pt4 += centre;

      double sa = getTriangleSolidAngle(pt1, pt4, pt3, observer);
      if (sa > 0.0) {
        solid_angle += sa;
      }
      sa = getTriangleSolidAngle(pt1, pt2, pt4, observer);
      if (sa > 0.0) {
        solid_angle += sa;
      }
    }
    z0 = z1;
    z1 += z_step;
  }

  return solid_angle;
}

/**
* Calculate the solid angle for a cone using triangulation.
* @param observer :: The observer's point
* @param centre :: The centre vector
* @param axis :: The axis vector
* @param radius :: The radius
* @param height :: The height
* @returns The solid angle value
*/
double Object::ConeSolidAngle(const V3D &observer,
                              const Mantid::Kernel::V3D &centre,
                              const Mantid::Kernel::V3D &axis,
                              const double radius, const double height) const {
  // The cone is broken down into three pieces and then in turn broken down into
  // triangles. Any triangle
  // that has a normal facing away from the observer gives a negative solid
  // angle and is excluded
  // For simplicity the triangulation points are constructed such that the cone
  // axis points up the +Z axis
  // and then rotated into their final position

  Kernel::V3D axis_direction = axis;
  axis_direction.normalize();
  // Required rotation
  Kernel::V3D initial_axis = Kernel::V3D(0., 0., 1.0);
  Kernel::V3D final_axis = axis_direction;
  Kernel::Quat transform(initial_axis, final_axis);

  // Do the base cap which is a point at the centre and nslices points around it
  const int nslices(Mantid::Geometry::Cone::g_nslices);
  const double angle_step = 2 * M_PI / (double)nslices;
  // Store the (x,y) points as they are used quite frequently
  double *cos_table = new double[nslices];
  double *sin_table = new double[nslices];

  double solid_angle(0.0);
  for (int sl = 0; sl < nslices; ++sl) {
    int vertex = sl;
    cos_table[vertex] = std::cos(angle_step * vertex);
    sin_table[vertex] = std::sin(angle_step * vertex);
    Kernel::V3D pt2 = Kernel::V3D(radius * cos_table[vertex],
                                  radius * sin_table[vertex], 0.0);

    if (sl < nslices - 1) {
      vertex = sl + 1;
      cos_table[vertex] = std::cos(angle_step * vertex);
      sin_table[vertex] = std::sin(angle_step * vertex);
    } else
      vertex = 0;

    Kernel::V3D pt3 = Kernel::V3D(radius * cos_table[vertex],
                                  radius * sin_table[vertex], 0.0);

    transform.rotate(pt2);
    transform.rotate(pt3);
    pt2 += centre;
    pt3 += centre;

    double sa = getTriangleSolidAngle(centre, pt2, pt3, observer);
    if (sa > 0.0) {
      solid_angle += sa;
    }
  }

  // Now the main section
  const int nstacks(Mantid::Geometry::Cone::g_nstacks);
  const double z_step = height / nstacks;
  const double r_step = height / nstacks;
  double z0(0.0), z1(z_step);
  double r0(radius), r1(r0 - r_step);

  for (int st = 1; st < nstacks; ++st) {
    if (st == nstacks)
      z1 = height;

    for (int sl = 0; sl < nslices; ++sl) {
      int vertex = sl;
      Kernel::V3D pt1 =
          Kernel::V3D(r0 * cos_table[vertex], r0 * sin_table[vertex], z0);
      if (sl < nslices - 1)
        vertex = sl + 1;
      else
        vertex = 0;
      Kernel::V3D pt3 =
          Kernel::V3D(r0 * cos_table[vertex], r0 * sin_table[vertex], z0);

      vertex = sl;
      Kernel::V3D pt2 =
          Kernel::V3D(r1 * cos_table[vertex], r1 * sin_table[vertex], z1);
      if (sl < nslices - 1)
        vertex = sl + 1;
      else
        vertex = 0;
      Kernel::V3D pt4 =
          Kernel::V3D(r1 * cos_table[vertex], r1 * sin_table[vertex], z1);
      // Rotations
      transform.rotate(pt1);
      transform.rotate(pt3);
      transform.rotate(pt2);
      transform.rotate(pt4);

      pt1 += centre;
      pt2 += centre;
      pt3 += centre;
      pt4 += centre;
      double sa = getTriangleSolidAngle(pt1, pt4, pt3, observer);
      if (sa > 0.0) {
        solid_angle += sa;
      }
      sa = getTriangleSolidAngle(pt1, pt2, pt4, observer);
      if (sa > 0.0) {
        solid_angle += sa;
      }
    }

    z0 = z1;
    r0 = r1;
    z1 += z_step;
    r1 -= r_step;
  }

  // Top section
  Kernel::V3D top_centre = Kernel::V3D(0.0, 0.0, height) + centre;
  transform.rotate(top_centre);
  top_centre += centre;

  for (int sl = 0; sl < nslices; ++sl) {
    int vertex = sl;
    Kernel::V3D pt2 =
        Kernel::V3D(r0 * cos_table[vertex], r0 * sin_table[vertex], height);

    if (sl < nslices - 1)
      vertex = sl + 1;
    else
      vertex = 0;
    Kernel::V3D pt3 =
        Kernel::V3D(r0 * cos_table[vertex], r0 * sin_table[vertex], height);

    // Rotate them to the correct axis orientation
    transform.rotate(pt2);
    transform.rotate(pt3);

    pt2 += centre;
    pt3 += centre;

    double sa = getTriangleSolidAngle(top_centre, pt3, pt2, observer);
    if (sa > 0.0) {
      solid_angle += sa;
    }
  }

  delete[] cos_table;
  delete[] sin_table;

  return solid_angle;
}

/**
* Returns an axis-aligned bounding box that will fit the shape
* @returns A shared pointer to a bounding box for this shape.
*/
const BoundingBox &Object::getBoundingBox() const {
  // This member function is const given that from a user's perspective it is
  // perfecly reasonable
  // to call it on a const object. We need to call a non-const function in
  // places to update the cache,
  // which is where the const_cast comes in to play.

  if (!TopRule) {
    // If we don't know the extent of the object, the bounding box doesn't mean
    // anything
    const_cast<Object *>(this)->setNullBoundingBox();
  } else if (m_boundingBox.isNull()) {
    // First up, construct the trial set of elements from the object's bounding
    // box
    const double big(1e10);
    double minX(-big), maxX(big), minY(-big), maxY(big), minZ(-big), maxZ(big);
    TopRule->getBoundingBox(maxX, maxY, maxZ, minX, minY, minZ);
    // If the object is not axis aligned then the bounding box will be poor, in
    // particular the minima are left at the trial start so return
    // a null object here
    if (minX < -100 || maxX > 100 || minY < -100 || maxY > 100 || minZ < -100 ||
        maxZ > 100) {
      // std::cerr << this->getName() << '(' << minX << ',' << maxX << ") (" <<
      // minY << ',' << maxY << ") (" << minZ << ',' << maxZ << ")\n";
      minX = -100;
      maxX = 100;
      minY = -100;
      maxY = 100;
      minZ = -100;
      maxZ = 100;
    }
    if (minX == -big || minY == -big || minZ == -big) {
      const_cast<Object *>(this)->setNullBoundingBox();
    } else {
      const_cast<Object *>(this)
          ->defineBoundingBox(maxX, maxY, maxZ, minX, minY, minZ);
    }
  } else {
  }

  return m_boundingBox;
}

/**
* Takes input axis aligned bounding box max and min points and calculates the
*bounding box for the
* object and returns them back in max and min points.
*
* @param xmax :: Maximum value for the bounding box in x direction
* @param ymax :: Maximum value for the bounding box in y direction
* @param zmax :: Maximum value for the bounding box in z direction
* @param xmin :: Minimum value for the bounding box in x direction
* @param ymin :: Minimum value for the bounding box in y direction
* @param zmin :: Minimum value for the bounding box in z direction
*/
void Object::getBoundingBox(double &xmax, double &ymax, double &zmax,
                            double &xmin, double &ymin, double &zmin) const {
  if (!TopRule) { // If no rule defined then return zero boundbing box
    xmax = ymax = zmax = xmin = ymin = zmin = 0.0;
    return;
  }
  if (!boolBounded) {
    AABBxMax = xmax;
    AABByMax = ymax;
    AABBzMax = zmax;
    AABBxMin = xmin;
    AABByMin = ymin;
    AABBzMin = zmin;
    TopRule->getBoundingBox(AABBxMax, AABByMax, AABBzMax, AABBxMin, AABByMin,
                            AABBzMin);
    if (AABBxMax >= xmax || AABBxMin <= xmin || AABByMax >= ymax ||
        AABByMin <= ymin || AABBzMax >= zmax || AABBzMin <= zmin)
      boolBounded = false;
    else
      boolBounded = true;
  }
  xmax = AABBxMax;
  ymax = AABByMax;
  zmax = AABBzMax;
  xmin = AABBxMin;
  ymin = AABByMin;
  zmin = AABBzMin;
}

/**
* Takes input axis aligned bounding box max and min points and stores these as
*the
* bounding box for the object. Can be used when getBoundingBox fails and bounds
*are
* known.
*
* @param xMax :: Maximum value for the bounding box in x direction
* @param yMax :: Maximum value for the bounding box in y direction
* @param zMax :: Maximum value for the bounding box in z direction
* @param xMin :: Minimum value for the bounding box in x direction
* @param yMin :: Minimum value for the bounding box in y direction
* @param zMin :: Minimum value for the bounding box in z direction
*/
void Object::defineBoundingBox(const double &xMax, const double &yMax,
                               const double &zMax, const double &xMin,
                               const double &yMin, const double &zMin) {
  BoundingBox::checkValid(xMax, yMax, zMax, xMin, yMin, zMin);

  AABBxMax = xMax;
  AABByMax = yMax;
  AABBzMax = zMax;
  AABBxMin = xMin;
  AABByMin = yMin;
  AABBzMin = zMin;
  boolBounded = true;

  PARALLEL_CRITICAL(defineBoundingBox) {
    m_boundingBox = BoundingBox(xMax, yMax, zMax, xMin, yMin, zMin);
  }
}

/**
* Set the bounding box to a null box
*/
void Object::setNullBoundingBox() { m_boundingBox = BoundingBox(); }

/**
Try to find a point that lies within (or on) the object
@param[out] point :: on exit set to the point value, if found
@return 1 if point found, 0 otherwise
*/
int Object::getPointInObject(Kernel::V3D &point) const {
  //
  // Simple method - check if origin in object, if not search directions along
  // axes. If that fails, try centre of boundingBox, and paths about there
  //
  Kernel::V3D testPt(0, 0, 0);
  if (searchForObject(testPt)) {
    point = testPt;
    return 1;
  }
  // Try centre of bounding box as initial guess, if we have one.
  const BoundingBox &boundingBox = getBoundingBox();
  if (boundingBox.isNonNull()) {
    testPt = boundingBox.centrePoint();
    if (searchForObject(testPt) > 0) {
      point = testPt;
      return 1;
    }
  }

  return 0;
}

/**
* Try to find a point that lies within (or on) the object, given a seed point
* @param point :: on entry the seed point, on exit point in object, if found
* @return 1 if point found, 0 otherwise
*/
int Object::searchForObject(Kernel::V3D &point) const {
  //
  // Method - check if point in object, if not search directions along
  // principle axes using interceptSurface
  //
  Kernel::V3D testPt;
  if (isValid(point))
    return 1;
  std::vector<Kernel::V3D> axes;
  axes.reserve(6);
  axes.push_back(Kernel::V3D(1, 0, 0));
  axes.push_back(Kernel::V3D(-1, 0, 0));
  axes.push_back(Kernel::V3D(0, 1, 0));
  axes.push_back(Kernel::V3D(0, -1, 0));
  axes.push_back(Kernel::V3D(0, 0, 1));
  axes.push_back(Kernel::V3D(0, 0, -1));
  std::vector<Kernel::V3D>::const_iterator dir;
  for (dir = axes.begin(); dir != axes.end(); ++dir) {
    Geometry::Track tr(point, (*dir));
    if (this->interceptSurface(tr) > 0) {
      point = tr.begin()->entryPoint;
      return 1;
    }
  }
  return 0;
}

/**
* Set the geometry handler for Object
* @param[in] h is pointer to the geometry handler. don't delete this pointer in
* the calling function.
*/
void Object::setGeometryHandler(boost::shared_ptr<GeometryHandler> h) {
  if (h == NULL)
    return;
  handle = h;
}

/**
* Draws the Object using geometry handler, If the handler is not set then this
* function does nothing.
*/
void Object::draw() const {
  if (handle == NULL)
    return;
  // Render the Object
  handle->Render();
}

/**
* Initializes/prepares the object to be rendered, this will generate geometry
* for object,
* If the handler is not set then this function does nothing.
*/
void Object::initDraw() const {
  if (handle == NULL)
    return;
  // Render the Object
  handle->Initialize();
}
/**
* set vtkGeometryCache writer
*/
void Object::setVtkGeometryCacheWriter(
    boost::shared_ptr<vtkGeometryCacheWriter> writer) {
  vtkCacheWriter = writer;
  updateGeometryHandler();
}

/**
* set vtkGeometryCache reader
*/
void Object::setVtkGeometryCacheReader(
    boost::shared_ptr<vtkGeometryCacheReader> reader) {
  vtkCacheReader = reader;
}

/**
* Returns the geometry handler
*/
boost::shared_ptr<GeometryHandler> Object::getGeometryHandler() {
  // Check if the geometry handler is upto dated with the cache, if not then
  // cache it now.
  return handle;
}

/**
* Updates the geometry handler if needed
*/
void Object::updateGeometryHandler() {
  if (bGeometryCaching)
    return;
  bGeometryCaching = true;
  // Check if the Geometry handler can be handled for cache
  if (handle == NULL)
    return;
  if (!handle->canTriangulate())
    return;
  // Check if the reader exist then read the cache
  if (vtkCacheReader.get() != NULL) {
    vtkCacheReader->readCacheForObject(this);
  }
  // Check if the writer exist then write the cache
  if (vtkCacheWriter.get() != NULL) {
    vtkCacheWriter->addObject(this);
  }
}

// Initialize Draw Object

/**
* get number of triangles
* @return the number of triangles
*/
int Object::NumberOfTriangles() const {
  if (handle == NULL)
    return 0;
  return handle->NumberOfTriangles();
}

/**
* get number of points
*/
int Object::NumberOfPoints() const {
  if (handle == NULL)
    return 0;
  return handle->NumberOfPoints();
}

/**
* get vertices
*/
double *Object::getTriangleVertices() const {
  if (handle == NULL)
    return NULL;
  return handle->getTriangleVertices();
}

/**
* get faces
*/
int *Object::getTriangleFaces() const {
  if (handle == NULL)
    return NULL;
  return handle->getTriangleFaces();
}

/**
* get info on standard shapes
*/
void Object::GetObjectGeom(int &type, std::vector<Kernel::V3D> &vectors,
                           double &myradius, double &myheight) const {
  type = 0;
  if (handle == NULL)
    return;
  handle->GetObjectGeom(type, vectors, myradius, myheight);
}

/** Getter for the shape xml
@return the shape xml.
*/
std::string Object::getShapeXML() const { return this->m_shapeXML; }

} // NAMESPACE Geometry
} // NAMESPACE Mantid
