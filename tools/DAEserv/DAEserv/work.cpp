#include "stdafx.h"

#include "isisds_command.h"

#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <string>
#include <vector>
#include <valarray>
#include <ctime>
#include <fstream>
#include <boost/math/special_functions/erf.hpp>

using namespace std;
// Need to link with Ws2_32.lib, Mswsock.lib, and Advapi32.lib

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "6789"

#define FINISH( s ) cout<<s<<"\n"; \
                    closesocket(ClientSocket); \
                    WSACleanup(); \
                    return 1; \

bool read_command(SOCKET s);
int peakFun(int i,float x, int t, float dc);

//ofstream ofil("C:\\Mantid\\Code\\DAEserv\\release\\tmp.txt");

struct Workspace
{
    typedef float real_t;
    Workspace()
    {

        ifstream fil("configDAE.txt");
        if (!fil)
        {
            numberOfSpectra = 3;
            numberOfBins = 100;
            x_start = 0.01;
            x_end = 2000;
        }
        else
        {
            fil>>numberOfSpectra
                >>numberOfBins
                >>x_start
                >>x_end;
        }
        numberOfPeriods = 1;
        alpha = (x_end-x_start)/(numberOfBins+1);
        peakStep = (x_end-x_start)/(numberOfSpectra);

        y.resize(numberOfSpectra);
        x.resize(numberOfBins+1);
        float dx = alpha;
        x[0] = x_start;
        for(size_t i=1;i<x.size();i++)
        {
            if (alpha < 0) dx = -x[i-1]*alpha;
            x[i] = x[i-1] + dx;
        }
        for(int i=0;i<numberOfSpectra;i++)
        {
            y[i].resize(numberOfBins+1);
            for(size_t j=0;j<y[i].size();j++)
            {
                y[i][j] = peakFun(i,x[j],0,peakStep);
            }
        }
        ndet = numberOfSpectra;
        udet.resize(ndet);
        spec.resize(ndet);
        for(int i=0;i<ndet;i++)
        {
            udet[i] = 1000 + i + 1;
            spec[i] = i+1;
        }
    }
    valarray< real_t > x;
    vector< valarray<int> > y; 
    int numberOfSpectra;
    int numberOfPeriods;
    int numberOfBins;
    int ndet;
    valarray<int> udet,spec;
    float x_start;
    float x_end;
    float alpha;
    float peakStep;
};


struct ThreadData
{
    SOCKET s;
    DWORD ID;
    HANDLE thread;
    time_t start_time;
    bool done;
};

// Thread functions
DWORD WINAPI workingThread(LPVOID p);
DWORD WINAPI updatingThread(LPVOID p);

// Global variables
Workspace workspace;
vector<ThreadData*> thData;
bool updatingData = false;
bool readingData = false;
HANDLE updatingThreadHandle = NULL;

DWORD WINAPI startService(LPVOID p)
{

    WSADATA wsaData;
    SOCKET ListenSocket = INVALID_SOCKET,
           ClientSocket = INVALID_SOCKET;
    struct addrinfo *result = NULL,
                    hints;
//    char recvbuf[DEFAULT_BUFLEN];
    int iResult;//, iSendResult;
    int recvbuflen = DEFAULT_BUFLEN;
    

    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed: %d\n", iResult);
        return 1;
    }

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    // Resolve the server address and port
    iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
    if ( iResult != 0 ) {
        printf("getaddrinfo failed: %d\n", iResult);
        WSACleanup();
        return 1;
    }

    updatingThreadHandle = CreateThread(NULL,0,&updatingThread,NULL,0,NULL);

    for(;;)
    {
        // Create a SOCKET for connecting to server
        ListenSocket = socket(result->ai_family, result->ai_socktype, 0);
        if (ListenSocket == INVALID_SOCKET) {
            printf("socket failed: %ld\n", WSAGetLastError());
            freeaddrinfo(result);
            WSACleanup();
            return 1;
        }

        // Setup the TCP listening socket
        iResult = bind( ListenSocket, result->ai_addr, (int)result->ai_addrlen);
        if (iResult == SOCKET_ERROR) {
            printf("bind failed: %d\n", WSAGetLastError());
            freeaddrinfo(result);
            closesocket(ListenSocket);
            WSACleanup();
            return 1;
        }

//        freeaddrinfo(result);

        iResult = listen(ListenSocket, SOMAXCONN);
        if (iResult == SOCKET_ERROR) {
            printf("listen failed: %d\n", WSAGetLastError());
            closesocket(ListenSocket);
            WSACleanup();
            return 1;
        }

        // Accept a client socket
        ClientSocket = accept(ListenSocket, NULL, NULL);
        if (ClientSocket == INVALID_SOCKET) {
            printf("accept failed: %d\n", WSAGetLastError());
            closesocket(ListenSocket);
            WSACleanup();
            return 1;
        }

        // No longer need server socket
        closesocket(ListenSocket);

        //ofil<<"Number of running threads "<<thData.size()<<'\n';
        ThreadData *td = new ThreadData;
        td->s = ClientSocket;
        td->done = false;
        time_t curr_time = time(NULL);
        td->start_time = curr_time;
        thData.push_back(td);
        td->thread = CreateThread(NULL,0,&workingThread,td,0,&td->ID);
        for(vector<ThreadData*>::iterator t=thData.begin();t!=thData.end();t++)
        {
            if ((**t).done)
            {
                delete *t;
                t = thData.erase(t);
            }
            double diff = difftime(curr_time,(**t).start_time);
            //ofil<<(**t).ID<<" diff="<<diff<<endl;
            // thread unfinished after 60 seconds is treated as failed and terminated
            if(diff > 2. && !(**t).done)
            {
                cerr<<"Terminate "<<(**t).ID<<endl;
                TerminateThread((**t).thread,0);
                closesocket((**t).s);
                delete *t;
                t = thData.erase(t);
            }
        }

    }


    WSACleanup();
    for(vector<ThreadData*>::iterator t=thData.begin();t!=thData.end();t++)
        delete *t;
   return 0;
}

