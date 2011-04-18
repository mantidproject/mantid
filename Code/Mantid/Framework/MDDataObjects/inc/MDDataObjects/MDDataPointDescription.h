#ifndef MD_DATAPOINT_DESCRIPTION_H
#define MD_DATAPOINT_DESCRIPTION_H

#include "MantidKernel/System.h"
#include <vector> 
//#include <string>
#include <cstring>
#include <stdexcept>
#include "MantidKernel/DllExport.h"

/**  the description for classes, which process the MDDataPoints packed into data buffer
*    The class describes the location and srtucre of classes, which process this data buffer
*
*

    @author Alex Buts, RAL ISIS
    @date 10/12/2010

    Copyright &copy; 2007-10 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

    This file is part of Mantid.

    Mantid is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    Mantid is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
    
    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>.
      Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
namespace Mantid
{
namespace MDDataObjects
{
//********************************************************************************************************************************************************************
//********************************************************************************************************************************************************************

/** the names of the fields, present in the MDPixelDescription class 
  * and describe the format of the MDPixelDescriptions class */
struct MDPointStructure{
  unsigned int NumDimensions;    //< number of dimensions in the dataset
  unsigned int NumRecDimensions; //< number of reciprocal dimensions among these dimensions
  unsigned int NumDataFields;    //< datafields -> signal and error for histohram data or 0 for event data
  unsigned int NumDimIDs;        //< some dimensions values are described by an ID, which allows to pick up the dimension value from look-up table
  unsigned int DimIDlength;      //< DimID are usually short words, the size of these words is expredssed in bytes (default 2)
  unsigned int SignalLength;       //< Signals are often doubles
  unsigned int DimLength;        //< Dimension fields can be float or double (default float 4)
  bool         DimFieldsPresent; //< dimension fields  can be absent; their values would be calculated dynamically on the basis of  DimIDs and look-up tables;
  bool         DataFieldsPresent; //< data fields can be absent for event data; false here actually means an event data
  unsigned int NumPixCompressionBits; //< Run number and detector number corresponding to the reciprocal dimensions of TOF experiments can be placed in single 32 bit word;
                                   // 10 in NumPixCompression means that it is pissible to have 2^10-1 (1023) different experiments and 2^22 detectors (4M) coded by this field 
                                  // 0 here should mean a class with even DimID fields -> implemented;
                                 //TO DO: does this specialisation is practically usefull?  necessary?
  MDPointStructure():NumDimensions(4),NumRecDimensions(3),NumDataFields(2),NumDimIDs(3),
	                       SignalLength(8),DimIDlength(2),DimLength(4),
                           DimFieldsPresent(true),DataFieldsPresent(true),NumPixCompressionBits(10){}
};

//********************************************************************************************************************************************************************
//********************************************************************************************************************************************************************
/** Small helper class describing format of the MDDataPoint in a form, 
 *  which can be conveniently stored on HDD or transferred between classes
 *  allowing to instanciate  proper version of MDDataPoint which does the
 * job transforming data from and to the HDD format
 */
class DLLExport MDPointDescription
{
public:
MDPointDescription(const MDPointStructure &pixInfo,const std::vector<std::string> &dataTags);
// use default tags;
MDPointDescription(const MDPointStructure &pixInfo);
// use defauld pixInfo and defailt tags
MDPointDescription();
		           
MDPointStructure & PixInfo(){return PixDescriptor;}
 /** Returns the column name, first come the names of dimensions (if any), data
   * (if any) after that and indexes of dimensions to follow (these are always present)
   *  no way to idenfify what are dimensios, what are signals or what are
   *  the indexes except counting them and comparing with the numbers of dimensions,
   *  signals and indexes from the MDPoindDescriptor
   */
std::string getColumnName(unsigned int nColumn)const{return dataIDs.at(nColumn);}

 /// gets all column names together; see the getColumnName description
 std::vector<std::string> getColumnNames(void) const{return dataIDs;}
 /// function returns the part of the colum-names which corresponds to the dimensions information;
 std::vector<std::string> getDimensionsID(void)const;

 /// returns the size of the described MDDataPoint in bytes
 unsigned int sizeofMDDPoint(void)const;
protected:
        MDPointStructure PixDescriptor;
  /** The names (tags) of every dimension column and every data column;
   * The former has to coincide (and would be obtained from) MDgeometryBasis,
   * and first columns (if present) have to represent
   * reciprocal dimensions;
   */
   std::vector<std::string> dataIDs;

   void buildDefaultIDs(const MDPointStructure &pixInfo);
};
} // endnamespaces;
}
#endif
