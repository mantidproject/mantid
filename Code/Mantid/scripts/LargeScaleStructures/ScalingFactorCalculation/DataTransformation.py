from MantidFramework import *
from mantidsimple import *
#from numpy import zeros
from pylab import *
import time

class Setup:
        
    nexus_path = '/mnt/hgfs/j35/'
    list_runs = {'D0': 55889,    #0
             'AiD0': 55890,  #1
             'AiD1': 55891,  #2
             'AiD2': 55892,  #3
             'AiD3': 55893,  #4
             'AiD4': 55894,  #5
             'AiD5': 55895,  #6
             'AiiAiD5': 55896,  #7
             'AiiAiD6': 55897,  #8
             'AiiAiD7': 55898,  #9
             'AiiiAiiAiD7': 55899,  #10
             'AiiiAiiAiD8': 55900,  #11
             'AivAiiiAiiAiD8': 55901,  #12
             'AivAiiiAiiAiD9': 55902}  #13
    pre = 'REF_L_'
    post = '_event.nxs'

    tof_min = 10000  #microS
    tof_max = 21600  #microS

    #range of x pixel to use in the X integration (we found out that there is a frame effect that introduces noise)
    x_pixel_min = 90
    x_pixel_max = 190

    #from,width,to in microS
    rebin_parameters = '0,200,200000'  

    #turn on or off the plots
    bPlot = False
    bFittingPlot = True
    
    #size of detector
    alpha_pixel_nbr = 256 
    beta_pixel_nbr = 304  #will be integrated over this dimension
    
    y_axis_ratio = None
    y_axis_error_ratio = None
    x_axis_ratio = None
    
    #fitting parameters  y=a+b*x
    a = None
    b = None
    
    def __init__(self):
        self.x_axis_ratio = None
        self.y_axis_ratio = None
        self.y_axis_error_ratio = None
    
    def setNexusPath(self, nexus_path=''):
        """
        Sets the full path to the folder where the data are located
        """
        if nexus_path == '':
            return
        
        self.nexus_path = nexus_path
        
    def setListRuns(self, list_runs):
        """
        Define the dictionnary 
        """
        self.list_runs = list_runs
    
    def setXPixelRange(self, pixel_range=[-1,-1]):
        """
        This will set the X range of pixels to avoid the frame effect we found by plotting Y vs X 
        integrated over the full range of TOF
        """
        if size(pixel_range) != 2:
            return
        
        if (pixel_range[0] != -1):
            self.x_pixel_min = pixel_range[0]
            
        if (pixel_range[1] != -1):
            self.x_pixel_max = pixel_range[1]
            
    def setTofRange(self, TOF_range=[-1,-1]):
        """
        Select the range of TOF in microS for the calculation
        """
        if size(TOF_range) != 2:
            return
        
        if (TOF_range[0] != -1):
            self.tof_min = TOF_range[0]
            
        if (TOF_range[1] != -1):
            self.tof_max = TOF_range[1]
            
    def setPeakPixelRange(self, peak_range=[-1,-1]):
        """
        Select the range of peak pixels
        """
        if size(peak_range) != 2:
            return
        
        if peak_range[0] != -1:
            self.peak_pixel_min = peak_range[0]
        
        if peak_range[1] != -1:
            self.peak_pixel_max = peak_range[1]
            
    def setBackPixelRange(self, back_range=[-1,-1]):
        """
        Select the range of background pixels
        """ 
        if size(back_range) != 2:
            return
        
        if back_range[0] != -1:
            self.back_pixel_min = back_range[0]
            
        if back_range[1] != -1:
            self.back_pixel_max = back_range[1]
            
    def setRebinParameters(self, para=None):
        """
        Set the min, max and width parameters for the rebinning
        'min,width,max'
        """
        if para is None:
            return   
            
        self.rebin_parameters = para    
            
    def __mul__(self, other):
        """
        operator * between two instances of the class
        """
        
        product = Setup()
        
        product.id_numerator = self.id_numerator + '*' + other.id_numerator
        product.id_denominator = self.id_denominator + '*' + other.id_denominator
        
        product.x_axis_ratio = self.x_axis_ratio
        product.y_axis_ratio = self.y_axis_ratio * other.y_axis_ratio
        product.y_axis_error_ratio = sqrt((other.y_axis_ratio*self.y_axis_error_ratio)**2 +
                                          (other.y_axis_error_ratio*self.y_axis_ratio)**2)
        return product
    
    def fit(self):
        """
        This is going to fit the counts_vs_tof with a linear expression and return the a and
        b coefficients (y=a+bx)
        """
        CreateWorkspace('DataToFit', 
                        DataX=self.x_axis_ratio, 
                        DataY=self.y_axis_ratio,
                        DataE=self.y_axis_error_ratio,
                        Nspec=1)
        Fit(InputWorkspace='DataToFit',
            Function="name=UserFunction, Formula=a+b*x, a=1, b=2", 
            Output='Res')
        res = mtd['Res_Parameters']
        self.a = res.getDouble("Value",0)
        self.b = res.getDouble("Value",1)
        
