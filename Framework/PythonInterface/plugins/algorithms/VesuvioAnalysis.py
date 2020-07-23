'''
This file was last edited by Giovanni Romanelli on 23/05/2020.
The structure of the script is as follows:
    First, the collection of functions allowing the reduction and analysis of the spectra;
        Second, a list of input parameters specific of the VESUVIO experiment;
            Last, the reduction and analysis procedure, iterative in TOF and finally in y-space for hydrogen.
            
The script has been tested to be run in Mantid Workbench in a Windows operative system.

PLEASE, DO NOT MODIFY THE "TECHNICAL SECTION" UNLESS YOU ARE AN 
EXPERT VESUVIO INSTRUMENT SCIENTIST.
'''

##########################################################
####        TECHNICAL SECTION - NOT FOR USERS
##########################################################

from __future__ import (absolute_import, division, print_function, unicode_literals)


import numpy as np
import matplotlib.pyplot as plt
import mantid                          
from mantid.simpleapi import *    
from scipy import signal
from scipy.optimize import curve_fit
from scipy import optimize
from scipy.ndimage import convolve1d
from mantid.api import PropertyMode
from mantid.api import *
from mantid.kernel import  StringListValidator, IntListValidator, FloatBoundedValidator, Direction

# command for the formatting of the printed output
np.set_printoptions(suppress=True, precision=4, linewidth= 150 )


# path to instrument parameter file:
#ipfile = '//olympic/babylon5/Public/Romanelli/Algorithms/ip2018_3.par'
ipfile = "C://Users//BTR75544//work//shared//vesuvio//ip2018.par"

#
#   INITIALISING FUNCTIONS AND USEFUL FUNCTIONS
#
def fun_gaussian(x, sigma):
    gaussian = np.exp(-x**2/2/sigma**2)
    gaussian /= np.sqrt(2.*np.pi)*sigma
    return gaussian

def fun_lorentzian(x, gamma):
    lorentzian = gamma/np.pi / (x**2 + gamma**2)
    return lorentzian

def fun_pseudo_voigt(x, sigma, gamma): #input std gaussiana e hwhm lorentziana
    fg, fl = 2.*sigma*np.sqrt(2.*np.log(2.)), 2.*gamma #parameters transformed to gaussian and lorentzian FWHM
    f = 0.5346 * fl + np.sqrt(0.2166*fl**2 + fg**2 )
    eta = 1.36603 *fl/f - 0.47719 * (fl/f)**2 + 0.11116 *(fl/f)**3
    sigma_v, gamma_v = f/(2.*np.sqrt(2.*np.log(2.))), f /2.
    pseudo_voigt = eta * fun_lorentzian(x, gamma_v) + (1.-eta) * fun_gaussian(x, sigma_v)
    norm=np.sum(pseudo_voigt)*(x[1]-x[0])
    return pseudo_voigt#/np.abs(norm)

def fun_derivative3(x,fun): # Used to evaluate numerically the FSE term.
    derivative =np.zeros(len(fun))
    denom =np.zeros(len(fun))
    for i in range(6,len(fun)-6):
        denom[i] = (x[i+1]-x[i])

    denom = np.power(denom,3.)
    indicies = [1,2,3,4,5,6]
    factors = [-1584., 387., 488., -192., 24., -1.0] 
      
    for i in range(6,len(fun)-6):
        tmp = 0.0
        for j in range(len(indicies)):
               tmp += factors[j]*fun[i+indicies[j]] - factors[j]*fun[i-indicies[j]]

        #tmp += 5#factors[0]*fun[i+indicies[0]] - factors[0]*fun[i-indicies[0]]
        #tmp += factors[1]*fun[i+indicies[1]] - factors[1]*fun[i-indicies[1]]
        #tmp += factors[2]*fun[i+indicies[2]] - factors[2]*fun[i-indicies[2]]
        #tmp += factors[3]*fun[i+indicies[3]] - factors[3]*fun[i-indicies[3]]
        #tmp += factors[4]*fun[i+indicies[4]] - factors[4]*fun[i-indicies[4]]
        #tmp += factors[5]*fun[i+indicies[5]] - factors[5]*fun[i-indicies[5]]
        if denom[i] != 0:
            derivative[i]= tmp/denom[i]
        else:
            print("divide by zero")
    #print(type(derivative))
    #derivative=np.array(derivative)/1728. #12**3
    derivative = derivative/1728. #12**3

    return derivative

def fun_derivative4(x,fun): # not used at present. Can be used for the H4 polynomial in TOF fitting.
    derivative =[0.]*len(fun)
    for i in range(8,len(fun)-8):
        derivative[i] = fun[i-8]   -32.*fun[i-7]  +384*fun[i-6]  -2016.*fun[i-5]  +3324.*fun[i-4]  +6240.*fun[i-3]  -16768*fun[i-2]  -4192.*fun[i-1]  +26118.*fun[i]
        derivative[i]+=fun[i+8] -32.*fun[i+7] +384*fun[i+6] -2016.*fun[i+5] +3324.*fun[i+4] +6240.*fun[i+3] -16768*fun[i+2] -4192.*fun[i+1]
        derivative[i]/=(x[i+1]-x[i])**4
    derivative=np.array(derivative)/(12**4)
    return derivative

def load_ip_file(spectrum):
    f = open(ipfile, 'r')
    data = f.read()
    lines = data.split('\n')
    for line in range(lines.__len__()):
        col = lines[line].split('\t')
        if col[0].isdigit() and int(col[0]) == spectrum:
            angle = float(col[2])
            #at present many VESUVIO scripts work only if a value of T0 is provided,
            # yet this is not included in the instrument definition file!
            T0 = float(col[3])
            L0 = float(col[4])
            L1 = float(col[5])
    f.close()
    return angle, T0, L0, L1

