#pylint: disable=invalid-name
import stresstesting
from mantid.simpleapi import *
from mantid import *
import os
import numpy as n
from abc import ABCMeta, abstractmethod

class PEARL_Reduction(stresstesting.MantidStressTest):
    '''Test adapted from actual script used by the scientists'''

    __metaclass__ = ABCMeta # Mark as an abstract class

    def __init__(self):
        stresstesting.MantidStressTest.__init__(self)
        self.attenfile = "PRL112_DC25_10MM_FF.OUT"
        self.tofbinning="1500,-0.0006,19900"
        self.calfile="pearl_offset_12_1.cal"
        self.groupfile="pearl_group_12_1_TT70.cal"
        self.vanfile="van_spline_TT70_cycle_12_1.nxs"
        self.cycle="12_1"
        self.instver="new2"
        self.mode="all"
        self.tt_mode="TT70"
        self.saved_outfile = ''
        self.saved_gssfile = ''
        self.reference_nexus = ''
        self.reference_gss = ''
        self.reference_workspace = ''

    def runTest(self):
        self.do_focus()

    def doValidation(self):
        '''Override doValidation to vaildate two things at the same time'''
		# reset validate() method to call validateNexus() instead
        self.validate = self.validateNexus
        res = self.validateWorkspaceToNeXus()
        if not res:
            return False
		# reset validate() method to call validateGSS() instead
        self.validate = self.validateGSS
        res = self.validateASCII()
        return res

    def cleanup(self):
        '''Remove temporary files'''
        if os.path.exists(self.saved_outfile):
            os.remove(self.saved_outfile)
        if os.path.exists(self.saved_gssfile):
            os.remove(self.saved_gssfile)

    @abstractmethod
    def do_focus(self):
        raise NotImplementedError("Implmenent do_focus to do actual test.")

    def validateNexus(self):
        '''Compare the result of reduction with the reference nexus file'''
        return self.reference_workspace,self.reference_nexus

    def validateGSS(self):
        '''Validate the created gss file'''
        from mantid.api import FileFinder
        return self.saved_gssfile,FileFinder.getFullPath(self.reference_gss)

    def PEARL_getlambdarange(self):
        return 0.03,6.00

    def PEARL_getmonitorspectrum(self, runno):
        return 1

    def PEARL_getfilename(self, run_number,ext):
        digit=len(str(run_number))

        numdigits=8
        filename="PEARL"

        for i in range(0,numdigits-digit):
            filename=filename+"0"

        filename+=str(run_number)+"."+ext
        return filename

    def PearlLoad(self, files,ext,outname):

        if type(files) is int:
            infile=self.PEARL_getfilename(files,ext)
            LoadRaw(Filename=infile,OutputWorkspace=outname,LoadLogFiles="0")
        else:
            loop=0
            num=files.split("_")
            frange=range(int(num[0]),int(num[1])+1)
            for i in frange:
                infile=self.PEARL_getfilename(i,ext)
                outwork="run"+str(i)
                LoadRaw(Filename=infile,OutputWorkspace=outwork,LoadLogFiles="0")
                loop=loop+1
                if loop == 2:
                    firstwk="run"+str(i-1)
                    secondwk="run"+str(i)
                    Plus(LHSWorkspace=firstwk,RHSWorkspace=secondwk,OutputWorkspace=outname)
                    mtd.remove(firstwk)
                    mtd.remove(secondwk)
                elif loop > 2:
                    secondwk="run"+str(i)
                    Plus(LHSWorkspace=outname,RHSWorkspace=secondwk,OutputWorkspace=outname)
                    mtd.remove(secondwk)
        return

    def PearlLoadMon(self, files,ext,outname):

        if type(files) is int:
            infile=self.PEARL_getfilename(files,ext)
            mspectra=self.PEARL_getmonitorspectrum(files)
            LoadRaw(Filename=infile,OutputWorkspace=outname,SpectrumMin=mspectra,SpectrumMax=mspectra,LoadLogFiles="0")
        else:
            loop=0
            num=files.split("_")
            frange=range(int(num[0]),int(num[1])+1)
            mspectra=self.PEARL_getmonitorspectrum(int(num[0]))
            for i in frange:
                infile=self.PEARL_getfilename(i,ext)
                outwork="mon"+str(i)
                LoadRaw(Filename=infile,OutputWorkspace=outwork,SpectrumMin=mspectra,SpectrumMax=mspectra,LoadLogFiles="0")
                loop=loop+1
                if loop == 2:
                    firstwk="mon"+str(i-1)
                    secondwk="mon"+str(i)
                    Plus(LHSWorkspace=firstwk,RHSWorkspace=secondwk,OutputWorkspace=outname)
                    mtd.remove(firstwk)
                    mtd.remove(secondwk)
                elif loop > 2:
                    secondwk="mon"+str(i)
                    Plus(LHSWorkspace=outname,RHSWorkspace=secondwk,OutputWorkspace=outname)
                    mtd.remove(secondwk)
        return



    def PEARL_getmonitor(self, number,ext,spline_terms=20):

        works="monitor"+str(number)
        self.PearlLoadMon(number,ext,works)
        ConvertUnits(InputWorkspace=works,OutputWorkspace=works,Target="Wavelength")
        lmin,lmax=self.PEARL_getlambdarange()
        CropWorkspace(InputWorkspace=works,OutputWorkspace=works,XMin=lmin,XMax=lmax)
        ex_regions=n.zeros((2,4))
        ex_regions[:,0]=[3.45,3.7]
        ex_regions[:,1]=[2.96,3.2]
        ex_regions[:,2]=[2.1,2.26]
        ex_regions[:,3]=[1.73,1.98]

        for reg in range(0,4):
            MaskBins(InputWorkspace=works,OutputWorkspace=works,XMin=ex_regions[0,reg],XMax=ex_regions[1,reg])

        SplineBackground(InputWorkspace=works,OutputWorkspace=works,WorkspaceIndex=0,NCoeff=spline_terms)
        return works


    def PEARL_read(self, number,ext,outname):
        self.PearlLoad(number,ext,outname)
        ConvertUnits(InputWorkspace=outname,OutputWorkspace=outname,Target="Wavelength")
        monitor=self.PEARL_getmonitor(number,ext,spline_terms=20)
        NormaliseToMonitor(InputWorkspace=outname,OutputWorkspace=outname,MonitorWorkspace=monitor,
                           IntegrationRangeMin=0.6,IntegrationRangeMax=5.0)
        ConvertUnits(InputWorkspace=outname,OutputWorkspace=outname,Target="TOF")
        mtd.remove(monitor)
        return

    def PEARL_focus(self, number,ext="raw",fmode="trans",ttmode="TT70",atten=True,van_norm=True):

        self.tt_mode=ttmode
        self.mode=fmode

        work="work"
        focus="focus"

        if type(number) is int:
            outfile="PRL"+str(number)+".nxs"
            gssfile="PRL"+str(number)+".gss"
            outwork="PRL"+str(number)
        else:
            outfile="PRL"+number+".nxs"
            gssfile="PRL"+number+".gss"
            outwork="PRL"+number

        self.PEARL_read(number,ext,work)
        Rebin(InputWorkspace=work,OutputWorkspace=work,Params=self.tofbinning)
        AlignDetectors(InputWorkspace=work,OutputWorkspace=work,CalibrationFile=self.calfile)
        DiffractionFocussing(InputWorkspace=work,OutputWorkspace=focus,GroupingFileName=self.groupfile)

        mtd.remove(work)

        for i in range(0,14):
            output="mod"+str(i+1)
            van="van"+str(i+1)
            rdata="rdata"+str(i+1)
            if van_norm:
                LoadNexus(Filename=self.vanfile,OutputWorkspace=van,EntryNumber=i+1)
                ExtractSingleSpectrum(InputWorkspace=focus,OutputWorkspace=rdata,WorkspaceIndex=i)
                Rebin(InputWorkspace=van,OutputWorkspace=van,Params=self.tofbinning)
                ConvertUnits(InputWorkspace=rdata,OutputWorkspace=rdata,Target="TOF")
                Rebin(InputWorkspace=rdata,OutputWorkspace=rdata,Params=self.tofbinning)
                Divide(LHSWorkspace=rdata,RHSWorkspace=van,OutputWorkspace=output)
                CropWorkspace(InputWorkspace=output,OutputWorkspace=output,XMin=0.1)
                Scale(InputWorkspace=output,OutputWorkspace=output,Factor=10)
            else:
                ExtractSingleSpectrum(InputWorkspace=focus,OutputWorkspace=rdata,WorkspaceIndex=i)
                ConvertUnits(InputWorkspace=rdata,OutputWorkspace=rdata,Target="TOF")
                Rebin(InputWorkspace=rdata,OutputWorkspace=output,Params=self.tofbinning)
                CropWorkspace(InputWorkspace=output,OutputWorkspace=output,XMin=0.1)

        mtd.remove(focus)

        if self.mode=="all":
            CloneWorkspace(InputWorkspace="mod1",OutputWorkspace="bank1")
            for i in range(1,9):
                toadd="mod"+str(i+1)
                Plus(LHSWorkspace="bank1",RHSWorkspace=toadd,OutputWorkspace="bank1")
            Scale(InputWorkspace="bank1",OutputWorkspace="bank1",Factor=0.111111111111111)
            SaveGSS(InputWorkspace="bank1",Filename=gssfile,Append=False,Bank=1)
            ConvertUnits(InputWorkspace="bank1",OutputWorkspace="bank1",Target="dSpacing")
            SaveNexus(Filename=outfile,InputWorkspace="bank1",Append=False)
            for i in range(0,5):
                tosave="mod"+str(i+10)
                SaveGSS(InputWorkspace=tosave,Filename=gssfile,Append=True,Bank=i+2)
                ConvertUnits(InputWorkspace=tosave,OutputWorkspace=tosave,Target="dSpacing")
                SaveNexus(Filename=outfile,InputWorkspace=tosave,Append=True)

            for i in range(0,14):
                output="mod"+str(i+1)
                van="van"+str(i+1)
                rdata="rdata"+str(i+1)
                mtd.remove(rdata)
                mtd.remove(van)
                mtd.remove(output)
            mtd.remove("bank1")

        elif self.mode=="groups":
            CloneWorkspace(InputWorkspace="mod1",OutputWorkspace="group1")
            CloneWorkspace(InputWorkspace="mod4",OutputWorkspace="group2")
            CloneWorkspace(InputWorkspace="mod7",OutputWorkspace="group3")
            for i in range(1,3):
                toadd="mod"+str(i+1)
                Plus(LHSWorkspace="group1",RHSWorkspace=toadd,OutputWorkspace="group1")
            Scale(InputWorkspace="group1",OutputWorkspace="group1",Factor=0.333333333333)
            for i in range(1,3):
                toadd="mod"+str(i+4)
                Plus(LHSWorkspace="group2",RHSWorkspace=toadd,OutputWorkspace="group2")
            Scale(InputWorkspace="group2",OutputWorkspace="group2",Factor=0.333333333333)
            for i in range(1,3):
                toadd="mod"+str(i+7)
                Plus(LHSWorkspace="group3",RHSWorkspace=toadd,OutputWorkspace="group3")
            Scale(InputWorkspace="group3",OutputWorkspace="group3",Factor=0.333333333333)
            Plus(LHSWorkspace="group2",RHSWorkspace="group3",OutputWorkspace="group23")
            Scale(InputWorkspace="group23",OutputWorkspace="group23",Factor=0.5)
            SaveGSS("group1",Filename=gssfile,Append=False,Bank=1)
            ConvertUnits(InputWorkspace="group1",OutputWorkspace="group1",Target="dSpacing")
            SaveNexus(Filename=outfile,InputWorkspace="group1",Append=False)
            SaveGSS(InputWorkspace="group2",Filename=gssfile,Append=True,Bank=2)
            ConvertUnits(InputWorkspace="group2",OutputWorkspace="group2",Target="dSpacing")
            SaveNexus(Filename=outfile,InputWorkspace="group2",Append=True)
            SaveGSS(InputWorkspace="group3",Filename=gssfile,Append=True,Bank=3)
            ConvertUnits(InputWorkspace="group3",OutputWorkspace="group3",Target="dSpacing")
            SaveNexus(Filename=outfile,InputWorkspace="group3",Append=True)
            SaveGSS(InputWorkspace="group23",Filename=gssfile,Append=True,Bank=4)
            ConvertUnits(InputWorkspace="group23",OutputWorkspace="group23",Target="dSpacing")
            SaveNexus(Filename=outfile,InputWorkspace="group23",Append=True)
            for i in range(0,3):
                tosave="mod"+str(i+10)
                SaveGSS(InputWorkspace=tosave,Filename=gssfile,Append=True,Bank=i+5)
                ConvertUnits(InputWorkspace=tosave,OutputWorkspace=tosave,Target="dSpacing")
                SaveNexus(Filename=outfile,InputWorkspace=tosave,Append=True)
            for i in range(0,14):
                output="mod"+str(i+1)
                van="van"+str(i+1)
                rdata="rdata"+str(i+1)
                mtd.remove(rdata)
                mtd.remove(van)
                mtd.remove(output)
            mtd.remove("group1")
            mtd.remove("group2")
            mtd.remove("group3")
            mtd.remove("group23")

        elif self.mode=="trans":
            CloneWorkspace(InputWorkspace="mod1",OutputWorkspace="bank1")
            for i in range(1,9):
                toadd="mod"+str(i+1)
                Plus(LHSWorkspace="bank1",RHSWorkspace=toadd,OutputWorkspace="bank1")
            Scale(InputWorkspace="bank1",OutputWorkspace="bank1",Factor=0.111111111111111)
            if atten:
                ConvertUnits(InputWorkspace="bank1",OutputWorkspace="bank1",Target="dSpacing")
                CloneWorkspace(InputWorkspace="bank1",OutputWorkspace=outwork+"_noatten")
                self.PEARL_atten("bank1","bank1")
                ConvertUnits(InputWorkspace="bank1",OutputWorkspace="bank1",Target="TOF")

            SaveGSS(InputWorkspace="bank1",Filename=gssfile,Append=False,Bank=1)
            ConvertUnits(InputWorkspace="bank1",OutputWorkspace="bank1",Target="dSpacing")
            SaveNexus(Filename=outfile,InputWorkspace="bank1",Append=False)
            for i in range(0,9):
                tosave="mod"+str(i+1)
                ConvertUnits(InputWorkspace=tosave,OutputWorkspace=tosave,Target="dSpacing")
                SaveNexus(Filename=outfile,InputWorkspace=tosave,Append=True)

            for i in range(0,14):
                output="mod"+str(i+1)
                van="van"+str(i+1)
                rdata="rdata"+str(i+1)
                mtd.remove(rdata)
                mtd.remove(van)
                mtd.remove(output)
            mtd.remove("bank1")

        elif self.mode=="mods":
            for i in range(0,12):
                output="mod"+str(i+1)
                van="van"+str(i+1)
                rdata="rdata"+str(i+1)
                if i==0:
                    SaveGSS(InputWorkspace=output,Filename=gssfile,Append=False,Bank=i+1)
                    ConvertUnits(InputWorkspace=output,OutputWorkspace=output,Target="dSpacing")
                    SaveNexus(Filename=outfile,InputWorkspace=output,Append=False)
                else:
                    SaveGSS(InputWorkspace=output,Filename=gssfile,Append=True,Bank=i+1)
                    ConvertUnits(InputWorkspace=output,OutputWorkspace=output,Target="dSpacing")
                    SaveNexus(Filename=outfile,InputWorkspace=output,Append=True)

            mtd.remove(rdata)
            mtd.remove(van)
            mtd.remove(output)

        else:
            print "Sorry I don't know that mode", mode
            return

        LoadNexus(Filename=outfile,OutputWorkspace=outwork)

		# temporary nxs file to be deleted on cleanup
        self.saved_outfile = os.path.join(config['defaultsave.directory'],outfile)
		# temporary gss file to be deleted on cleanup
        self.saved_gssfile = os.path.join(config['defaultsave.directory'],gssfile).replace('.gss','-0.gss')
		# name of the reference nxs file which is the same as outfile
        self.reference_nexus = outfile.replace('PRL','PEARL')
		# name of the reference gss file
        self.reference_gss = gssfile.replace('.gss','-0.gss').replace('PRL','PEARL')
		# workspace to be compared with reference_nexus
        self.reference_workspace = outwork

    def PEARL_atten(self, work,outwork):
        PearlMCAbsorption(Filename=self.attenfile,OutputWorkspace="wc_atten")
        ConvertToHistogram(InputWorkspace="wc_atten",OutputWorkspace="wc_atten")
        RebinToWorkspace(WorkspaceToRebin="wc_atten",WorkspaceToMatch=work,OutputWorkspace="wc_atten")
        Divide(LHSWorkspace=work,RHSWorkspace="wc_atten",OutputWorkspace=outwork)
        mtd.remove("wc_atten")
        return

#================================================================================
class PEARL_Mode_trans(PEARL_Reduction):
    def do_focus(self):
		#self.reference_nexus = "PRL75318_75323.nxs"
        return self.PEARL_focus("75318_75323","raw",fmode="trans",ttmode="TT70",atten=True)

    def doValidation(self):
        '''Validate an additional workspace'''
        res = PEARL_Reduction.doValidation(self)
        if not res:
            return False
        self.validate = self.validateNoAtten
        return self.validateWorkspaceToNeXus()

    def validateNoAtten(self):
        return 'PRL75318_75323_noatten','PEARL75318_75323_noatten.nxs'

#================================================================================
class PEARL_Mode_all_Si(PEARL_Reduction):
    def do_focus(self):
		#self.reference_nexus = "PRL74798_74800.nxs"
        return self.PEARL_focus("74798_74800","raw",fmode="all",ttmode="TT70",atten=False)

#================================================================================
class PEARL_Mode_all_CeO2(PEARL_Reduction):
    def do_focus(self):
		#self.reference_nexus = "PRL74795_74797.nxs"
        return self.PEARL_focus("74795_74797","raw",fmode="all",ttmode="TT70",atten=False)

#================================================================================