class CalculateAD(Setup):      
    
    id_numerator = None  #ex: AiD0
    id_denominator = None   #ex: AiD1

    _y_axis_numerator = None
    _y_axis_error_numerator = None
    _y_axis_denominator = None
    _y_axis_error_denominator = None
    _x_axis = None     
         
    #define the peak region      
    n_peak_pixel_min = 130
    n_peak_pixel_max = 135
    d_peak_pixel_min = 130
    d_peak_pixel_max = 135

    peak_pixel_min = None
    peak_pixel_max = None
    back_pixel_min = None
    back_pixel_max = None

    #define the background range used in the background subtraction
    n_back_pixel_min = 125
    n_back_pixel_max = 140
    d_back_pixel_min = 125
    d_back_pixel_max = 140

    _y_axis_ratio = None
    _y_axis_error_ratio = None
    _x_axis_ratio = None
            
    def __init__(self, id_numerator=None, id_denominator=None):
        
        self.id_numerator = id_numerator
        self.id_denominator = id_denominator

        self.x_axis_ratio = None
        self.y_axis_error_ratio = None
        self.y_axis_ratio = None
        
        #Launch the calculation
        #self.run()

    def setNumerator(self, minPeak=None, maxPeak=None, minBack=None, maxBack=None):
        if minPeak is not None:
            self.n_peak_pixel_min = minPeak
        if maxPeak is not None:
            self.n_peak_pixel_max = maxPeak
        if minBack is not None:
            self.n_back_pixel_min = minBack
        if maxBack is not None:
            self.n_back_pixel_max = maxBack
        
    def setDenominator(self, minPeak=None, maxPeak=None, minBack=None, maxBack=None):
        if minPeak is not None:
            self.d_peak_pixel_min = minPeak
        if maxPeak is not None:
            self.d_peak_pixel_max = maxPeak
        if minBack is not None:
            self.d_back_pixel_min = minBack
        if maxBack is not None:
            self.d_back_pixel_max = maxBack
        
    def run(self):
        """
        Perform the calculation

        """
        
        #perform calculation for numerator
        self._calculateFinalYAxis(bNumerator=True)
        
        #perform calculation for denominator
        self._calculateFinalYAxis(bNumerator=False)
        
        #calculate y_axis of numerator/denominator