def load_resolution_parameters(spectrum): 
    if spectrum > 134: # resolution parameters for front scattering detectors, in case of single difference
        dE1= 73. # meV , gaussian standard deviation
        dTOF= 0.37 # us
        dTheta= 0.016 #rad
        dL0= 0.021 # meters
        dL1= 0.023 # meters
        lorentzian_res_width = 24. # meV , HFHM
    if spectrum < 135: # resolution parameters for back scattering detectors, in case of double difference
        dE1= 88.7 # meV , gaussian standard deviation
        dTOF= 0.37 # us
        dTheta= 0.016 #rad
        dL0= 0.021 # meters
        dL1= 0.023 # meters
        lorentzian_res_width = 40.3 # meV , HFHM
    return dE1, dTOF, dTheta, dL0, dL1, lorentzian_res_width
    
def load_constants():
    mN=1.008    #a.m.u.
    Ef=4906.         # meV
    en_to_vel = 4.3737 * 1.e-4
    vf=np.sqrt( Ef ) * en_to_vel        #m/us
    hbar = 2.0445
    return mN, Ef, en_to_vel, vf, hbar

def load_workspace(ws_name, spectrum):
    ws=mtd[str(ws_name)]
    ws_len, ws_spectra =ws.blocksize()-1, ws.getNumberHistograms()
    ws_x,ws_y, ws_e = [0.]*ws_len, [0.]*ws_len,[0.]*ws_len
    for spec in range(ws_spectra):
        if ws.getSpectrum(spec).getSpectrumNo() == spectrum :
            # mantid ws has convert to points
            # mantid.ConvertToPointData(InputWorkspace=ws.name(), OutputWorkspace=ws.name())
            for i in range(ws_len):
                # converting the histogram into points
                ws_y[i] = ( ws.readY(spec)[i] / (ws.readX(spec)[i+1] - ws.readX(spec)[i] ) )
                ws_e[i] = ( ws.readE(spec)[i] / (ws.readX(spec)[i+1] - ws.readX(spec)[i] ) )
                ws_x[i] = ( 0.5 * (ws.readX(spec)[i+1] + ws.readX(spec)[i] ) )
    ws_x, ws_y, ws_e = np.array(ws_x), np.array(ws_y), np.array(ws_e)
    return ws_x, ws_y, ws_e
#
#   FITTING FUNCTIONS
#

# whats ncp?
def block_fit_ncp(par,first_spectrum,last_spectrum, masses,ws_name,fit_arguments, verbose):
    print( "\n", "Fitting Workspace: ", str(ws_name))
    if verbose:
        print ("Fitting parameters are given as: [Intensity Width Centre] for each NCP")
    widths=np.zeros((len(masses),last_spectrum-first_spectrum+1))
    intensities=np.zeros((len(masses),last_spectrum-first_spectrum+1))
    centres=np.zeros((len(masses),last_spectrum-first_spectrum+1))
    spectra=np.zeros((last_spectrum-first_spectrum+1))
    tof_fit_ws = CloneWorkspace(InputWorkspace=str(ws_name),OutputWorkspace=str(ws_name)+"_fit")
    j=0
    for spectrum in range(first_spectrum,last_spectrum+1):
        data_x, data_y,data_e = load_workspace(ws_name , spectrum)
        ncp, fitted_par, result = fit_ncp(par, spectrum, masses, data_x, data_y, data_e, fit_arguments)
        #print("ho",ncp)
        for bin in range(len(data_x)-1):
            tof_fit_ws.dataY(j)[bin] = ncp[bin]*(data_x[bin+1]-data_x[bin])
            tof_fit_ws.dataE(j)[bin] = 0.
        # Calculate the reduced chi2 from the fitting Cost function:
        reduced_chi2 = result.fun/(len(data_x) - len(par))        
        if verbose:
            if (reduced_chi2 > 1.e-3):
                print( spectrum, fitted_par, "%.4g" % reduced_chi2)
            else:
                print( spectrum, " ... skipping ...")
        npars = len(par)/len(masses)
        for m in range(len(masses)):
            if (reduced_chi2>1.e-3):
                index = int(npars*m)
                intensities[m][j]=float(fitted_par[index])
                widths[m][j]=float(fitted_par[index+1])
                centres[m][j]=float(fitted_par[index+2])
            else:
                widths[m][j]=None
                intensities[m][j]=None
                centres[m][j]=None
        
        spectra[j]=spectrum
        j+=1
    #print("moo")
    return spectra, widths, intensities, centres

def fit_ncp(par, spectrum, masses, data_x, data_y, data_e, fit_arguments):
    boundaries, constraints = fit_arguments[0], fit_arguments[1]
    result = optimize.minimize(errfunc, par[:], args=(spectrum, masses, data_x, data_y, data_e), method='SLSQP', bounds = boundaries, constraints=constraints)
    fitted_par = result.x
    ncp = calculate_ncp(fitted_par, spectrum , masses, data_x)
    return ncp, fitted_par, result

def errfunc( par ,  spectrum, masses, data_x,  data_y, data_e):
    # this function provides the scalar to be minimised, with meaning of the non-reduced chi2
    ncp = calculate_ncp(par, spectrum , masses, data_x)
    if (np.sum(data_e) > 0):
        chi2 =  (ncp - data_y)**2/(data_e)**2 # Distance to the target function
    else:
        chi2 =  (ncp - data_y)**2
    return chi2.sum()

