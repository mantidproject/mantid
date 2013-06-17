"""*WIKI* 
== Algorithm Reference Discussion ==

Looking at [https://kur.web.psi.ch/sans1/manuals/sas_manual.pdf Computing guide for Small Angle Scattering Experiments] by Ghosh, Egelhaaf & Rennie, we see that for scattering at larger angles the transmission should be modified due to the longer path length after the scattering event.

The longer path length after scattering will also slightly increase the probability of a second scattering event, but this is not dealt with here.
 
If our on-axis transmission is <math>T_0</math> through a sample of thickness <math>d</math>, then the transmission at some other thickness <math>x</math> is <math>\exp(-\mu x)</math> where attenuation coefficient <math>\mu = -\ln( \frac{T_0}{d})</math>.


If a neutron scatters at angle <math>2\theta</math> at distance <math>x</math> into the sample, its total transmission is then:


<math>
T^{''} = \exp(-\mu x) \exp( \frac{-\mu(d-x)}{\cos(2\theta)})
</math>


<math>T^{''}</math> should be integrated and averaged between <math>x = 0</math> and <math>x = d</math>.

 
Hammouda, gives an approximate result for the integral, see page 208 of [[http://www.ncnr.nist.gov/staff/hammouda/the_SANS_toolbox.pdf SANS toolbox]]:

<math>
T^{'} = \frac{T_0(T_0^A - 1)}{A \ln(T_0)}
</math>

For:

<math>
A = \frac{1}{\cos(2\theta)} - 1 
</math>
   
For example if  <math>T_0 = 0.2</math> and <math>2\theta = 40</math>  then  <math>T^{'} = 0.158</math>, a shift of <math>~20</math>% of the SANS curve. Note that the result is independent of sample thickness. 

<math>T_0</math> is a function of neutron wavelength, whilst <math>A</math> is a function of detector pixel location.

The output of this algorithm is:

<math>OutputWorkspace = \frac{T^'}{T_0}</math>

=== Error Propagation ===

The error propagation follows this formula:

  <math>OutputWorkspace_{error} = \frac{T_{0E} ^A - 1}{A\ln(T_0E)}</math>

Which means, that we do not consider the error in the definition of the <math>2\theta</math> (the parameter A)


== Enabling Wide Angle Correction for Reduction of SANS ISIS ==

To enable the Wide Angle correction use the User File settings: 

 SAMPLE/PATH/ON

More information on: [[SANS_User_File_Commands#SAMPLE]]

== SANS ISIS Reduction ==

The output of SANSWideAngleCorrection is used as WavePixelAdj parameter at [[Q1D]].

== Wide Angle Correction and the SANS Reduction ==

The equation for the reduction is (see [[Q1D]])

<math>P_I(Q) = \frac{\sum_{\{i, j, n\} \supset \{I\}} S(i,j,n)}{\sum_{\{i, j, n\} \supset \{I\}}M(n)\eta(n)T(n)\Omega_{i j}F_{i j}}</math> 

But, <math>T(n)</math> is not really <math>T(n)</math>, because of the wide angles, it is now <math>T(n,theta)</math> or <math>T(n,i,j)</math>.

So, we decided to have a new factor that changes this equation to: 

<math>P_I(Q) = \frac{\sum_{\{i, j, n\} \supset \{I\}} S(i,j,n)}{\sum_{\{i, j, n\} \supset \{I\}}M(n)\eta(n)T(n)\Omega_{i j}F_{i j}Corr(i,j,n)}</math>

Where Corr (Correction factor) in this case will be:

<math>
Corr = \frac{T_0^A - 1}{A \ln(T_0)}
</math>

Which is the OutputWorkspace of SANSWideAngleCorrection.

This parameter enters inside [[Q1D]] as WavePixelAdj. But, this is all done for you inside the Reduction Script.

== Comparison with Wide Angle Correction at SNS ==

The transmission correction applied at SNS is described [http://www.mantidproject.org/HFIR_SANS#Transmission_correction here], and it is applied through the [[ApplyTransmissionCorrection]] algorithm. The correction applied there is an approximation for the same equations described here. The picture above compare their results 

[[File:SNS_ISIS_WideAngleCorrections.png]]

Note a difference among them is when they are applied. At SNS, the correction is applied before averaging the counters per bin inside [[Q1D]] algorithm, while at ISIS, it is used after, inside the [[Q1D]] algorithm, for the division of the counters per bin normalized by the transmission counters.


== References == 

Annie Brulet et al. - Improvement of data treatment in SANS - J. Appl. Cryst. (2007). 40

Ghosh, Egelhaaf & Rennie - Computing guide for Small Angle Scattering Experiments

*WIKI*"""

