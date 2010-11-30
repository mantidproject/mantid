#ifndef MD_DATAPOINT_H
#define MD_DATAPOINT_H

#include <stdint.h>
#include <vector> 
#include <string>
#include "MantidKernel/DllExport.h"

/**  path-through class which provide transformation of the data from pixel buffer format to the data fields format; 
*   
*
* Templated for efficiency as critical
*
* It is hard-coded in the class that the run indexes and the detector indexes are present in the data and are located 
* after two double sized image fields namely signal and error.  This can be modified in more regular fashion;
*
* CODED FOR LITTLE ENDIAN -> big endian is not checked and not supported at the moment
*

    @author Alex Buts, RAL ISIS
    @date 17/10/2010

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
namespace Mantid{
    namespace MDDataObjects{
      /** the names of the fields, present in the MDPixelDescription class and describe the format of the MDPixelDescriptions class */
      struct MDPointSignature{
        unsigned int NumDimensions;    //< number of dimensions in the dataset
        unsigned int NumRecDimensions; //< number of reciprocal dimensions among these dimensions
        unsigned int NumDataFields;    //< datafields -> signal and error for histohram data or 0 for event data
        unsigned int NumDimIDs;        //< some dimensions values are described by an ID, which allows to pick up the dimension value from look-up table
        unsigned int DimIDlength;      //< DimID are usually short words, the size of these words is expredssed in bytes (default 2)
        unsigned int DimLength;        //< Dimension fields can be float or double (default float 4)
        bool         DimFieldsPresent; //< dimension fields  can be absent; their values would be calculated dynamically on the basis of  DimIDs and look-up tables;
        bool         DataFieldsPresent; //< data fields can be absent for event data; false here actually means an event data
        unsigned int NumPixCompressionBits; //< Run number and detector number corresponding to the reciprocal dimensions of TOF experiments can be placed in single 32 bit word;
                                            // 10 in NumPixCompression means that it is pissible to have 2^10-1 (1023) different experiments and 2^22 detectors (4M) coded by this field 
                                            // 0 here should mean a class with even DimID fields -> not implemented;
                                            //TO DO: does this specialisation is practically usefull?  necessary?
        MDPointSignature():NumDimensions(4),NumRecDimensions(3),NumDataFields(2),NumDimIDs(3),DimIDlength(2),DimLength(4),
                           DimFieldsPresent(true),DataFieldsPresent(true),NumPixCompressionBits(10){}
      };

      /** Small helper class describing format of the MDDataPoint in a form, which can be conveniently stored on HDD or transferred between classes
       *  allowing to instanciatei proper version of MDDataPoint which does the job transforming data from and to the HDD format
       */
      class DLLExport MDPointDescription{
      public:
        MDPointDescription(const MDPointSignature &pixInfo,const std::vector<std::string> &dataTags);
        // use default tags;
        MDPointDescription(const MDPointSignature &pixInfo);
        // use defauld pixInfo and defailt tags
        MDPointDescription();

        MDPointSignature & PixInfo(){return PixDescriptor;}
        /// returns the column name, first come the names of dimensios (if any), data (if any) after that and indexes of dimensions to follow (these are always present)
        /// no way to idenfify what are dimensios, what are signals or what are the indexes except counting them and comparing with the numbers of dimensions, 
        /// signals and indexes from the MDPoindDescriptor
        std::string getColumnName(unsigned int nColumn)const{return dataTags.at(nColumn);}
        /// gets all column names together; see the getColumnName description
        std::vector<std::string> const getColumnNames(void) const{return dataTags;}
      protected:
        MDPointSignature PixDescriptor;
        /// the names (tags) of every dimension column and every data column; The former has to coinside (and would be obtained from) MDgeometryBasis, and first columns (if present) have to represent 
        /// reciprocal dimensions;
        std::vector<std::string> dataTags;
        void buildDefaultTags(const MDPointSignature &pixInfo);
      };

