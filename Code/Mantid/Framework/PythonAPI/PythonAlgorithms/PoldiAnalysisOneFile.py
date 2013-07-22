"""*WIKI* 


*WIKI*"""
from mantid.api import PythonAlgorithm, registerAlgorithm, MatrixWorkspaceProperty
from mantid.api import FileProperty, FileAction
from mantid.kernel import Direction
from mantid.simpleapi import *
from pylab import *
import math
import numpy as np
import os.path



PI=math.pi
TWOPI = 2*PI

CONVLAMV = 3956.034*1000.
CONVKV = CONVLAMV / TWOPI


class PoldiAnalysisOneFile(PythonAlgorithm):
    
    def category(self):
        """ Mantid required
        """
        return "Poldi\old"

    def name(self):
        """ Mantid required
        """
        return "PoldiAnalysisOneFile"

    def PyInit(self):
        """ Mantid required
        """
        self.declareProperty(FileProperty(name="Filename",defaultValue="",action=FileAction.Load, extensions = ["hdf","h5"]))
#         self.declareProperty(WorkspaceProperty("OutputWorkspaceName", "", direction=Direction.Output), "Name of the output Workspace")
#          self.declareProperty("OutputWorkspaceName", 'toto', doc="Name of the output Workspace")

        self.declareProperty("wlenmin", 1.1, doc = 'minimal wavelength considered' , direction = Direction.Input)
        self.declareProperty("wlenmax", 5.0, doc = 'maximal wavelength considered' , direction = Direction.Input)
        
        
        self.declareProperty("BadWiresThreshold", 0.8, doc = 'Bad wires threshold parameter' , direction = Direction.Input)
        
        
#         self.declareProperty(MatrixWorkspaceProperty("CorrelationWorkspace", "correlation", direction=Direction.Output), "Correlation data workspace")
    
    def path_leaf(path):
        head, tail = ntpath.split(path)
        return tail
    
    def PyExec(self):
        """ Mantid required
        """
        self.log().warning('Poldi Data Analysis ---- add one file')
        
        filename = self.getProperty("Filename").value
        wlen_min  = self.getProperty("wlenmin").value
        wlen_max  = self.getProperty("wlenmax").value
        
        bad_wires_threshold  = self.getProperty("BadWiresThreshold").value
        
        
        head, sampleName = os.path.split(filename)
        sampleName = os.path.splitext(sampleName)[0]
        self.log().error('Poldi - sample %s' %(sampleName))
        
        
        self.log().debug('Poldi - load data')
        LoadSINQFile(Instrument="POLDI", 
                     Filename=filename, 
                     OutputWorkspace=sampleName)
        
        ws = mtd[sampleName]

#         run = ws.getRun()
#         all_logs = run.getLogData() # With no argument it returns all logs
#         for ll in run.getProperties():
#             self.log().error(' ***** %s' %(ll.value))
        
        dictsearch='/home/christophe/poldi/dev/mantid-2.3.2-Source/PSIScripts'+"/mantidpoldi.dic"
        sampleNameLog = "%sLog" %sampleName
        PoldiLoadLog(Filename=filename, 
                     Dictionary=dictsearch, 
                     PoldiLog=sampleNameLog)
        
        self.log().debug('Poldi - load detector')
        LoadInstrument(Workspace=ws, 
                       InstrumentName="Poldi", 
                       RewriteSpectraMap=True)
        
        
        self.log().debug('Poldi - dead wires')
        PoldiRemoveDeadWires(InputWorkspace=ws, 
                             RemoveExcludedWires=True, 
                             AutoRemoveBadWires=True, 
                             BadWiresThreshold=bad_wires_threshold, 
                             PoldiDeadWires="PoldiDeadWires")
        
        
        self.log().debug('Poldi - chopper slits')
        PoldiLoadChopperSlits(InputWorkspace=ws, 
                              PoldiChopperSlits="PoldiChopperSlits")
        
        
        self.log().debug('Poldi - spectra')
        PoldiLoadSpectra(InputWorkspace=ws, 
                         PoldiSpectra="PoldiSpectra")
        
        
        self.log().debug('Poldi - IPP')
        PoldiLoadIPP(InputWorkspace=ws, 
                     PoldiIPP="PoldiIPP")
        
        
        sampleNameCorr = "%sCorr" %sampleName
        sampleNameLog = "%sLog" %sampleName
        self.log().debug('Poldi - analysis')
        PoldiAutoCorrelation(Filename=filename, 
                             InputWorkspace=ws, 
                             PoldiSampleLogs=sampleNameLog, 
                             PoldiDeadWires=mtd["PoldiDeadWires"], 
                             PoldiChopperSlits=mtd["PoldiChopperSlits"], 
                             PoldiSpectra=mtd["PoldiSpectra"], 
                             PoldiIPP=mtd["PoldiIPP"], 
                             PoldiAutoCorrelation=sampleNameCorr)
        
        sampleNamePeak = "PoldiPeak"
        
        PoldiPeakDetection(InputWorkspace=sampleNameCorr,
                           OutputWorkspace=sampleNamePeak)
            
            