bool read_command(SOCKET s)
{
    char comm_buffer[256];
    int comm_buff_size = sizeof(comm_buffer);
//    ISISDSDataType ret_type;
    int sv_dims_array[1] = { 1 }, sv_ndims = 1;   // used for rading single values with IDC routines
//    int dims_array[2];

    isisds_command_header_t comm;
    ZeroMemory(&comm,sizeof(comm));
    int stat = recv(s, (char*)&comm, sizeof(comm), 0);
    if (stat < 0) return false;
    if (comm.len < 0) return false;
    //ofil<<" Command "<<comm.command<<' '<<comm.len<<'\n';
    //ofil<<" Type "<<comm.type<<' '<<isisds_type_name[comm.type]<<'\n';
    //ofil<<" NDims "<<comm.ndims<<'\n';
    //for(int i=0;i<comm.ndims;i++)
    //    ofil<<"    dim_"<<i<<"= "<<comm.dims_array[i]<<endl;//*/
    stat = recv(s, comm_buffer, comm_buff_size, 0);
    //ofil<<"Data size="<<stat<<endl;
    if (stat < 0) return false;
    if (comm.type == ISISDSChar)
    {
        comm_buffer[stat] = '\0';
        string name(comm_buffer);
        //ofil<<name<<endl;
        if (name == "NSP1")
        {
            isisds_send_command(s, "OK", &workspace.numberOfSpectra, ISISDSInt32, sv_dims_array, sv_ndims);
            return true;
        }

        if (name == "NPER")
        {
            isisds_send_command(s, "OK", &workspace.numberOfPeriods, ISISDSInt32, sv_dims_array, sv_ndims);
            return true;
        }
        
        if (name == "NTC1")
        {
            isisds_send_command(s, "OK", &workspace.numberOfBins, ISISDSInt32, sv_dims_array, sv_ndims);
            return true;
        }
        
        if (name == "RTCB1")
        {
            int dim = (int)workspace.x.size();
            int n = isisds_send_command(s, "OK", &workspace.x[0], ISISDSReal32, &dim, sv_ndims);
            return true;
        }

        if (name == "NAME")
        {
            int dim = 7;
            char instr[] = "DAESERV";
            int n = isisds_send_command(s, "OK", instr, ISISDSChar, &dim, sv_ndims);
            return true;
        }

        if (name == "CNT1")
        {
            int dim = (int)workspace.x.size()*workspace.y.size();
            int *data = new int[dim];
            int mv_dims_array[1], mv_ndims = 1;
            mv_dims_array[0] = dim;
            int k = 0;
            for(size_t i=0;i<workspace.y.size();i++)
                for(size_t j=0;j<workspace.y[i].size();j++)
                    data[k++] = workspace.y[i][j];
            isisds_send_command(s, "OK", data, ISISDSInt32, mv_dims_array, mv_ndims);
            delete[] data;
            return true;
        }
        
        if (name == "RRPB")
        {
            int dim = 32;
            float rpb[32];
            rpb[8] = 0.1;
            int n = isisds_send_command(s, "OK", rpb, ISISDSReal32, &dim, sv_ndims);
            return true;
        }

        if (name == "NDET")
        {
            isisds_send_command(s, "OK", &workspace.ndet, ISISDSInt32, sv_dims_array, sv_ndims);
            return true;
        }

        if (name == "UDET")
        {
            int dim = workspace.ndet;
            isisds_send_command(s, "OK", &workspace.udet[0], ISISDSInt32,&dim, sv_ndims);
            return true;
        }
        
        if (name == "SPEC")
        {
            int dim = workspace.ndet;
            isisds_send_command(s, "OK", &workspace.spec[0], ISISDSInt32, &dim, sv_ndims);
            return true;
        }
        
    }
    else if (comm.type == ISISDSInt32)
    {
        //ofil<<"Reading data\n";
        int *idat = (int*)comm_buffer;
        int n = comm.dims_array[0];
        //for(int i=0;i<n;i++)
        //    ofil<<"int("<<i<<")="<<idat[i]<<'\n';
       int dim = workspace.numberOfBins+1;
       int i = idat[0]-1;
       if (i < 0 || i >= dim)
       {
           cerr<<"Spectrum number out of range\n";
           return false;
       }
       int res = isisds_send_command(s, "OK", &workspace.y[i][0], ISISDSInt32, &dim, sv_ndims);
       return true;
    }
    return false;
}