//********************************************************************************************************************************************************************
/** the class to work with the pixels buffer itself*/
 template<class T=float, class I=uint16_t>
 class MDDataPoint: public MDPointDescription
 {
  
 public:
   /**   the constructor which defines the size of the dataset, number of fields in this dataset and data location in memory
   *    this constructor should be used mainly for debugging and unit tests as relies on default column names only
   */
   MDDataPoint(char * buf, unsigned int nDims=4,unsigned int nData=2,unsigned int nIDfields=3):
   pDataBuffer(buf),
   MDPointDescription()
   {
     bool old_column_names(true);
     if(this->PixDescriptor.NumDimensions != nDims||this->PixDescriptor.NumDataFields!=nData||this->PixDescriptor.NumDimIDs!=nIDfields){
        old_column_names=false;
     }
     this->PixDescriptor.NumDimensions = nDims;
     this->PixDescriptor.NumDataFields = nData;
     this->PixDescriptor.NumDimIDs     = nIDfields;
     if(!old_column_names){
       this->buildDefaultTags(this->PixDescriptor);
     }
     this->buildPixel();
   }
 /**   main constructor which defines the size of the dataset, number of fields in this dataset and data location in memory and on hdd
   *   
   */
   MDDataPoint(char * buf, const MDPointDescription &pixSignature):
   pDataBuffer(buf),
   MDPointDescription(pixSignature)    
   {
     buildPixel();
   }
   
 public:
   // Accessors:;
   /// obtain the value of correspondent data field;
   T getDataField(unsigned int nField,size_t n_point)const{return *(reinterpret_cast<T *>     (pDataBuffer + n_point*MDPointStride+field_loc[nField]));}
   /// function returns signal
   double getSignal(size_t n_point)                  const{return *(reinterpret_cast<double *>(pDataBuffer + n_point*MDPointStride+pSignal));}
   /// function returns error
   double getError  (size_t n_point)                 const{return *(reinterpret_cast<double *>(pDataBuffer + n_point*MDPointStride +pError));}
   /// function returns and dimension index e.g. position of this dimension in some kind of look-up table
   I getIndex(unsigned int nf,size_t n_point)        const{return *(reinterpret_cast<I *>     (pDataBuffer + n_point*MDPointStride+field_loc[PixIndex+nf]));}

   // these two are coded for particular type of experiments -> number of runs (2^pix_id_shift-1) <~2^9, number of pixels <~ 2^(23)
   uint32_t getRunID(size_t n_point)                 const{return   RunIDMask&(*(reinterpret_cast<uint32_t *>(pDataBuffer+n_point*MDPointStride+pPixIndex)));}
   uint32_t getPixID(size_t n_point)                 const{return ((PixIDMask)&(*(reinterpret_cast<uint32_t *>(pDataBuffer+n_point*MDPointStride +pPixIndex)))>>pix_id_shift);}
   /// get size (in bytes) for the MDdataPoint;
   unsigned int sizeofMDDataPoint(void)              const{return MDPointStride;}
   /// returns the total number of data point fields as sum of all contributing fields, e.g. dimensions, datas and signals
   unsigned int getNumPointFields(void)              const{return n_dimensions+n_indFields+n_signals;}
   /// get the numbers of all contributing fields separately
   unsigned int getNumDimensions(void)              const{return n_dimensions;}
   unsigned int getNumSignals(void)                 const{return n_signals;}
   unsigned int getNumDimIndex(void)                const{return n_indFields;}
   //************************************************************************************************************************
   //Mutators
   /// copy pixel from the specified location among origin pixels to the specified location among target pixels
   void copyPixel(size_t iOrigin, char *targetBuff, size_t iTarget)const{
     memcpy(base+MDPointStride*iOrigin,targetBuff+MDPointStride*iTarget,MDPointStride);
   }
     
   ///function sets data from external source into MDDataPoint format and is specialized for Horace data e.g. expetcs no more than 2^pix_id_shift-1 runs and no more than 2^(32-pix_id_shift) pixels (unique detectors)
   // @param ind           - the location of the pixel in the MDDataPoints dataset
   // @param dim_fields    - the values of the dimension coordinates (may be absent)
   // @param Signal_fields - Signal and error for histogram data, absent for event data
   // @param iFields       - array of dimension ID in some look-up table; this function assumes that 2 first fields represent the detector location (detectorId and runID)
   void setData(unsigned int ind,T dim_fields[],double SignalFields[],int iFiels[]){
     unsigned int i,i0;

     char *const base = pDataBuffer+ind*MDPointStride;
     // copy dimension values (axis values)
     memcpy(base,dim_fields,sizeof(T)*n_dimensions);
     // copy signals
     i0 = n_dimensions;
     memcpy(base+field_loc[i0],SignalFields,sizeof(double)*n_signals);

     // this part is specialized for coding detectors from runID and dimID
     i0=PixIndex;
     // compress n_runs and n_pixels into single 32 bit word
     uint32_t *pBuf = (uint32_t *)pWorkingBuf;
     uint32_t  w1   = (uint32_t)iFiels[0];
     uint32_t  w2   = (uint32_t)iFiels[1];
     *pBuf = (w1&RunIDMask)|((w2<<pix_id_shift)&(~RunIDMask));

     // deal with remaining dimention indexes
     unsigned int ic = sizeof(uint32_t)/sizeof(I);
     if(ic==0)ic=1;
     for(i=2;i<n_indFields;i++){
       pWorkingBuf[ic]=(I)iFiels[i];
       ic++;
     }
     // copy indexes into data buffer;
      memcpy(base + field_loc[i0],pWorkingBuf,indexBufSize);

   }
   //
   ~MDDataPoint(){
     delete [] field_loc;
     delete [] pWorkingBuf;
   }
// 
 private:
   // numbers of different fields in the data array;
   unsigned int n_dimensions, //< number of dataset dimensions
                n_indFields,  //< number of integer identifiers (indexes) for the dimensions values
                n_signals;    // < number of signal fields
   // the location of the beginning of the data buffer
   char *const  pDataBuffer;
 /// the size of one data point in bytes;
   unsigned int MDPointStride;
   unsigned int *field_loc;

   unsigned int pSignal,pError,pPixIndex,PixIndex;
   // data to support packing pixID and RunId into one integer word -> shift is basis and other are derived;
   uint32_t pix_id_shift,PixIDMask,RunIDMask,indexBufSize;
   // auxiliary 
   I*           pWorkingBuf;
   std::vector<unsigned int> field_lengths;

 // the main constructor function;
   void buildPixel(void){
     unsigned int i;
     // set the length of main fields as the function of the template initialisation parameters
     this->PixInfo().DimLength = sizeof(T);
     this->PixInfo().DimIDlength=sizeof(I);

     n_dimensions = this->PixInfo().NumDimensions;
     n_indFields  = this->PixInfo().NumDimIDs;
     n_signals    = this->PixInfo().NumDataFields;
     pix_id_shift = this->PixInfo().NumPixCompressionBits;


     pWorkingBuf = new I[n_indFields];
     // arrange array for all data fields length-es which can be present in the data structure and set its initial values to sizeof(T);
      this->field_lengths.assign(n_dimensions+n_signals+n_indFields,sizeof(T));
   
      // set field sizes of signals to size of double;
      for(i=n_dimensions;i<n_dimensions+n_signals;i++){
        this->field_lengths[i]=sizeof(double);
      }
      // set field sizes of all integer indicators to sizeof(I); Some fields will be combined in a partucular way
      for(i=n_dimensions+n_signals;i<n_dimensions+n_signals+n_indFields;i++){
        this->field_lengths[i]=sizeof(I);
      }
      // specialisation : two first fields are packed into one 32 bit field
      this->field_lengths[n_dimensions+n_signals]  =2;
      this->field_lengths[n_dimensions+n_signals+1]=2;

     // allocate and calculate the field locations
     unsigned int n_fields      = n_dimensions+n_signals+n_indFields;
     PixIndex                   = n_dimensions+n_signals;
     field_loc                  = new unsigned int[n_fields];

     field_loc[0] = 0;
     for(i=1;i<n_fields;i++){
        field_loc[i]=field_loc[i-1]+field_lengths[i-1];
     }
   // set up specific pointers for fast accessors;
     MDPointStride= field_loc[n_fields-1]+field_lengths[n_fields-1];
     pSignal      = field_loc[n_dimensions];
     pError       = field_loc[n_dimensions+1];
     pPixIndex    = field_loc[PixIndex];

   // this defines PixID masks and RinID mask -- it is assumed that these data are present in all experiments and are packed into one 32bit word
     RunIDMask=0;
     for(i=0;i<pix_id_shift;i++){        RunIDMask=(RunIDMask<<1)|0x1;
     }
     PixIDMask=(~RunIDMask)>>pix_id_shift;
    // calculate the size of buffer for data indexes;
     indexBufSize = sizeof(uint32_t);
     if(n_indFields>2){
       indexBufSize+=sizeof(I)*(n_indFields-2);
     }

      
   }


};

}
}

#endif