#         fit_fct = "name=Gaussian, PeakCentre=4.6, Height=10, Sigma=0.5"
#         fit_input_ws = sampleNameCorr
#         fit_WorkspaceIndex = 0
# #             fit_StartX = 
# #             fit_EndX = 
#         fit_minimizer = "Levenberg-Marquardt" 
#         fit_costfct = "Least squares"
#             
#         result = Fit(Function=fit_fct,InputWorkspace=fit_input_ws,WorkspaceIndex=fit_WorkspaceIndex,Output="toto")
#         
#         self.log().debug('Poldi - fit %s'%result[0])
#         self.log().debug('Poldi - fit %s'%result[1])
#         self.log().debug('Poldi - fit %s'%result[2])
#         self.log().debug('Poldi - fit %s'%result[3])
#         self.log().debug('Poldi - fit %s'%result[4])
#         
            
        
#         ws_corr = mtd["PoldiAutoCorrelation"]
#         self.log().error('Poldi - analysis %d %d' % (len(ws_corr.column(0)), len(ws_corr.column(1))))
#         plot(ws_corr.column(0), ws_corr.column(1))
#         xs = [x for (x) in ws_corr.column(0)]
#         ys = [y for (y) in ws_corr.column(1)]
#         t = np.newTable("MyTable",len(xs),2)
#         for i in range(len(xs)):
#             t.setCell(0, i, xs(i))
#             t.setCell(1, i, ys(i))
#         g1 = plot(t, (0,1), 0)
        
        
#         
#         
#         poldi_IPP_ws = mtd['PoldiIPP']
#         poldi_IPP_data = poldi_IPP_ws.column(1)
#         
#         
#          
# 
#          
# ########---------------------------------------------------------------
# #
# #  chopper configuration
# #     
#         ws_chopper_slits = mtd["PoldiChopperSlits"]
#         nb_chopper_slits = len(ws_chopper_slits.column(1))
#         chopper_slits_pos = np.zeros(nb_chopper_slits+2)
#         chopper_slits_pos[0:nb_chopper_slits] = ws_chopper_slits.column(1)
#         
#         chopper_rot_speed = poldi_IPP_data[7]
#         time_chopper_tcycle=60./(4.*chopper_rot_speed)*1.e6    # tcycle
#         
#         chopper_slits_pos *= time_chopper_tcycle
#         
#         self.log().error('')
#         self.log().error('Poldi - chopper conf --------------  ')
#         self.log().error('Poldi -        nb_chopper_slits           %d' % nb_chopper_slits)
#         self.log().error('Poldi -        chopper_rot_speed          %.2f' % chopper_rot_speed)
#         self.log().error('Poldi -        time_chopper_tcycle        %.2f' % time_chopper_tcycle)
#         self.log().error('Poldi -        chopper_slits_pos          ' + repr(chopper_slits_pos).replace('array',' '))
# 
# 
# 
# ########---------------------------------------------------------------
# #
# #  time configuration
# #        
#         time_channels = ws.readX(1)
# 
#         time_delta_t = time_channels[1] - time_channels[0]
#         time_offset = time_channels[0]
#         
#         
#         time_t0 = poldi_IPP_data[5]
#         time_tconst =  poldi_IPP_data[6]
#         
#         time_t0=time_t0*time_chopper_tcycle+time_tconst
#         
#         self.log().error('')
#         self.log().error('Poldi - time conf ------------------------  ')
#         self.log().error('Poldi -        time_delta_t               %.2f' % time_delta_t)
#         self.log().error('Poldi -        time_offset                %.2f' % time_offset  )
#         self.log().error('Poldi -        time_tconst                %.2f' % time_tconst  )
#         self.log().error('Poldi -        time_t0                    %.2f' % time_t0  )
#         
#          
# 
# ########---------------------------------------------------------------
# #
# #  Detector configuration
# #  
#         dist_chopper_sample = poldi_IPP_data[0]
#         dist_sample_detector = poldi_IPP_data[1]
#         pos_x0_det = poldi_IPP_data[2]
#         pos_y0_det = poldi_IPP_data[3]
#         ang_twotheta_det_deg = poldi_IPP_data[4]
#         ang_twotheta_det = ang_twotheta_det_deg*PI/180.
#         dist_detector_radius =  poldi_IPP_data[8]              # rdet
#         nb_det_channel =  int(poldi_IPP_data[9])                    # 400
#         nb_time_channels = len(time_channels)             
#         indice_mid_detector = int((1+nb_det_channel)/2.)     # mitteldet
#         ang_det_channel_resolution =  poldi_IPP_data[10]
#         ang_det_resolution =  poldi_IPP_data[11]               # resdet
#         
#         
#         self.log().error('')
#         self.log().error('Poldi - setup conf -----------------------  ')
#         self.log().error('Poldi -        dist_chopper_sample        %d mm' % dist_chopper_sample)
#         self.log().error('Poldi -        dist_sample_detector       %d mm' % dist_sample_detector)
#         self.log().error('Poldi -        dist_detector_radius       %d mm' % dist_detector_radius)
#         self.log().error('Poldi -')
#         self.log().error('Poldi -        nb_det_channel             %d' % nb_det_channel)
#         self.log().error('Poldi -        indice_mid_detector        %d' % indice_mid_detector)
#         self.log().error('Poldi -        nb_time_channels           %d' % nb_time_channels)
#         self.log().error('Poldi -')
#         self.log().error('Poldi -        pos_x0_det                 %.2f mm' % pos_x0_det)
#         self.log().error('Poldi -        pos_y0_det                 %.2f mm' % pos_y0_det)
#         self.log().error('Poldi -        ang_twotheta_det_deg       %.2f deg' % ang_twotheta_det_deg)
#         self.log().error('Poldi -        ang_det_channel_resolution %.2f' % ang_det_channel_resolution)
#         self.log().error('Poldi -        ang_det_resolution         %.2f' % ang_det_resolution)
#         
# #         if(pos_x0_det < 0):
# #             ang_alpha1 = math.atan2(pos_y0_det, pos_x0_det)
# #         elif(pos_x0_det > 0):
# #             ang_alpha1 = PI + math.atan2(pos_y0_det, pos_x0_det)
#         ang_alpha1 = math.atan2(pos_y0_det, pos_x0_det)
#         self.log().error('Poldi -        ang_alpha1                 %.2f deg' % (ang_alpha1*180./PI))
#             
#         ang_alpha_sample = PI + ang_alpha1 - ang_twotheta_det
#         self.log().error('Poldi -        ang_alpha_sample           %.2f deg' % (ang_alpha_sample*180./PI))
#         
#         dist_sms = math.sqrt(pos_x0_det*pos_x0_det + pos_y0_det*pos_y0_det)
#         self.log().error('Poldi -        dist_sms                   %.2f mm' % dist_sms)
#         
#         ang_phi_det_mittel = math.asin(dist_sms / dist_detector_radius * math.sin(ang_alpha_sample))
#         self.log().error('Poldi -        ang_phi_det_mittel         %.2f deg' % (ang_phi_det_mittel*180./PI))
#         
#         ang_phiges = nb_det_channel * ang_det_channel_resolution / dist_detector_radius  # shoulb be = 2.5
#         self.log().error('Poldi -        ang_phiges                 %.2f deg' % (ang_phiges*180./PI))
#         
#         ang_phi2anf = (ang_twotheta_det - ang_phiges/2.) - math.asin(dist_sms/dist_detector_radius * math.sin(ang_alpha_sample))
#         self.log().error('Poldi -        ang_phi2anf                %.2f deg' % (ang_phi2anf*180./PI))
#         
#         
#         
# ########---------------------------------------------------------------
# #
# #  dead wires configuration
# #     
#         self.log().error('')
#         self.log().error('Poldi - dead wires conf --------------  ')
#         ws_dead_wires = mtd["PoldiDeadWires"]
#         list_dead_wires = ws_dead_wires.column(0)
#         nb_dead_wires = len(ws_dead_wires.column(0))
#         self.log().error('Poldi -        nb_dead_wires              %d' % nb_dead_wires)
#         
#         table_dead_wires = np.zeros(nb_det_channel, np.bool)
#         for dw in list_dead_wires:
#             table_dead_wires[dw-1] = True
# #             self.log().error(' dw %d %d %d' % (dw, dw-1, table_dead_wires[dw-1]))
#             
# #         for i in range(nb_det_channel):
# #             if(table_dead_wires[i]):
# #                 self.log().error(' %d %d' % (i+1, table_dead_wires[i]))
#         
#         
# 
# ########---------------------------------------------------------------
# #
# #  count configuration
# #        
#         self.log().error('')
#         self.log().error('Poldi - detector count acquisition ------- ')
#         nhe3 = np.zeros((nb_det_channel,nb_time_channels))
#         for i in range(nb_det_channel):
#             nhe3[i,:] = ws.readY(i)
#         self.log().error('Poldi -        dim tps                     %d' % len(nhe3))
#         self.log().error('Poldi -        dim wires                   %d' % len(nhe3[0]))
#         
#         
# #*****  Calculating the number of time elements ******
#         self.log().error('')
#         self.log().error('Poldi - time elements conf --------------- ')
#         ati = time_chopper_tcycle / time_delta_t
#         nti = int(ati+0.01)
#         self.log().error('Poldi -        ati                        %d' % ati)
#         self.log().error('Poldi -        nti                        %d' % nti)
#       
# #****** Calculate the sample scattering angle and distance from the sample for each element of the detector
#         ang_phim = ang_alpha1
#         
#         ang_pw_from_center = np.zeros(nb_det_channel)
#         ang_pw_for_sample = np.zeros(nb_det_channel)
#         dist_from_sample = np.zeros(nb_det_channel)
#         
#         for i in range(nb_det_channel):
#             ang_phi2det = ang_phi2anf+(i+0.5)*ang_det_resolution/(dist_detector_radius)
#             help1 = dist_detector_radius*math.sin(ang_phi2det)-dist_sms*math.sin(ang_phim)
#             help2 = dist_detector_radius*math.cos(ang_phi2det)-dist_sms*math.cos(ang_phim)
#             ang_phi2samp = math.atan(help1/help2)
#             if(ang_phi2samp < 0):
#                 ang_phi2samp = PI + ang_phi2samp
#             dist_s2det = math.sqrt(dist_detector_radius*dist_detector_radius+dist_sms*dist_sms - 2.*dist_detector_radius*dist_sms*math.cos(ang_phi2det-ang_phim))
#             ang_pw_from_center[i] = ang_phi2det
#             ang_pw_for_sample[i] = ang_phi2samp
#             dist_from_sample[i] = dist_s2det
#             
# #*****  End calculation detector parameters  
#   
# #*****  Calculate the integrated intensity
# #       summdet=0.
# #       iex=1
# #       do i=1,nzell
# #           if(listexcl(iex).ne.i)then
# #               do j=1,nti
# #                   summdet=summdet+nhe3(i,j)
# #               end do
# #           else
# #               iex=iex+1
# #           end if
# #       end do      
# 
# 
# # ****    Calculation of the various values of Q
#         self.log().error('')
#         self.log().error('Poldi - diffraction calibration ---------- ')
# 
#         qmin=2.*(2.*PI/wlen_max) * math.sin(ang_pw_for_sample[0]/2.)
#         qmax=2.*(2.*PI/wlen_min) * math.sin(ang_pw_for_sample[nb_det_channel-1]/2.)
#         
#         self.log().error('Poldi -        wlen_min                   %0.2f' % wlen_min)
#         self.log().error('Poldi -        wlen_max                   %0.2f' % wlen_max)
#         self.log().error('Poldi -        qmin                       %0.2f' % qmin)
#         self.log().error('Poldi -        qmax                       %0.2f' % qmax)
#         
#         vqmin = CONVKV*qmin / (2.*math.sin(ang_pw_for_sample[indice_mid_detector]/2.))
#         dist_samp_mid_detector = dist_chopper_sample + dist_from_sample[indice_mid_detector]
#         self.log().error('Poldi -        dist_samp_mid_detector     %0.2f' % dist_samp_mid_detector)
# 
#         dspace2 = CONVKV / (2.*dist_samp_mid_detector*math.sin(ang_pw_for_sample[indice_mid_detector]/2.))
#         dspace2 = dspace2 * time_delta_t *1.e-6 * TWOPI
#         n0_dspace = int(TWOPI/qmax/dspace2)
#         dspace1 = n0_dspace*dspace2
#         n1_dspace = int(TWOPI/qmin/dspace2)
#         
#         nd_space = n1_dspace-n0_dspace
#         self.log().error('Poldi -        nd_space                   %d' % nd_space)
# 
# 
# # ***   Calculate what time an n arrives for a lattice spacing of 1 A
# 
#         time_TOF_for_1A = np.zeros(nb_det_channel)
#         
#         for i in range(nb_det_channel):
#             time_TOF_for_1A[i] = 2.* math.sin(ang_pw_for_sample[i]/2.) * (dist_chopper_sample+dist_from_sample[i]) /CONVKV*1.e6/TWOPI
# #             self.log().debug('    loop  %d %f'% (i, time_TOF_for_1A[i-1]) )
# 
# 
# 
# 
#         self.log().error('Poldi Data Analysis ---- analyse')
# 
#         self.log().error('')
#         self.log().error('Poldi - analysis ------------------------- ')
#         summdbr=0.
#         for i in range(nb_det_channel):
#             if(not(table_dead_wires[i])):
#                 summdbr += time_TOF_for_1A[i] * dspace2 * nd_space
#         summdbr /= time_delta_t
#         self.log().error('Poldi -        summdbr                     %0.2f' % summdbr)
# 
# 
#         
#         sudbr2=0
#         subr = np.zeros(nd_space)
#         dint = np.zeros(nd_space)
#         for i in range(nd_space):
#             iex=1
#             for j in range(nb_det_channel):
#                 if(not(table_dead_wires[j])):
#                     subr[i] += time_TOF_for_1A[j] * dspace2
#                 else:
#                     iex += 1
#             subr[i] /= time_delta_t
# #             if(subr(i).lt.0.000001)write(6,*)'mist:',i, subr(i)
#             if(subr[i] < 0.000001):
#                 subr[i] = 0.000001
#             sudbr2=sudbr2+subr[i]
#         self.log().error('Poldi -        sudbr2                      %0.2f' % sudbr2)
# 
# 
# 
# 
# 
# #*******  Calculation of the deviation of the measured value ***********
# 
#         deld = np.zeros(nd_space)
#         wzw = np.zeros(nd_space)
#         for i in range(nd_space):
#             self.log().error('Poldi -        analysis                %d / %d' % (i,nd_space))
#             cmess = np.zeros(nb_chopper_slits)
#             csigm = np.zeros(nb_chopper_slits)
#             for j2 in range(nb_chopper_slits):
#                 ttd=0
#                 if(j2>=2):
#                     ttd=chopper_slits_pos[j2]
#                       
#                 for j in range(nb_det_channel):
#                     if(not(table_dead_wires[j])):
# # ****  Calculation of the sensing elements to be considered
#                         center = time_t0 + time_TOF_for_1A[j]*(dspace1+float(i)*dspace2)
#                         center /= time_delta_t
#                         center += -int(center/float(nti))*nti+1.
#                         width = time_t0 + time_TOF_for_1A[j]*dspace2/time_delta_t
# 
#                         if(j2>=2):
#                             center += ttd/time_delta_t
#                         cmin = center-width/2.
#                         cmax = center+width/2.
#                         icmin = int(cmin+5.)-5      #!+5/-5 falls cmin negativ
#                         icmax = int(cmax+5.)-5
# 
#                         if(icmin<1):
#                             iicmin = icmin+nti
#                         elif(icmin>nti):
#                             iicmin = icmin-nti
#                         else:
#                             iicmin = icmin
#                             
#                         if((iicmin<1) or (iicmin>nti)):
#                             self.log().error("icmin,iicmin, nti : %.2f %.2f %.2f" %(icmin,iicmin, nti))
#                         if(icmax>nti):
#                             iicmax = icmax-nti
#                         else:
#                             iicmax = icmax
# 
#                         if(icmax == icmin):
#                             cmess[j2] = cmess[j2] + nhe3[j,iicmin]*width /float(max(1,nhe3[j,iicmin]))
#                             csigm[j2] = csigm[j2] + width/float(max(1,nhe3[j,iicmin]))
#                         else:
#                             cmess[j2] = cmess[j2] + nhe3[j,iicmin]*(icmin-cmin+1.) /float(max(1,nhe3[j,iicmin]))
#                             cmess[j2] = cmess[j2] + nhe3[j,iicmax]*(cmax-icmax) /float(max(1,nhe3[j,iicmax]))
#                             csigm[j2] = csigm[j2] + (icmin-cmin+1.)/float(max(1,nhe3[j,iicmin]))
#                             csigm[j2] = csigm[j2] + (cmax-icmax)/ float(max(1,nhe3[j,iicmax]))
#                             if(icmax == (icmin+2)):
#                                 if(icmin>=0):
#                                     if(icmin<=nti-1):
#                                         cmess[j2] = cmess[j2] +nhe3[j,icmin+1]*1. /float(max(1,nhe3[j,icmin+1]))
#                                         csigm[j2] = csigm[j2] +1./float(max(1,nhe3[j,icmin+1]))
#                                     else:
#                                         cmess[j2] = cmess[j2] +nhe3[j,icmin+1-nti]*1./float(max(1,nhe3[j,icmin+1-nti]))
#                                         csigm[j2] = csigm[j2] +1./float(max(1,nhe3[j,icmin+1-nti]))
#                                 else:
#                                     cmess[j2] = cmess[j2] +nhe3[j,icmin+1+nti]*1. /float(max(1,nhe3[j,icmin+1+nti]))
#                                     csigm[j2] = csigm[j2] +1./float(max(1,nhe3[j,icmin+1+nti]))
# #                             if(icmax>icmin+2):
# #                                   self.log().error(' Zu breit !!!!')
#                   
#   
# 
#             csigm0 = 30000
#             for j2 in range(nb_chopper_slits):
#                 csigm0 = min(csigm0,csigm[j2])
# 
#             deld[i] = 0
#             if(csigm0>0.001):
#                 ivzwzw = 0
#                 for j2 in range(nb_chopper_slits):
#                     wzw[j2] = cmess[j2]/csigm[j2]
#                     if(wzw[j2]>0):
#                         ivzwzw = ivzwzw+1
#                     if(wzw[j2]<0):
#                         ivzwzw = ivzwzw-1
#                 if(abs(ivzwzw) == nb_chopper_slits):
#                     for j2 in range(nb_chopper_slits):
#                         deld[i] += 1./wzw[j2]
#                     deld[i] = float(nb_chopper_slits*nb_chopper_slits)/deld[i] * subr[i]
#                     
#     #! Ende i-Schleife
# 
# 
# 
# 
#         sudeld = 0
#         for i in range(nd_space):
#             sudeld = sudeld+deld[i]
#         diffdel -= summdet
# #** Dividing the difference in the individual Q values
#         for i in range(nd_space):
#             deld[i] -= diffdel*subr[i]/sudbr2
#               
#         for i in range(nd_space):
#             sukor += deld[i]
# 
#         self.log().error('integr. counts detector, 1st. approach %.2f %.2f'%(summdet,sukor))
# 
#         qi = np.zeros(nd_space)
#         for i in range(nd_space):
#             dint[i] = deld[i]
#             deld[i] = 0
#             qi[i] = 2.*pi/(dspace1+float(i)*dspace2)
# 
# 
# 
# 
# 
#         self.log().error('Poldi Data Analysis ---- end')
#         

















registerAlgorithm(PoldiAnalysisOneFile())