#        self._x_axis_ratio = self._x_axis
        self.y_axis_ratio = self._y_axis_numerator/self._y_axis_denominator
        self.y_axis_error_ratio = ((self._y_axis_error_numerator/self._y_axis_numerator)**2 +
                                    (self._y_axis_error_denominator/self._y_axis_denominator)**2)
        self.y_axis_error_ratio = sqrt(self.y_axis_error_ratio)
        self.y_axis_error_ratio *= self.y_axis_ratio
        
    def _calculateFinalYAxis(self, bNumerator=True):
        """
        run full calculation for numerator or denominator
        """
        if bNumerator is True:
            _id = self.id_numerator
            self.peak_pixel_min = self.n_peak_pixel_min
            self.peak_pixel_max = self.n_peak_pixel_max
            self.back_pixel_min = self.n_back_pixel_min
            self.back_pixel_max = self.n_back_pixel_max
        else:
            _id = self.id_denominator
            self.peak_pixel_min = self.d_peak_pixel_min
            self.peak_pixel_max = self.d_peak_pixel_max
            self.back_pixel_min = self.d_back_pixel_min
            self.back_pixel_max = self.d_back_pixel_max
        
        nexus_file_numerator = self._determineNexusFileName(_id)    
        LoadEventNexus(Filename=nexus_file_numerator, 
                       OutputWorkspace='EventDataWks')
        mt1 = mtd['EventDataWks']
        proton_charge = self._getProtonCharge(mt1)
        rebin(InputWorkspace='EventDataWks', 
              OutputWorkspace='HistoDataWks', 
              Params=self.rebin_parameters)
        #mtd.deleteWorkspace('EventDataWks')
        mt2 = mtd['HistoDataWks']
        _x_axis = mt2.readX(0)[:]
        self._x_axis = _x_axis
        
        self._createIntegratedWorkspace(InputWorkspace=mt2, 
                                        OutputWorkspace='IntegratedDataWks',
                                        proton_charge=proton_charge, 
                                        from_pixel=self.x_pixel_min, 
                                        to_pixel=self.x_pixel_max)

        #mtd.deleteWorkspace('HistoDataWks')
        Transpose(InputWorkspace='IntegratedDataWks', 
                  OutputWorkspace='TransposeIntegratedDataWks')
        #mtd.deleteWorkspace('IntegratedDataWks')
        ConvertToHistogram(InputWorkspace='TransposeIntegratedDataWks',
                           OutputWorkspace='TransposeIntegratedDataWks_t')
        #mtd.deleteWorkspace('TransposeIntegratedDataWks')
        FlatBackground(InputWorkspace='TransposeIntegratedDataWks_t',
                       OutputWorkspace='TransposeHistoFlatDataWks',
                       StartX=self.back_pixel_min,
                       EndX=self.peak_pixel_min)
        #mtd.deleteWorkspace('TransposeIntegratedDataWks_t')
        Transpose(InputWorkspace='TransposeHistoFlatDataWks',
                  OutputWorkspace='DataWks')
        #mtd.deleteWorkspace('TransposeHistoFlatDataWks')
        mt3 = mtd['DataWks']        
        self._calculateFinalAxis(Workspace=mt3, 
                           bNumerator=bNumerator)
        #mtd.deleteWorkspace('DataWks')
    
    
    def _calculateFinalAxis(self, Workspace=None, bNumerator=None):
        """
        this calculates the final y_axis and y_axis_error of numerator and denominator
        """
        mt = Workspace
        _x_axis = mt.readX(0)[:]
        self._x_axis = _x_axis
        
        counts_vs_tof = zeros(len(_x_axis))
        counts_vs_tof_error = zeros(len(_x_axis))
        for x in range(self.alpha_pixel_nbr):
            counts_vs_tof += mt.readY(x)[:]
            counts_vs_tof_error += mt.readE(x)[:]**2
        counts_vs_tof_error = sqrt(counts_vs_tof_error)
        index_tof_min = self._getIndex(self.tof_min, _x_axis)
        index_tof_max = self._getIndex(self.tof_max, _x_axis)

        if (bNumerator is True):
            self._y_axis_numerator = counts_vs_tof[index_tof_min:index_tof_max]
            self._y_axis_error_numerator = counts_vs_tof_error[index_tof_min:index_tof_max]
            self.x_axis_ratio = self._x_axis[index_tof_min:index_tof_max]
        else:
            self._y_axis_denominator = counts_vs_tof[index_tof_min:index_tof_max]
            self._y_axis_error_denominator = counts_vs_tof_error[index_tof_min:index_tof_max]

    def _determineNexusFileName(self, id):
        """
        returns the full path to the nexus file
        """
        run_number = self.list_runs[id]
        full_path = self.nexus_path + self.pre
        full_path += str(run_number) + self.post
        return full_path
            
            
    def _createIntegratedWorkspace(self,
                                   InputWorkspace=None, 
                                   OutputWorkspace=None,
                                   proton_charge=None, 
                                   from_pixel=0, 
                                   to_pixel=303):
        """
        This creates the integrated workspace over the second pixel range (beta_pixel_nbr here) and
        returns the new workspace handle
        """
        _x_axis = InputWorkspace.readX(0)[:]
        x_size = to_pixel - from_pixel + 1 
        _y_axis = zeros((self.alpha_pixel_nbr, len(_x_axis) - 1))
        _y_error_axis = zeros((self.alpha_pixel_nbr, len(_x_axis) - 1))
        y_range = arange(x_size) + from_pixel
        for x in range(self.beta_pixel_nbr):
            for y in y_range:
                _index = int(self.alpha_pixel_nbr * x + y)
                _y_axis[y, :] += InputWorkspace.readY(_index)[:]
                _y_error_axis[y, :] += ((InputWorkspace.readE(_index)[:]) * 
                                        (InputWorkspace.readE(_index)[:]))

        _y_axis = _y_axis.flatten()
        _y_error_axis = sqrt(_y_error_axis)
        #plot_y_error_axis = _y_error_axis #for output testing only    -> plt.imshow(plot_y_error_axis, aspect='auto', origin='lower')
        _y_error_axis = _y_error_axis.flatten()

        #normalization by proton charge
        _y_axis /= (proton_charge * 1e-12)

        CreateWorkspace(OutputWorkspace, 
                        DataX=_x_axis, 
                        DataY=_y_axis, 
                        DataE=_y_error_axis, 
                       Nspec=self.alpha_pixel_nbr)
