#ifndef MD_DATAPOINT_H
#define MD_DATAPOINT_H

#include "MDDataObjects/MDDataPointDescription.h"

/**  path-through classes which provide transformation of the data from pixel buffer format to the data fields format; 
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
namespace Mantid
{
namespace MDDataObjects
{
//********************************************************************************************************************************************************************
//********************************************************************************************************************************************************************



//********************************************************************************************************************************************************************
/** the class to work with the pixels buffer itself. Class works the MDpoints having equal index fields*/
 template<class T=float, class I=uint16_t,class S=double>
 class MDDataPointEven: public MDPointDescription
 {
  
 public:
  //------------------------------------------------------------------------------------------------
  /** The constructor which defines the size of the dataset, number of fields in
   *  this dataset and data location in memory.
   *
   *  This constructor should be used mainly for debugging and unit tests as relies on default column names only,
   */
   MDDataPointEven(char * buf, unsigned int nDims=4,unsigned int nData=2,unsigned int nIDfields=3):
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

 //------------------------------------------------------------------------------------------------
  /** Main constructor which defines the size of the dataset,
   * number of fields in this dataset and data location in memory and on hdd.
   *   
   */
   MDDataPointEven(char * buf, const MDPointDescription &pixSignature):
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
   S getSignal(size_t n_point)                       const{return *(reinterpret_cast<S *>(pDataBuffer + n_point*MDPointStride+pSignal));}
   /// function returns error
   S getError  (size_t n_point)                      const{return *(reinterpret_cast<S *>(pDataBuffer + n_point*MDPointStride +pError));}
   /// function returns and dimension index e.g. position of this dimension in some kind of look-up table
   I getIndex(unsigned int nf,size_t n_point)        const{return *(reinterpret_cast<I *>     (pDataBuffer + n_point*MDPointStride+field_loc[PixIndex+nf]));}
   /// get size (in bytes) for the MDdataPoint;
   unsigned int sizeofMDDataPoint(void)              const{return MDPointStride;}
   /// returns the total number of data point fields as sum of all contributing fields, e.g. dimensions, datas and signals
   unsigned int getNumPointFields(void)              const{return n_dimensions+n_indFields+n_signals;}
   /// get the numbers of all contributing fields separately
   unsigned int getNumDimensions(void)              const{return n_dimensions;}
   unsigned int getNumSignals(void)                 const{return n_signals;}
   unsigned int getNumDimIndex(void)                const{return n_indFields;}
   /// copy pixel from the specified location among origin pixels to the specified location among target pixels
   void copyPixel(size_t iOrigin, char *targetBuff, size_t iTarget)const{
     memcpy(targetBuff+MDPointStride*iTarget,pDataBuffer+MDPointStride*iOrigin,MDPointStride);
   }
   //************************************************************************************************************************
   //Mutators
     
  //------------------------------------------------------------------------------------------------
  /** function sets data from external source into MDDataPoint format 
   *
   *  @param ind           - the location of the pixel in the MDDataPoints dataset
   *  @param dim_fields    - the values of the dimension coordinates (may be absent)
   *  @param Signal_fields - Signal and error for histogram data, absent for event data
   *  @param iFields       - array of dimension ID in some look-up table; 
   */
   void setData(size_t ind,T dim_fields[],S SignalFields[],I iFields[]){
     unsigned int i0;

     char *const base = pDataBuffer+ind*MDPointStride;
     // copy dimension values (axis values)
     memcpy(base,dim_fields,sizeof(T)*n_dimensions);
     // copy signals
     i0 = n_dimensions;
     memcpy(base+field_loc[i0],SignalFields,sizeof(S)*n_signals);

     // this part is specialized for coding detectors from runID and dimID
     i0=PixIndex;
     memcpy(base+field_loc[i0],iFields,sizeof(I)*n_indFields);


   }
   //
   ~MDDataPointEven(){
     delete [] field_loc;
   }
// 
 protected:
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
   // auxiliary 
   std::vector<unsigned int> field_lengths;
 //*************************************************************************************************************
 /// the main constructor function;
   void buildPixel(void)
   {
     unsigned int i;
     // set the length of main fields as the function of the template initialisation parameters
     this->PixInfo().DimLength   = sizeof(T);
     this->PixInfo().DimIDlength = sizeof(I);
	 this->PixInfo().SignalLength= sizeof(S);

     n_dimensions = this->PixInfo().NumDimensions;
     n_indFields  = this->PixInfo().NumDimIDs;
     n_signals    = this->PixInfo().NumDataFields;
  

     // arrange array for all data fields length-es which can be present in the data structure and set its initial values to sizeof(T);
      this->field_lengths.assign(n_dimensions+n_signals+n_indFields,sizeof(T));
   
      // set field sizes of signals to size of double;
      for(i=n_dimensions;i<n_dimensions+n_signals;i++){
        this->field_lengths[i]=sizeof(S);
      }
      // set field sizes of all integer indicators to sizeof(I); Some fields will be combined in a partucular way
      for(i=n_dimensions+n_signals;i<n_dimensions+n_signals+n_indFields;i++){
        this->field_lengths[i]=sizeof(I);
      }
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


   }

};
//********************************************************************************************************************************************************************
/** the class to work with the pixels buffer itself*/
 template<class T=float, class I=uint16_t,class S=double>
 class MDDataPoint: public MDDataPointEven<T,I,S>
 {
  
 public:
  //------------------------------------------------------------------------------------------------
  /** The constructor which defines the size of the dataset, number of fields in
   *  this dataset and data location in memory.
   *
   *  This constructor should be used mainly for debugging and unit tests as relies on default column names only,
   */
   MDDataPoint(char * buf, unsigned int nDims=4,unsigned int nData=2,unsigned int nIDfields=3):
   MDDataPointEven(buf,nDims,nData,nIDfields),
   pix_id_shift(0),PixIDMask(0),RunIDMask(0),indexBufSize(0)
   {
     this->modifyPixel();
   }

 //------------------------------------------------------------------------------------------------
  /** Main constructor which defines the size of the dataset,
   * number of fields in this dataset and data location in memory and on hdd.
   *   
   */
   MDDataPoint(char * buf, const MDPointDescription &pixSignature):
   MDDataPointEven(buf,pixSignature),
   pix_id_shift(0),PixIDMask(0),RunIDMask(0),indexBufSize(0)
   {
	this->modifyPixel();
   }
   
 public:
   // Additional Accessors:;
   // these two are coded for particular type of experiments -> number of runs (2^pix_id_shift-1) <~2^9, number of pixels <~ 2^(23)
   uint32_t getRunID(size_t n_point)  const{return   RunIDMask&(*(reinterpret_cast<uint32_t *>(pDataBuffer+n_point*MDPointStride+pPixIndex)));}
   uint32_t getPixID(size_t n_point)  const{return ((PixIDMask)&(*(reinterpret_cast<uint32_t *>(pDataBuffer+n_point*MDPointStride +pPixIndex)))>>pix_id_shift);}
   //************************************************************************************************************************
   //Mutators
     
  //------------------------------------------------------------------------------------------------
  /** function sets data from external source into MDDataPoint format and is specialized
   *  for Horace data e.g. expetcs no more than 2^pix_id_shift-1 runs and no more than
   *  2^(32-pix_id_shift) pixels (unique detectors)
   *
   *  @param ind           - the location of the pixel in the MDDataPoints dataset
   *  @param dim_fields    - the values of the dimension coordinates (may be absent)
   *  @param Signal_fields - Signal and error for histogram data, absent for event data
   *  @param iFields       - array of dimension ID in some look-up table; this function assumes that 2 first 
   *                         fields represent the detector location (detectorId and runID)
   */
   void setData(unsigned int ind,T dim_fields[],S SignalFields[],int iFields[]){
     unsigned int i,i0;

     char *const base = pDataBuffer+ind*MDPointStride;
     // copy dimension values (axis values)
     memcpy(base,dim_fields,sizeof(T)*n_dimensions);
     // copy signals
     i0 = n_dimensions;
     memcpy(base+field_loc[i0],SignalFields,sizeof(S)*n_signals);

     // this part is specialized for coding detectors from runID and dimID
     i0=PixIndex;
     // compress n_runs and n_pixels into single 32 bit word
     uint32_t *pBuf = (uint32_t *)pWorkingBuf;
     uint32_t  w1   = (uint32_t)iFields[0];
     uint32_t  w2   = (uint32_t)iFields[1];
     *pBuf = (w1&RunIDMask)|((w2<<pix_id_shift)&(~RunIDMask));

     // deal with remaining dimention indexes
     unsigned int ic = sizeof(uint32_t)/sizeof(I);
     if(ic==0)ic=1;
     for(i=2;i<n_indFields;i++){
       pWorkingBuf[ic]=(I)iFields[i];
       ic++;
     }
     // copy indexes into data buffer;
      memcpy(base + field_loc[i0],pWorkingBuf,indexBufSize);

   }
   /// modified version of above, used when signal and dimension fields are the same width
  void setData(unsigned int ind,T dim_sig_fields[],int iFields[]){
     unsigned int i,i0;

     char *const base = pDataBuffer+ind*MDPointStride;
     // copy dimension values (axis values) and short signals
     memcpy(base,dim_sig_fields,sizeof(T)*(n_dimensions+n_signals));
   
     // this part is specialized for coding detectors from runID and dimID
     i0=PixIndex;
     // compress n_runs and n_pixels into single 32 bit word
     uint32_t *pBuf = (uint32_t *)pWorkingBuf;
     uint32_t  w1   = (uint32_t)iFields[0];
     uint32_t  w2   = (uint32_t)iFields[1];
     *pBuf = (w1&RunIDMask)|((w2<<pix_id_shift)&(~RunIDMask));

     // deal with remaining dimention indexes
     unsigned int ic = sizeof(uint32_t)/sizeof(I);
     if(ic==0)ic=1;
     for(i=2;i<n_indFields;i++){
       pWorkingBuf[ic]=(I)iFields[i];
       ic++;
     }
     // copy indexes into data buffer;
      memcpy(base + field_loc[i0],pWorkingBuf,indexBufSize);

   }
   ~MDDataPoint(){
     delete [] pWorkingBuf;
   }
// 
 private:
   // data to support packing pixID and RunId into one integer word -> shift is basis and other are derived;
   uint32_t pix_id_shift,PixIDMask,RunIDMask,indexBufSize;
   // auxiliary 
   I*           pWorkingBuf;
 //*************************************************************************************************************
 /// the overloaded constructor function, modifies the 
   void modifyPixel(void)
   {
     unsigned int i;

     pix_id_shift= this->PixDescriptor.NumPixCompressionBits;

     pWorkingBuf = new I[n_indFields];
  
     // specialisation : two first fields are packed into one 32 bit field
     this->field_lengths[n_dimensions+n_signals]  =2;
     this->field_lengths[n_dimensions+n_signals+1]=2;
	 //
     unsigned int n_fields      = n_dimensions+n_signals+n_indFields;

     // recalculate new field length
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