def calculate_ncp(par, spectrum , masses, data_x):
    angle, T0, L0, L1 = load_ip_file(spectrum)
    mN, Ef, en_to_vel, vf, hbar = load_constants()
    ncp = 0. # initialising the function values
    # velocities in m/us, times in us, energies in meV
    v0, E0, delta_E, delta_Q = calculate_kinematics(data_x, angle, T0, L0, L1 )
    npars = len(par)/len(masses)
    for m in range(len(masses)):#   [parameter_index + number_of_mass * number_of_parameters_per_mass ]
        index = int(m*npars)
        mass, hei , width, centre = masses[m] , par[index], par[1+index], par[2+index]
        E_r = ( hbar * delta_Q )**2 / (2. * mass)
        y = mass*(delta_E-E_r)/(hbar*hbar*delta_Q) #mass / hbar**2 /delta_Q * (delta_E - E_r)

        if np.isnan(centre) :
            centre = 0. # the line below give error if centre = nan, i.e., if detector is masked
        joy = fun_gaussian(y-centre, 1.)
        pcb = np.where(joy == joy.max()) # this finds the peak-centre bin (pcb)
        gaussian_res_width, lorentzian_res_width = calculate_resolution(spectrum, data_x[pcb], mass)
        # definition of the experimental neutron compton profile
        gaussian_width = np.sqrt( width**2 + gaussian_res_width**2 )
        joy = fun_pseudo_voigt(y-centre, gaussian_width, lorentzian_res_width)
        FSE =  -0.72*fun_derivative3(y,joy)/delta_Q#*(width*width*width*width)/delta_Q # 0.72 is an empirical coefficient. One can alternatively add a fitting parameter for this term.
        #H4  = some_missing_coefficient *  fun_derivative4(y,joy) /(4.*(width**4)*32.)
        ncp += hei * (joy + FSE ) * E0 * E0**(-0.92) * mass / delta_Q # Here -0.92 is a parameter describing the epithermal flux tail.
    return ncp

def calculate_kinematics(data_x, angle, T0, L0, L1 ):
    mN, Ef, en_to_vel, vf, hbar = load_constants()
    t_us = data_x - T0
    v0 = vf * L0 / ( vf * t_us - L1 )
    E0 =  ( v0 / en_to_vel )**2 
    delta_E = E0 -Ef  
    delta_Q2 = 2. * mN / hbar**2 * ( E0 + Ef - 2. * np.sqrt( E0*Ef) * np.cos(angle/180.*np.pi) )
    delta_Q = np.sqrt( delta_Q2 )
    return v0, E0, delta_E, delta_Q

def calculate_resolution(spectrum, data_x, mass):
    angle, T0, L0, L1 = load_ip_file(spectrum)
    mN, Ef, en_to_vel, vf, hbar = load_constants()
    # all standard deviations, apart from lorentzian hwhm
    dE1, dTOF, dTheta, dL0, dL1, lorentzian_res_width = load_resolution_parameters(spectrum)
    v0, E0, delta_E, delta_Q = calculate_kinematics(data_x, angle, T0, L0, L1 )
    # definition of the resolution components in meV:
    dEE = (1. + (E0 / Ef)**1.5 * ( L1 / L0 ) )**2 * dE1**2 + (2. * E0 * v0 / L0 )**2 * dTOF**2 
    dEE+= ( 2. * E0**1.5 / Ef**0.5 / L0 )**2 * dL1**2 + ( 2. * E0 / L0 )**2 * dL0**2
    dQQ =  (1. - (E0 / Ef )**1.5 *L1 / L0 - np.cos(angle/180.*np.pi) * ( ( E0 / Ef )**0.5 - L1 / L0 * E0 / Ef ))**2 * dE1**2
    dQQ+= ( ( 2. * E0 * v0 / L0 )**2 * dTOF**2 + (2. * E0**1.5 / L0 / Ef**0.5 )**2 *dL1**2 + ( 2. * E0 / L0 )**2 * dL0**2 ) * np.abs( Ef / E0 * np.cos(angle/180.*np.pi) -1.)
    dQQ+= ( 2. * np.sqrt( E0 * Ef )* np.sin(angle/180.*np.pi) )**2 * dTheta**2
    # conversion from meV^2 to A^-2
    dEE*= ( mass / hbar**2 /delta_Q )**2
    dQQ*= ( mN / hbar**2 /delta_Q )**2
    gaussian_res_width =   np.sqrt( dEE + dQQ ) # in A-1
    #lorentzian component in meV
    dEE_lor = (1. + (E0 / Ef)**1.5 * ( L1 / L0 ) )**2                                                       # is it - or +?
    dQQ_lor =  (1. - (E0 / Ef )**1.5 *L1 / L0 - np.cos(angle/180.*np.pi) * ( ( E0 / Ef )**0.5 + L1 / L0 * E0 / Ef )) **2
    # conversion from meV^2 to A^-2
    dEE_lor*= ( mass / hbar**2 /delta_Q )**2
    dQQ_lor*= ( mN / hbar**2 /delta_Q )**2
    lorentzian_res_width *= np.sqrt( dEE_lor + dQQ_lor ) # in A-1
    return gaussian_res_width, lorentzian_res_width # gaussian std dev, lorentzian hwhm 
