#ifndef BnId_h
#define BnId_h 

namespace Mantid
{

namespace Geometry
{
/*!
  \class  BnId
  \brief Tri-state variable 
  \author S. Ansell
  \date April 2005
  \version 1.0

  
  This class holds a tri-state variable 
  of -1 (false) 0 (not-important) 1 (true) against
  each of the possible input desisions. It has
  arbiatary lenght (unlike using a long integer)
  \todo Could be improved by using a set of 
  unsigned integers. 

*/

class BnId
{
  /// Output Friend 
  friend std::ostream& operator<<(std::ostream&,const BnId&); 

 private:

  static Logger& PLog;           ///< The official logger

  int size;            ///< number of variables
  int PI;              ///< Prime Implicant
  int Tnum;            ///< True number (1 in Tval)
  int Znum;            ///< Zero number (0 in Tval)
  std::vector<int> Tval;   ///< Truth values

  void setCounters();    ///< Calculates Tnum and Znum

 public:
  
  BnId();
  BnId(const int,unsigned int);
  BnId(const BnId&);
  BnId& operator=(const BnId&);
  ~BnId();

  int operator==(const BnId&) const;
  int operator<(const BnId&) const;  
  int operator>(const BnId&) const;  
  int operator[](const int) const;   
  int operator++(int);
  int operator++();         
  int operator--(int);      
  int operator--();         
  int equivalent(const BnId&) const;
  void reverse();                     ///< Swap -1 to 1 and leave the zeros

  int PIstatus() const { return PI; }     ///< PI accessor
  void setPI(const int A) { PI=A; }       ///< PI accessor
  int intValue() const;                   ///< Integer from binary expression
  std::pair<int,BnId> 
    makeCombination(const BnId&) const;   ///< Combination operator
  std::string display() const;            ///< Get output string

  /// Total requiring expression
  int expressCount() const { return size-Znum; } 
  int Size() const { return size; }                      ///< returns number of variables / size          
  void mapState(const std::vector<int>&,std::map<int,int>&) const;
};

std::ostream& operator<<(std::ostream&,const BnId&);

}   // NAMESPACE 

}  // NAMESPACE Mantid

#endif