DWORD WINAPI workingThread(LPVOID p){

    if (updatingData) WaitForSingleObject(updatingThreadHandle,10000);
    readingData = true;

    ThreadData& data = *(ThreadData*)p;
    SOCKET ClientSocket = data.s;

        ISISDSAccessMode access_type;
        int stat = isisds_recv_open(ClientSocket, &access_type);
        if (stat <= 0)
        {
            readingData = false;
            FINISH("Couldnt connect")
        }


        // Receive until the peer shuts down the connection
        bool ok = true;
        do {

            //ofil<<"Running\n";
            ok = read_command(ClientSocket);

        } while (ok);

        // shutdown the connection since we're done
        int iResult = shutdown(ClientSocket, SD_SEND);
        if (iResult == SOCKET_ERROR) {
            printf("shutdown failed: %d\n", WSAGetLastError());
            closesocket(ClientSocket);
            WSACleanup();
            readingData = false;
            return 1;
        }//*/

        // cleanup
        closesocket(ClientSocket);

        //ofil<<"Stopping "<<data.ID<<endl;

        data.done = true;
        readingData = false;
  return 0;
}


int peakFun(int i,float xx, int t, float dc)
{
   double x = double(xx);
   double I=10*t;
   double a=2.;
   double b=0.03;
   double c=100 + double(dc)*i;
   double s=8;
   double bk=8;
   double s2 = s*s;
   double p1 = a/2*(a*s2+2*(x-c));
   double p2 = (a*s2+(x-c))/sqrt(2*s2);
   double p3 = b/2*(b*s2-2*(x-c));
   double p4 = (b*s2-(x-c))/sqrt(s*s2);
   if (p1 > 400) p1 = 400;
   if (p1 > 400) p1 = 400;
   if (p1 < -400) p1 = -400;
   if (p3 > 400) p3 = 400;
   if (p3 < -400) p3 = -400;
   double res = I*(exp(p1)*boost::math::erfc(p2)+ exp(p3)*boost::math::erfc(p4))+bk;
   return int(res);
}

DWORD WINAPI updatingThread(LPVOID p){

    HANDLE timer = CreateWaitableTimerA(NULL,FALSE,"tim");
    if (timer == NULL)
        return -1;

    LARGE_INTEGER tstart = {0,0};
    if (!SetWaitableTimer(timer,&tstart,10000,NULL,NULL,TRUE))
        return -1;

    for(int t=0;;t++)
    {
        if (t > 300) t = 0;
        if (WaitForSingleObject(timer,20000) == WAIT_FAILED)
            return -1;

        if (readingData) continue;
        updatingData = true;
        int length = workspace.numberOfBins+1;
        for(int i=0;i<workspace.numberOfSpectra;i++)
        {
            for(size_t j=0;j<length;j++)
            {
                workspace.y[i][j] = peakFun(i,workspace.x[j],t,workspace.peakStep);
            }
        }
        updatingData = false;

    }
    return 0;
}