#
#       CORRECTION FUNCTIONS
#
def calculate_mean_widths_and_intensities(masses,widths,intensities,spectra, verbose):
    better_widths, better_intensities =np.zeros((len(masses),len(widths[0]))),np.zeros((len(masses),len(widths[0])))
    mean_widths,widths_std,mean_intensity_ratios=np.zeros((len(masses))),np.zeros((len(masses))),np.zeros((len(masses)))
    for m in range(len(masses)):
        mean_widths[m]=np.nanmean(widths[m])
        widths_std[m]=np.nanstd(widths[m])
        for index in range(len(widths[0])): # over all spectra
            if  abs( widths[m][index]-mean_widths[m] ) > widths_std[m]:
                better_widths[m][index],better_intensities[m][index]= None, None
            else:
                better_widths[m][index],better_intensities[m][index]= widths[m][index],intensities[m][index]
        mean_widths[m]=np.nanmean(better_widths[m])
        widths_std[m]=np.nanstd(better_widths[m])
    for spec in range(len(spectra)):
        normalisation = better_intensities[:,spec]
        better_intensities[:,spec]/=normalisation.sum()
    for m in range(len(masses)):
        mean_intensity_ratios[m] = np.nanmean(better_intensities[m])
    for m in range(len(masses)):
        print ("\n", "Mass: ", masses[m], " width: ", mean_widths[m], " \pm ", widths_std[m])
    return mean_widths, mean_intensity_ratios

def calculate_sample_properties(masses,mean_widths,mean_intensity_ratios, mode, verbose):
    if mode == "GammaBackground":
        profiles=""
        for m in range(len(masses)):
            mass, width, intensity=str(masses[m]), str(mean_widths[m]),str(mean_intensity_ratios[m])
            profiles+= "name=GaussianComptonProfile,Mass="+mass+",Width="+width+",Intensity="+intensity+';' 
        sample_properties = profiles
    elif mode == "MultipleScattering":
        MS_properties=[]
        for m in range(len(masses)):
            MS_properties.append(masses[m])
            MS_properties.append(mean_intensity_ratios[m])
            MS_properties.append(mean_widths[m])
        sample_properties = MS_properties        
    if verbose:
        print ("\n", "The sample properties for ",mode," are: ", sample_properties)
    return sample_properties
        
def correct_for_gamma_background(ws_name,first_spectrum,last_spectrum, sample_properties, verbose):
    if verbose:
        print( "Evaluating the Gamma Background Correction.")
    gamma_background_correction=CloneWorkspace(ws_name)
    for spec in range(first_spectrum,last_spectrum+1):
        ws_index=gamma_background_correction.getIndexFromSpectrumNumber(spec)
        tmp_bkg, tmp_cor = VesuvioCalculateGammaBackground(InputWorkspace=ws_name,ComptonFunction=sample_properties, WorkspaceIndexList=ws_index)
        for bin in range(gamma_background_correction.blocksize()):
            gamma_background_correction.dataY(ws_index)[bin]=tmp_bkg.dataY(0)[bin]
            gamma_background_correction.dataE(ws_index)[bin]=0.
    RenameWorkspace(InputWorkspace= "gamma_background_correction", OutputWorkspace = str(ws_name)+"_gamma_background")
    DeleteWorkspace("tmp_cor")
    DeleteWorkspace("tmp_bkg")
    return

def create_slab_geometry(ws_name,vertical_width, horizontal_width, thickness):
        half_height, half_width, half_thick = 0.5*vertical_width, 0.5*horizontal_width, 0.5*thickness
        xml_str = \
        " <cuboid id=\"sample-shape\"> " \
        + "<left-front-bottom-point x=\"%f\" y=\"%f\" z=\"%f\" /> " % (half_width,-half_height,half_thick) \
        + "<left-front-top-point x=\"%f\" y=\"%f\" z=\"%f\" /> " % (half_width, half_height, half_thick) \
        + "<left-back-bottom-point x=\"%f\" y=\"%f\" z=\"%f\" /> " % (half_width, -half_height, -half_thick) \
        + "<right-front-bottom-point x=\"%f\" y=\"%f\" z=\"%f\" /> " % (-half_width, -half_height, half_thick) \
        + "</cuboid>"
        CreateSampleShape(ws_name, xml_str)
        return

def correct_for_multiple_scattering(ws_name,first_spectrum,last_spectrum, sample_properties, transmission_guess, multiple_scattering_order, number_of_events, verbose, masses, mean_intensity_ratios):
    if verbose:
        print( "Evaluating the Multiple Scattering Correction.")
    dens, trans = VesuvioThickness(Masses=masses, Amplitudes=mean_intensity_ratios, TransmissionGuess=transmission_guess,Thickness=0.1)         
    _TotScattering, _MulScattering = VesuvioCalculateMS(ws_name, NoOfMasses=len(masses), SampleDensity=dens.cell(9,1), 
                                                                        AtomicProperties=sample_properties, BeamRadius=2.5,
                                                                        NumScatters=multiple_scattering_order, 
                                                                        NumEventsPerRun=int(number_of_events))
    data_normalisation = Integration(ws_name) 
    simulation_normalisation = Integration("_TotScattering")
    for workspace in ("_MulScattering","_TotScattering"):
        ws = mtd[workspace]
        for j in range(ws.getNumberHistograms()):
            for k in range(ws.blocksize()):
                ws.dataE(j)[k] =0. # set the errors from the MonteCarlo simulation to zero - no propagation of such uncertainties - Use high number of events for final corrections!!!
        Divide(LHSWorkspace = workspace, RHSWorkspace = simulation_normalisation, OutputWorkspace = workspace)
        Multiply(LHSWorkspace = workspace, RHSWorkspace = data_normalisation, OutputWorkspace = workspace)
        RenameWorkspace(InputWorkspace = workspace, OutputWorkspace = str(ws_name)+workspace)
    DeleteWorkspace(data_normalisation)
    DeleteWorkspace(simulation_normalisation)
    DeleteWorkspace(trans)
    DeleteWorkspace(dens)
    return
