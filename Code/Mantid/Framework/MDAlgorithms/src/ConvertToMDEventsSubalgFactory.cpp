#include "MantidMDAlgorithms/ConvertToMDEventsSubalgFactory.h"




//using namespace Mantid::MDAlgorithms::ConvertToMD;

namespace Mantid
{
namespace MDAlgorithms
{


/** Get access to a subalgorithm defined by its name
  * @param   AlgName -- symbolic name, which defines the algorithm
  * @returns    Pointer fo the algorithm, which do the conversion
  *
  * Thows invalid_agrument if subalgorithm, defined by the name is not exist (not initated);
*/
IConvertToMDEventsMethods * ConvertToMDEventsSubalgFactory::getAlg(const std::string &AlgName)
{
    auto algoIt  = alg_selector.find(AlgName);
    if (algoIt == alg_selector.end())
    {
        std::string Error = "Undefined subalgoritm: "+AlgName+" requested ";
        throw(std::invalid_argument(Error));
    }
    return algoIt->second;

}

ConvertToMDEventsSubalgFactory::ConvertToMDEventsSubalgFactory()
{
}
ConvertToMDEventsSubalgFactory::~ConvertToMDEventsSubalgFactory()
{
    // clear all initated subalgorithms
    auto it = alg_selector.begin();
    for(; it!=alg_selector.end();++it){
        delete it->second;  
    }
    alg_selector.clear();
}


//----------------------------------------------------------------------------------------------
// AUTOINSTANSIATION OF EXISTING CODE:
/** helper class to orginize metaloop instansiating various subalgorithms dealing with particular 
  * workspaces and implementing particular user requests */
template<ConvertToMD::QMode Q, size_t AlgoNum>
class LOOP_ALGS{
private:
    enum{
        CONV = AlgoNum%NConvUintsStates,                        // internal loop over conversion modes, the variable changes first
        MODE = (AlgoNum/NConvUintsStates)%ANY_Mode,             // level one loop over momentum conversion mode  
        SAMPL= ((AlgoNum/NConvUintsStates)/ANY_Mode)%NSampleTypes, //level two loop over crystal or powder sample
        WS   = (((AlgoNum/NConvUintsStates)/ANY_Mode)/NSampleTypes)%NInWSTypes //level three loop over ws type;
    
    };
  public:
    static inline void EXEC(const ConvertToMD::ConvertToMDEventsParams &AlgoKey,ConvertToMDEventsSubalgFactory *pH){
        // cast loop integers to proper enum type
        ConvertToMD::CnvrtUnits Conv = static_cast<ConvertToMD::CnvrtUnits>(CONV);
        ConvertToMD::AnalMode   Mode = static_cast<ConvertToMD::AnalMode>(MODE);
        ConvertToMD::SampleType Sampl= static_cast<ConvertToMD::SampleType>(SAMPL);
        ConvertToMD::InputWSType Ws  = static_cast<ConvertToMD::InputWSType>(WS);

        std::string  Key = AlgoKey.getAlgoID(Q,Mode,Conv,Ws,Sampl);
        pH->alg_selector.insert(std::pair<std::string, IConvertToMDEventsMethods *>(Key,
                (new ConvertToMDEventsWS<ConvertToMD::InputWSType(WS),Q,ConvertToMD::AnalMode(MODE),
                 ConvertToMD::CnvrtUnits(CONV),ConvertToMD::SampleType(SAMPL)>())));

/*#ifdef _DEBUG
            std::cout<<" Instansiating algorithm with ID: "<<Key<<std::endl;
#endif*/
            LOOP_ALGS<Q, AlgoNum-1>::EXEC(AlgoKey,pH);
    }
};

/** Templated metaloop specialization for noQ case */
template< size_t AlgoNum>
class LOOP_ALGS<ConvertToMD::NoQ,AlgoNum>{
private:
    enum{
        CONV = AlgoNum%ConvertToMD::NConvUintsStates,       // internal Loop over conversion modes, the variable changes first
        WS   = ((AlgoNum/ConvertToMD::NConvUintsStates))%ConvertToMD::NInWSTypes  //level one loop over ws type;
        //MODE => noQ -- no mode conversion ANY_Mode,     
    };
  public:
    static inline void EXEC(const ConvertToMD::ConvertToMDEventsParams &AlgoKey,ConvertToMDEventsSubalgFactory *pH){

      // cast loop integers to proper enum type
      ConvertToMD::CnvrtUnits Conv = static_cast<ConvertToMD::CnvrtUnits>(CONV);
      ConvertToMD::InputWSType Ws  = static_cast<ConvertToMD::InputWSType>(WS);

      std::string  Key   = AlgoKey.getAlgoID(ConvertToMD::NoQ,ConvertToMD::ANY_Mode,Conv,Ws,ConvertToMD::NSampleTypes);
      pH->alg_selector.insert(std::pair<std::string,IConvertToMDEventsMethods *>(Key,
         (new ConvertToMDEventsWS<ConvertToMD::InputWSType(WS),ConvertToMD::NoQ,ConvertToMD::ANY_Mode,ConvertToMD::CnvrtUnits(CONV),ConvertToMD::NSampleTypes>())));
           
//#ifdef _DEBUG
//            std::cout<<" Instantiating algorithm with ID: "<<Key<<std::endl;
//#endif

             LOOP_ALGS<ConvertToMD::NoQ,AlgoNum-1>::EXEC(AlgoKey,pH);
    }
};

//static_cast<size_t>(ANY_Mode*NConvUintsStates)
/** Q3d and ModQ metaloop terminator */
template<ConvertToMD::QMode Q >
class LOOP_ALGS<Q,0>{
  public:
      static inline void EXEC(const ConvertToMD::ConvertToMDEventsParams &AlgoKey,ConvertToMDEventsSubalgFactory *pH)
      {
          ConvertToMD::CnvrtUnits Conv = static_cast<ConvertToMD::CnvrtUnits>(0);
          ConvertToMD::AnalMode   Mode = static_cast<ConvertToMD::AnalMode>(0);
          ConvertToMD::SampleType Sampl= static_cast<ConvertToMD::SampleType>(0);
          ConvertToMD::InputWSType Ws  = static_cast<ConvertToMD::InputWSType>(0);

          std::string  Key   = AlgoKey.getAlgoID(Q,Mode,Conv,Ws,Sampl);
          pH->alg_selector.insert(std::pair<std::string,IConvertToMDEventsMethods *>(Key,
            (new ConvertToMDEventsWS<ConvertToMD::InputWSType(0),Q,ConvertToMD::AnalMode(0),ConvertToMD::CnvrtUnits(0),ConvertToMD::SampleType(0)>())));     
      } 
};

/** ANY_Mode (NoQ) metaloop terminator */
template<>
class LOOP_ALGS<ConvertToMD::NoQ,0>{
  public:
      static inline void EXEC(const ConvertToMD::ConvertToMDEventsParams &AlgoKey,ConvertToMDEventsSubalgFactory *pH)
      {     
          ConvertToMD::CnvrtUnits Conv = static_cast<ConvertToMD::CnvrtUnits>(0);
          ConvertToMD::InputWSType Ws  = static_cast<ConvertToMD::InputWSType>(0);

          std::string  Key   = AlgoKey.getAlgoID(ConvertToMD::NoQ,ConvertToMD::ANY_Mode,Conv,Ws,ConvertToMD::NSampleTypes);
          pH->alg_selector.insert(std::pair<std::string,IConvertToMDEventsMethods *>(Key,
          (new ConvertToMDEventsWS<ConvertToMD::InputWSType(0),ConvertToMD::NoQ,ConvertToMD::ANY_Mode,ConvertToMD::CnvrtUnits(0),ConvertToMD::NSampleTypes>())));

      } 
};

/** initiate the subalgorithms and made them availible for getAlg function
  * @param SubAlgDescriptor  -- the class which has infromation about existing subalgorthms and used to generate subalgorithms keys.
*/
void ConvertToMDEventsSubalgFactory::init(const ConvertToMD::ConvertToMDEventsParams &SubAlgDescriptor)
{
    if (alg_selector.empty()) // Instansiate the subalgorithms for different cases
    {    
    // NoQ --> any Analysis mode will do as it does not depend on it; we may want to convert unuts
    //    LOOP_ALGS<ConvertToMD::NoQ,ConvertToMD::NInWSTypes*ConvertToMD::NConvUintsStates>::EXEC(SubAlgDescriptor,this); 
    // MOD Q
    //   LOOP_ALGS<ConvertToMD::ModQ,ConvertToMD::NInWSTypes*ConvertToMD::NConvUintsStates*ConvertToMD::ANY_Mode*ConvertToMD::NSampleTypes>::EXEC(SubAlgDescriptor,this);
    // Q3D
//        LOOP_ALGS<ConvertToMD::Q3D,ConvertToMD::NInWSTypes*ConvertToMD::NConvUintsStates*ConvertToMD::ANY_Mode*ConvertToMD::NSampleTypes>::EXEC(SubAlgDescriptor,this);
    }

}

//
} // endnamespace MDAlgorithms
} // endnamespace Mantid