#        mt3 = mtd[OutputWorkspace]
#        return mt3 
            
    def _getIndex(self, value, array):
        """
        returns the index where the value has been found
        """
        return array.searchsorted(value)
            
    def _getProtonCharge(self, st=None):
        """
        Returns the proton charge of the given workspace in picoCoulomb
        """
        if st is not None:
            mt_run = st.getRun()
            proton_charge_mtd_unit = mt_run.getProperty('gd_prtn_chrg').value
            proton_charge = proton_charge_mtd_unit / 2.77777778e-10
            return proton_charge
        return None              
            

def plotObject(instance):
    
    return
    
    print 'a: ' + str(instance.a)
    print 'b: ' + str(instance.b)    
    
    figure()
    errorbar(instance.x_axis_ratio, 
             instance.y_axis_ratio, 
             instance.y_axis_error_ratio, 
             marker='s', 
             mfc='red',
             linestyle='')
    x=linspace(10000,22000,100)
    plot(x,instance.a+instance.b*x)
    xlabel("TOF (microsS)")
    ylabel("Ratio")
    title(instance.id_numerator + '/' + instance.id_denominator)
    show()


if __name__ == '__main__':
    
#    t_start = time.time()
    
    cal_D1 = CalculateAD(id_numerator='AiD1', id_denominator='AiD0')
    cal_D1.run()
    cal_D1.fit()
    plotObject(cal_D1)
    
    cal_D2 = CalculateAD(id_numerator='AiD2', id_denominator='AiD0')
    cal_D2.run()
    cal_D2.fit()
    plotObject(cal_D2)
               
    cal_D3 = CalculateAD(id_numerator='AiD3', id_denominator='AiD0')
    cal_D3.run()
    cal_D3.fit()
    plotObject(cal_D3)

    cal_D4 = CalculateAD(id_numerator='AiD4', id_denominator='AiD0')
    cal_D4.run()
    cal_D4.fit()
    plotObject(cal_D4)

    cal_D5 = CalculateAD(id_numerator='AiD5', id_denominator='AiD0')
    cal_D5.run()
    cal_D5.fit()
    plotObject(cal_D5)

    cal_D6 = CalculateAD(id_numerator='AiiAiD6', id_denominator='AiiAiD5')
    cal_D6.run()
    cal_D6.fit()
    plotObject(cal_D6)

    cal_D6 = CalculateAD(id_numerator='AiiAiD6', id_denominator='AiiAiD5')
    cal_D6.run()
    product_D6 = cal_D6 * cal_D5
    product_D6.fit()
    plotObject(product_D6)

    cal_D7 = CalculateAD(id_numerator='AiiAiD7', id_denominator='AiiAiD5')
    cal_D7.setNumerator(minPeak=128, maxPeak=136, minBack=120, maxBack=145)
    cal_D7.run()
    product_D7 = cal_D7 * cal_D5
    product_D7.fit()
    plotObject(product_D7)

    cal_D8 = CalculateAD(id_numerator='AiiiAiiAiD8', id_denominator='AiiiAiiAiD7')
    cal_D8.setNumerator(minPeak=125, maxPeak=140, minBack=115, maxBack=150)
    cal_D8.setDenominator(minPeak=128, maxPeak=136, minBack=120, maxBack=145)
    cal_D8.run()
    product_D8 = cal_D8 * product_D7
    product_D8.fit()
    plotObject(product_D8)
    
    cal_D9 = CalculateAD(id_numerator='AivAiiiAiiAiD9', id_denominator='AivAiiiAiiAiD8')
    cal_D9.setNumerator(minPeak=120, maxPeak=145, minBack=105, maxBack=155)
    cal_D9.setDenominator(minPeak=125, maxPeak=140, minBack=115, maxBack=150)
    cal_D9.run()
    product_D9 = cal_D9 * product_D8 
    product_D9.fit()
    plotObject(product_D9)

#    t_end = time.time()
#    print 'Time to run the process: %0.1f s' % (t_end-t_start)                   
                     
            