############################
### functions to fit the NCP in the y space
############################
def subtract_other_masses(ws_name, widths, intensities, centres, spectra, masses):
    hydrogen_ws = CloneWorkspace(InputWorkspace=ws_name)
    for index in range(len(spectra)): # for each spectrum
        data_x, data_y, data_e = load_workspace(ws_name , spectra[index]) # get the experimental data after the last correction
        for m in range(len(masses)-1): # for all the masses but the first (generally H)
            other_par = (intensities[m+1, index], widths[m+1, index], centres[m+1,index]) # define the input parameters to get the NCPs
            ncp = calculate_ncp(other_par, spectra[index], [masses[m+1]], data_x)
            for bin in range(len(data_x)-1):
                hydrogen_ws.dataY(index)[bin] -= ncp[bin]*(data_x[bin+1]-data_x[bin])
    return hydrogen_ws

def convert_to_y_space_and_symmetrise(ws_name,mass):
    # phenomenological roule-of-thumb to define the y-range for a given mass
    max_Y = np.ceil(2.5*mass+27)
    rebin_parameters = str(-max_Y)+","+str(2.*max_Y/120)+","+str(max_Y)
    # converting to y-space, rebinning, and defining a normalisation matrix to take into account the kinetic cut-off
    ConvertToYSpace(InputWorkspace=ws_name,Mass=mass,OutputWorkspace=ws_name+"_JoY",QWorkspace=ws_name+"_Q")
    ws = Rebin(InputWorkspace=ws_name+"_JoY", Params = rebin_parameters,FullBinsOnly=True, OutputWorkspace= ws_name+"_JoY")
    tmp=CloneWorkspace(InputWorkspace=ws_name+"_JoY")
    for j in range(tmp.getNumberHistograms()):
        for k in range(tmp.blocksize()):
            tmp.dataE(j)[k] =0.
            if np.isnan( tmp.dataY(j)[k] ) :
                ws.dataY(j)[k] =0.
                tmp.dataY(j)[k] =0.
            if (tmp.dataY(j)[k]!=0):
                tmp.dataY(j)[k] =1.
    tmp=SumSpectra('tmp')
    SumSpectra(InputWorkspace=ws_name+"_JoY",OutputWorkspace=ws_name+"_JoY_sum")
    Divide(LHSWorkspace=ws_name+"_JoY_sum", RHSWorkspace="tmp", OutputWorkspace =ws_name+"_JoY_sum")
    #rewriting the temporary workspaces ws and tmp
    ws=mtd[ws_name+"_JoY_sum"]
    tmp=CloneWorkspace(InputWorkspace=ws_name+"_JoY_sum")
    for k in range(tmp.blocksize()):
        tmp.dataE(0)[k] =(ws.dataE(0)[k]+ws.dataE(0)[ws.blocksize()-1-k])/2.
        tmp.dataY(0)[k] =(ws.dataY(0)[k]+ws.dataY(0)[ws.blocksize()-1-k])/2.
    RenameWorkspace(InputWorkspace="tmp",OutputWorkspace=ws_name+"_JoY_sym")
    normalise_workspace(ws_name+"_JoY_sym")
    return max_Y

def calculate_mantid_resolutions(ws_name, mass):
    max_Y = np.ceil(2.5*mass+27)
    rebin_parameters = str(-max_Y)+","+str(2.*max_Y/240)+","+str(max_Y) # twice the binning as for the data
    ws= mtd[ws_name]
    for index in range(ws.getNumberHistograms()):
        VesuvioResolution(Workspace=ws,WorkspaceIndex=index,Mass=mass,OutputWorkspaceYSpace="tmp")
        tmp=Rebin("tmp",rebin_parameters)
        if index == 0:
            RenameWorkspace("tmp","resolution")
        else:
            AppendSpectra("resolution", "tmp", OutputWorkspace= "resolution")
    SumSpectra(InputWorkspace="resolution",OutputWorkspace="resolution")
    normalise_workspace("resolution")
    DeleteWorkspace("tmp")
    
def normalise_workspace(ws_name):
    tmp_norm = Integration(ws_name)
    Divide(LHSWorkspace=ws_name,RHSWorkspace="tmp_norm",OutputWorkspace=ws_name)
    DeleteWorkspace("tmp_norm")

def final_fit(fit_ws_name, constraints,y_range, correct_for_offsets, masses) :
    function = """
    composite=Convolution,FixResolution=true,NumDeriv=true;
        name=Resolution,Workspace=resolution,WorkspaceIndex=0,X=(),Y=();
        name=UserFunction,Formula=exp( -x^2/2./sigma1^2)
        *(1.+c4/32.*(16.*(x/sqrt(2)/sigma1)^4-48.*(x/sqrt(2)/sigma1)^2+12)
              +c6/384.*( 64.*(x/sqrt(2)/sigma1)^6 -480.*(x/sqrt(2)/sigma1)^4 +720.*(x/sqrt(2)/sigma1)^2 -120.) )*A + B0,
        sigma1=3.0,c4=0.0, c6=0.0,A=0.08, B0=0.00, ties = (c6=0. )
        """
    function+=constraints
    minimiser = "Simplex"
    Fit(Function= function, InputWorkspace=fit_ws_name, MaxIterations=2000, Minimizer= minimiser, Output=fit_ws_name,
                OutputCompositeMembers=True, StartX = y_range[0] , EndX = y_range[1])
    ws = mtd[fit_ws_name+"_Parameters"]
    print( "\n Final parameters \n")
    print( "width: ",ws.cell(0,1)," +/- ",ws.cell(0,2), " A-1 ")
    print( "c4: ",ws.cell(1,1)," +/- ",ws.cell(1,2), " A-1 ")
    sigma_to_energy = 1.5 * 2.0445**2 / masses[0] 
    print( "mean kinetic energy: ",sigma_to_energy*ws.cell(0,1)**2," +/- ", 2.*sigma_to_energy*ws.cell(0,2)*ws.cell(0,1), " meV ")
    if correct_for_offsets :
        Scale(InputWorkspace=fit_ws_name,Factor=-ws.cell(4,1),Operation="Add",OutputWorkspace=fit_ws_name+'_cor')
        Scale(InputWorkspace=fit_ws_name+'_cor',Factor=(2.*np.pi)**(-0.5)/ws.cell(0,1)/ws.cell(3,1),Operation="Multiply",OutputWorkspace=fit_ws_name+'_cor')

