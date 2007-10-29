#ifndef SimSupport_h
#define SimSupport_h


namespace SimSupport
{

  void printFactors(const SimControl&,const SimControl&,
		    const SimControl&,Work);
};


void printFactors(const SimControl& Sample,
		  const SimControl& Can,
		  const SimControl& Van)
{
  const std::vector<double>& Angle=Sample.getAngle();
  std::vector<double> Ivv=Van.getSample();
  std::vector<double> Mvv=Van.getMult();       
  std::vector<double> Icc=Can.getSample();         // I_{c,c}
  std::vector<double> Issc=Sample.getSample();  // I_{s,sc}
  std::vector<double> Icsc=Sample.getCan();   // I_{c,sc}
  const int size(Angle.size());

  for(int i=0;i<size;i++)
    {
      const double Cscale=Icsc[i]/Icc[i];
       
      
    }
  
  
  return;
}