"""*WIKI_USAGE*

'''NB''': This algorithm is not intended to be called and used as a standalone Mantid algorithm because actual transmission values are <i>not</i> passed to the reduction algorithm [[Q1D]].

*WIKI_USAGE*"""


from mantid.api import *
from mantid.kernel import *
import os
import sys
import numpy as np

class SANSWideAngleCorrection(PythonAlgorithm):
    """ Calculate the Wide Angle correction for SANS transmissions.
    """
   
    def category(self):
        return "CorrectionFunctions\\TransmissionCorrections"

    def name(self):
        return "SANSWideAngleCorrection"

    def PyInit(self):
	self.declareProperty(MatrixWorkspaceProperty("SampleData", "", direction = Direction.Input), 
		"A workspace cropped to the detector to be reduced (the SAME as the input to [[Q1D]]); used to verify the solid angle. The workspace is not modified, just inspected.")
        self.declareProperty(MatrixWorkspaceProperty("TransmissionData","",direction=Direction.Input),
                                      "The transmission data calculated, referred to as <math>T_0</math> in equations in discussion section")
        self.declareProperty(MatrixWorkspaceProperty("OutputWorkspace","",direction=Direction.Output),
                                      "The transmission corrected SANS data, normalised (divided) by <math>T_0</math>, see discussion section")
	self.setWikiSummary("Calculate the Wide Angle correction for SANS transmissions.")
      
    def PyExec(self):
        """ Main body of execution
        """
        # 1. get parameter values
        wd = self.getProperty("SampleData").value
        trans = self.getProperty("TransmissionData").value
        
        # check transmission input workspace
        if (len(trans.dataX(0)) != len(wd.dataX(0))):
            raise RuntimeError("Uncompatible sizes. Transmission must have the same bins of sample values")
        if min(trans.dataY(0)) < 0:
            raise RuntimeError("Invalid workspace for transmission, it does not accept negative values.")
        
        #check input sample has associated instrument
        if not wd.getInstrument().getSample():
            raise RuntimeError("You can not apply this correction for workspace not associated to instrument")

        #initialization of progress bar: 
        #
        endrange = 5+wd.getNumberHistograms()
        prog_reporter = Progress(self,start=0.0,end=1.0,nreports=endrange)

        # creating the workspace for the output
        prog_reporter.reportIncrement(5,"Preparing Wide Angle")
        trans_wc = WorkspaceFactory.create(wd)

        # get the values of transmission (data, error, binning)
        to = np.array(trans.dataY(0))
        to_e = np.array(trans.dataE(0))        
        x_bins = np.array(wd.dataX(0))

        # get the position of the sample and the instrument
        sample_pos = wd.getInstrument().getSample().getPos()
        inst_pos = sample_pos - wd.getInstrument().getSource().getPos()

        # for each spectrum (i,j) calculate the correction factor for the transmission.
        for i in range(wd.getNumberHistograms()):
            try:
                prog_reporter.report("Correcting Wide Angle")
                # calculation of A
                twoTheta = wd.getDetector(i).getTwoTheta(sample_pos, inst_pos)
                A = 1/np.cos(twoTheta) - 1
                # calculation of factor for transmission            
                to_l = (np.power(to,A)-1)/(np.log(to)*A)
                to_err_l = (np.power(to_e, A) - 1)/ (np.log(to_e) * A)
                # applying the data to the workspace
                trans_wc.setY(i,to_l)
                trans_wc.setE(i,to_err_l)
                trans_wc.setX(i,x_bins)
            except:
                self.getLogger().warning("WideAngleCorrection error: " + str(sys.exc_info()[2]))

        self.setProperty("OutputWorkspace", trans_wc)
        
#############################################################################################
AlgorithmFactory.subscribe(SANSWideAngleCorrection)        