class element:
    def __init__(self, mass, intensity_range, width_range, centre_range):
        self.mass = float(mass)
        self.intensity_low, self.intensity, self.intensity_high = intensity_range[0],intensity_range[1],intensity_range[2]
        self.width_low, self.width, self.width_high = width_range[0],width_range[1],width_range[2]
        self.centre_low, self.centre, self.centre_high = centre_range[0],centre_range[1],centre_range[2]

class constraint: # with reference to the "elements" vector positions
    def __init__(self, lhs_element_position, rhs_element_position, rhs_factor , type):
        self.lhs_element_position = lhs_element_position
        self.rhs_element_position = rhs_element_position
        self.rhs_factor = rhs_factor
        self.type = type

def prepare_fit_arguments(elements, constraints) :
    masses=list(np.zeros( len(elements) ) ) 
    masses[0]=elements[0].mass
    par = (elements[0].intensity, elements[0].width, elements[0].centre)
    bounds = ( (elements[0].intensity_low, elements[0].intensity_high), (elements[0].width_low, elements[0].width_high), (elements[0].centre_low, elements[0].centre_high) )
    for m in range(len(elements) -1 ):
        m += 1
        masses[m] = elements[m].mass
        par += ( elements[m].intensity, elements[m].width, elements[m].centre )
        bounds += ( (elements[m].intensity_low, elements[m].intensity_high), (elements[m].width_low, elements[m].width_high), (elements[m].centre_low, elements[m].centre_high) )
    for k in range(len(constraints) ):
        # from element position in elements to intensity position in par
        lhs_int, rhs_int = 3*constraints[k].lhs_element_position, 3*constraints[k].rhs_element_position
        fit_constraints = ({'type': constraints[k].type, 'fun': lambda par:  par[lhs_int] -constraints[k].rhs_factor*par[rhs_int] })
    return masses, par, bounds, fit_constraints

################################################################################################
##################                                                                                                                                 ##################
##################                                                                                                                                 ##################
##################                                                                                                                                 ##################
##################                                                  HAVE FUN!                                                               ##################
##################                                                                                                                                 ##################
##################                                                                                                                                 ##################
##################                                                                                                                                 ##################
##################                                                                                                                                 ##################
################################################################################################
##########################################################
####        USER SECTION  -  FOR USERS 
##########################################################
'''
The user section is composed of an initialisation section, an iterative analysis/reduction section
of the spectra in the time-of-flight domain, and a final section where the analysis of the corrected
hydrogen neutron Compton profile is possible in the Y-space domain.

The fit procedure in the time-of-flight domain is  based on the scipy.minimize.optimize() tool,
used with the SLSQP minimizer, that can handle both boundaries and constraints for fitting parameters.

The Y-space analysis is, at present, performed on a single spectrum, being the result of
the sum of all the corrected spectra, subsequently symmetrised and unit-area normalised.

The Y-space fit is performed using the Mantid minimiser and average Mantid resolution function, using
a Gauss-Hermite expansion including H0 and H4 at present, while H3 (proportional to final-state effects)
is not needed as a result of the symmetrisation.
'''

def cleanNames(list):
    return [name.replace(" ","").lower() for name in list]

class vesuvioTest(PythonAlgorithm):
    def category(self):
        return "Indirect"

    def seeAlso(self):
        return ["Rebin"]

    def PyInit(self):
        self.declareProperty("AnalysisMode","LoadReduceAnalyse", doc="In the first case, all the algorithm is run. In the second case, the data are not re-loaded, and only the TOF and y-scaling bits are run. In the third case, only the y-scaling final analysis is run.", validator=StringListValidator(["LoadReduceAnalyse","ReduceAnalyse","Analyse"]))
        self.declareProperty("Verbose",True, doc="If to print the fitting parameters.")
        self.declareProperty("NumberOfIterations",0, doc="Number of time the reduction is reiterated.", validator=IntListValidator([0,1,2,3,4]))
        self.declareProperty("OutputName", "vesuvio",doc="The base name for the outputs." )
        self.declareProperty("Runs", "", doc="List of Vesuvio run numbers (e.g. 20934-20937, 30924)")
        self.declareProperty("Spectra","135-182", doc="Range of spectra to be analysed.")
        self.declareProperty("TOFRangeStart", 110,doc="In micro seconds.")
        self.declareProperty("TOFRangeLowerBound", 110,doc="In micro seconds.")
        self.declareProperty("TOFRangeBinning", 1.5, doc="In micro seconds.")
        self.declareProperty("TOFRangeUpperBound", 460, doc="In micro seconds.")
        self.declareProperty("TransmissionGuess", 1.0, doc="A number from 0 to 1 to represent the experimental transmission value of the sample for epithermal neutrons. This value is used for the multiple scattering corrections. If 1, the multiple scattering correction is not run.",validator=FloatBoundedValidator(0,1) )
        self.declareProperty("MultipleScatteringOrder", 1, doc="Order of multiple scattering events in MC simultation.",validator=IntListValidator([1,2,3,4]))
        self.declareProperty("MonteCarloEvents", 1.e5, doc="Number of events for MC multiple scattering simulation.")
        self.declareProperty(ITableWorkspaceProperty("ComptonProfile",
                                   "",
                                   direction=Direction.Input),doc="Table for Compton profiles")
        self.declareProperty("ConstraintsProfileNumbers", "1,2")
        self.declareProperty("ConstraintsProfileScatteringCrossSection", "3.*5.551/4.232")
        self.declareProperty("ConstraintsProfileState", "eq", validator=StringListValidator(["eq","ineq"]))
        self.declareProperty("SpectraToBeMasked", "")
        self.declareProperty("SubtractResonancesFunction", "", doc="Function for resonance subtraction. Empty means no subtraction.")
        #self.declareProperty(FunctionProperty("SubtractResonancesFunction", PropertyMode.Optional), doc="Function for resonance subtraction. Empty means no subtraction.")
        #self.declareProperty(FunctionProperty("YSpaceFitFunction",PropertyMode.Optional), doc="The TOF spectra are subtracted by all the fitted profiles but the first element specified in the ?elements? string. Then such spectra are converted to the Y space of the first element (using the ConvertToYSPace algorithm). The spectra are summed together and symmetrised. A fit on the resulting spectrum is performed using a Gauss Hermte function up to the sixth order.") 
        self.declareProperty("YSpaceFitFunction", "",doc="The TOF spectra are subtracted by all the fitted profiles but the first element specified in the elements string. Then such spectra are converted to the Y space of the first element (using the ConvertToYSPace algorithm). The spectra are summed together and symmetrised. A fit on the resulting spectrum is performed using a Gauss Hermte function up to the sixth order.") 
    def validateInputs(self):
        tableCols = ["symbol", "mass(a.u.)", "Intensity lower limit", "Intensity value", "Intensity upper limit", "Width lower limit", "Width value", "Width upper limit", "Centre lower limit", "Centre value", "Centre upper limit"]
        issues = dict()
        table = self.getProperty("ComptonProfile").value
        if(table.columnCount()!= len(tableCols) or sorted(cleanNames(tableCols))!=sorted(cleanNames(table.getColumnNames())) ):
            issues["ComptonProfile"] = "The table should be of the form: "
            for name in tableCols:
                issues["ComptonProfile"] += name + ", "
        return issues

    def PyExec(self):
       load_data=True                             # If data have already been loaded, it can be put to Fasle to save time;
       verbose=True                                 # If True, prints the value of the fitting parameters for each time-of-flight spectrum
       number_of_iterations = 2                # This is the number of iterations for the reduction analysis in time-of-flight.
       # Parameters of the measurement
       ws_name="zrh2"
       runs = "30544-30564"
       first_spectrum, last_spectrum = 135, 182
       tof_range = "110,1.5,460"
       # Parameters for the multiple-scattering correction, including the shape of the sample.
       transmission_guess = .9 #  experimental value from VesuvioTransmission
       multiple_scattering_order, number_of_events = 2, 1.e4

       # parameters of the neutron Compton profiles to be fitted.
       # parameters as:  Intensity,            NCP width,   NCP centre A-1
       # first  element:       H  (structural)                                                                                           
       # provide:      mass [amu],   intensity_range [arb. units], width_range [A-1], centre_range [us]; 
       # ranges as [low limit ,value, high limit]
       H = element( 1.0079, [0,1,None], [3.,4.5,6.], [-1.5,0.,0.5] )
       Al = element( 27.0, [0,1,None], [10.,15.5,30.], [-1.5,0.,0.5] )
       Zr = element( 91.22, [0,1,None], [10.,15.5,30.], [-1.5,0.,0.5] )
       elements = [H, Al, Zr]
       # constraint on the intensities of element peaks
       #provide LHS element, RHS element, mult. factor, flag
       # if flag=True inequality; if flag = False equality
       C1 = constraint( 0, 2, 2.*82.03/6.46, "eq")# the type can be either "eq" or "ineq"  
       constraints = [C1]

       # spectra to be masked
       spectra_to_be_masked = "173, 174, 181, 140"
       subtract_resonances = True
       resonance_function = 'name=Voigt,LorentzAmp=1.,LorentzPos=284.131,LorentzFWHM=2,GaussianFWHM=3;'

       fit_hydrogen_in_Y_space = True      # If True, corrected time-of-flight spectra containing H only are transformed to Y-space and fitted.
       y_fit_ties = "(c6=0., c4=0.)"

       ####### END OF USER INPUT

       #
       # Start of the reduction and analysis procedure
       #
       if load_data:
           spectrum_list=str(first_spectrum)+'-'+str(last_spectrum)
           LoadVesuvio(Filename=runs,SpectrumList=spectrum_list,Mode="SingleDifference",SumSpectra=False,InstrumentParFile=ipfile, OutputWorkspace=ws_name)
           Rebin(InputWorkspace=ws_name,Params=tof_range,OutputWorkspace=ws_name) # chose limits such that there is at list one non-nan bin among all the spectra between -30 and 30 \AA-1

       vertical_width, horizontal_width, thickness = 0.1, 0.1, 0.001 # expressed in meters
       create_slab_geometry(ws_name,vertical_width, horizontal_width, thickness)

       masses, par, bounds, constraints = prepare_fit_arguments(elements, constraints)
       fit_arguments = [bounds, constraints]

       # Iterative analysis and correction of time-of-flight spectra.
       for iteration in range(number_of_iterations):
           if iteration == 0:
               ws_to_be_fitted = CloneWorkspace(InputWorkspace = ws_name, OutputWorkspace = ws_name+"_cor")
           ws_to_be_fitted = mtd[ws_name+"_cor"] 
           MaskDetectors(Workspace=ws_to_be_fitted,SpectraList=spectra_to_be_masked)

           # Fit and plot where the spectra for the current iteration
           spectra, widths, intensities, centres = block_fit_ncp(par, first_spectrum,last_spectrum, masses, ws_to_be_fitted, fit_arguments, verbose)
    
           # Calculate mean widths and intensities
           mean_widths, mean_intensity_ratios = calculate_mean_widths_and_intensities(masses, widths, intensities, spectra, verbose) # at present is not multiplying for 0,9

           if (number_of_iterations - iteration -1 > 0):
               # evaluate gamma background correction ---------- This creates a background workspace with name :  str(ws_name)+"_gamma_background"
               sample_properties = calculate_sample_properties(masses, mean_widths, mean_intensity_ratios, "GammaBackground", verbose)
               correct_for_gamma_background(ws_name, first_spectrum,last_spectrum, sample_properties, verbose)# 
               Scale(InputWorkspace = str(ws_name)+"_gamma_background", OutputWorkspace = str(ws_name)+"_gamma_background", 
                                       Factor=0.9, Operation = "Multiply")
               # evaluate multiple scattering correction --------- This creates a background workspace with name :  str(ws_name)+"_MulScattering"
               if transmission_guess < 1. :
                   sample_properties = calculate_sample_properties(masses, mean_widths, mean_intensity_ratios, "MultipleScattering", verbose)
                   correct_for_multiple_scattering(ws_name, first_spectrum,last_spectrum, sample_properties, transmission_guess, 
                                                               multiple_scattering_order, number_of_events,verbose, masses,mean_intensity_ratios)
               # Create corrected workspace
               Minus(LHSWorkspace= ws_name, RHSWorkspace = str(ws_name)+"_gamma_background", 
                                   OutputWorkspace = ws_name+"_cor")
               if transmission_guess < 1. :
                   Minus(LHSWorkspace= ws_name+"_cor", RHSWorkspace = str(ws_name)+"_MulScattering", 
                                       OutputWorkspace = ws_name+"_cor")
               if subtract_resonances :
                   Minus(LHSWorkspace=ws_name+"_cor",RHSWorkspace=ws_name+"_cor_fit",
                                       OutputWorkspace=ws_name+"_cor_residuals")
                   ws = CloneWorkspace(ws_name+"_cor_residuals")
                   for index in range( ws.getNumberHistograms() ):
                       Fit(Function=resonance_function, InputWorkspace=ws_name+"_cor_residuals", WorkspaceIndex=index, MaxIterations=10000, 
                               Output=ws_name+"_cor_residuals", OutputCompositeMembers=True, StartX=110., EndX=460.)
                       fit_ws=mtd[ws_name+"_cor_residuals_Workspace"]
                       for bin in range( ws.blocksize() ) :
                           ws.dataY(index)[bin]=fit_ws.readY(1)[bin]
                           ws.dataE(index)[bin]=0.
                   RenameWorkspace(InputWorkspace="ws", OutputWorkspace=ws_name+"_fitted_resonances")
                   Minus(LHSWorkspace=ws_name+"_cor",RHSWorkspace=ws_name+"_fitted_resonances",
                                   OutputWorkspace=ws_name+"_cor" )
                   Minus(LHSWorkspace=ws_name+"_cor_residuals",RHSWorkspace=ws_name+"_fitted_resonances",
                                   OutputWorkspace=ws_name+"_cor_residuals" )
           else:
               if fit_hydrogen_in_Y_space:
                   hydrogen_ws = subtract_other_masses(ws_name+"_cor", widths, intensities, centres, spectra, masses)
                   RenameWorkspace("hydrogen_ws", ws_name+'_H')
                   SumSpectra(InputWorkspace=ws_name+'_H', OutputWorkspace=ws_name+'_H_sum')
                   calculate_mantid_resolutions(ws_name, masses[0])

       # Fit of the summed and symmetrised hydrogen neutron Compton profile in its Y space using MANTID.
       if fit_hydrogen_in_Y_space:
           #calculate_mantid_resolutions(ws_name, masses[0])
           max_Y = convert_to_y_space_and_symmetrise(ws_name+"_H",masses[0])
           # IT WOULD BE GOOD TO HAVE THE TIES DEFINED IN THE USER SECTION!!! 
           constraints = "   sigma1=3.0,c4=0.0, c6=0.0,A=0.08, B0=0.00, ties = {}".format(y_fit_ties)
           correct_for_offsets = True
           y_range = (-20., 20.)
           final_fit(ws_name+'_H_JoY_sym', constraints, y_range,correct_for_offsets, masses)

AlgorithmFactory.subscribe(vesuvioTest)
    
"""

    zrh2_H_JoY_sym_Parameters

    

    name          value       error

    f1.sigma     4.29559     0.02

    f1.c4         0              0

    f1.c6         0              0
    
    f1.A          0.09          0.0003

    f1.B0        8.8347e-5   0.00015

    
"""