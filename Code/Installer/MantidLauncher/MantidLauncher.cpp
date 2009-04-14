// 
// MantidLauncher.cpp : Launches MantidPlot if there aren't any updates available.
//                      If there are new updates MantidLauncher downloads them from http://download.mantidproject.org/updates 
//                      and installs.
//

#include "stdafx.h"
#include "Resource.h"
#include <windows.h>
#include <winhttp.h>
#include <commctrl.h>

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>


using namespace std;

class Cancel:public runtime_error
{
public:
    Cancel():runtime_error(""){}
};

// Download file from website ws and path fn, so that URI is http://<ws>/<fn> and saves it in local file lfn
void readFile(const wchar_t* ws,const wchar_t* fn,const string& lfn);
void readFile_tst(const string& url,const string& fn);

// Read version and product GUID from local file
void readVersion(const string& fn,int& ver_maj,int& ver_min, int& ver_buid, string& guid, int& fSize);

// Launches application with command line cmd. Returns straight away, not waiting for
// the application to finish.
void Launch(const string& cmd);

// Progress dialog function
BOOL CALLBACK ProgressDialogProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam);

// Starts Progress dialog in a separate thread
DWORD WINAPI ThreadProc(PVOID pParameter);

// Returns true if HKLM\Software\Mantid exists meaning Mantid is installed
bool ReadMantidRegValue(const std::string& valueName,std::string& value);

void mess(const string& str)
{
    MessageBoxA(0,str.c_str(),"Mantid Laincher",MB_OK);
}

// Global variables
HINSTANCE hInst;
HWND hwndPB;
HWND hwndDialog;
int fileSize = 35; // Size of Mantid.msi in MB?
bool cancelDownload = false;

int APIENTRY _tWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

    hInst = hInstance;

    // Get the temporary directory name
    char tmpBuff[1000];
    GetTempPathA(1000,tmpBuff);
    string tmpDir(tmpBuff);
    string newVersionFN(tmpDir+"mantid_version.txt");
    string msiFN(tmpDir+"Mantid.msi");

    string MantidInstallDir; 
    int ver_maj, ver_min, ver_build;
    string guid;
    string version;

    if (!ReadMantidRegValue("GUID",guid))
    {
        // Mantid is not installed

        if (IDYES == MessageBox(0,L"Mantid Installation was not found on this computer.\n\nWhould you like to download and install it?",
            L"Mantid Launcher",MB_YESNO | MB_ICONQUESTION))
        {
            DWORD dwThreadId;// Start progress bar dialog
            HANDLE hThread = CreateThread( NULL,0,ThreadProc, 0, 0, &dwThreadId );

            try // to download the installer
            {
                readFile(L"download.mantidproject.org",L"updates/Mantid.msi",msiFN);
            }
            catch(Cancel& c) // if canceled simply stop
            {
                return 0;
            }
            catch(runtime_error& e)
            {
                MessageBox(0,L"Unable to download Mantid installer",L"Mantid Launcher - Error",MB_OK);
                return 0;
            }

            string cmd = string("msiexec /i ")+msiFN;
            Launch(cmd); // launch the installer
            
        }
        return 0;
    }

    ReadMantidRegValue("InstallDir",MantidInstallDir);
    ReadMantidRegValue("Version",version);
    for(size_t i=0;i<version.size();i++)
        if ( version[i] == '.' ) version[i] = ' ';
    istringstream istr(version);
    istr >> ver_maj >> ver_min >> ver_build;

    // If an exception happens skip the updating part quietly and start MantidPlot
    try
    {
        // Download the new version file
        readFile(L"download.mantidproject.org",L"updates/mantid_version.txt",newVersionFN);
        int new_ver_maj, new_ver_min, new_ver_build;
        string new_guid;
        readVersion(newVersionFN,new_ver_maj, new_ver_min, new_ver_build, new_guid, fileSize);

        // Install new version
        if (new_ver_build > ver_build &&
            IDYES == MessageBoxA(NULL,"New Mantid version is ready.\n\nWould you like to install it?","Message",MB_YESNO | MB_ICONQUESTION))
        {
            
            DWORD dwThreadId;            
            HANDLE hThread = CreateThread( NULL,0,ThreadProc, 0, 0, &dwThreadId );

            readFile(L"download.mantidproject.org",L"updates/Mantid.msi",msiFN);

            string cmd;
            if (new_guid != guid) cmd = string("msiexec /i ")+msiFN;
            else
                cmd = string("msiexec /i ")+msiFN+" REINSTALL=ALL REINSTALLMODE=vomus";

            Launch(cmd);
             
            return 0;
        }

    }
    catch(...)
    {
        // Do nothing
    }

    // Launch MantidPlot
    Launch(MantidInstallDir + "bin/MantidPlot.exe");

	return 0;
}

void readFile(const wchar_t* ws,const wchar_t* fn,const string& lfn)
{
  DWORD dwSize = 0;
  DWORD dwDownloaded = 0;
  LPSTR pszOutBuffer;
  BOOL  bResults = FALSE;
  HINTERNET  hSession = NULL, 
             hConnect = NULL,
             hRequest = NULL;

  // Use WinHttpOpen to obtain a session handle.
  hSession = WinHttpOpen( L"Mantid update",  
                          WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                          WINHTTP_NO_PROXY_NAME, 
                          WINHTTP_NO_PROXY_BYPASS, 0 );

  WinHttpSetTimeouts(hSession,1000,1000,1000,1000);

  // Specify an HTTP server.
  if( hSession )
  {
      hConnect = WinHttpConnect( hSession, ws, 
                               INTERNET_DEFAULT_PORT, 0 );
  }

  // Create an HTTP request handle.
  if( hConnect )
  {
    hRequest = WinHttpOpenRequest( hConnect, L"GET", fn,
                                   NULL, WINHTTP_NO_REFERER, 
                                   WINHTTP_DEFAULT_ACCEPT_TYPES, 
                                   0 );
  }

  // Send the request.
  if( hRequest )
  {
    bResults = WinHttpSendRequest( hRequest,
                                   WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                                   WINHTTP_NO_REQUEST_DATA, 0, 
                                   0, 0 );
  }

  // End the request.
  if( bResults )
  {
    bResults = WinHttpReceiveResponse( hRequest, NULL );
  }

  ofstream ofil(lfn.c_str(),ios_base::binary);
  if (!ofil) 
  {
      throw runtime_error("");
  }
  // Keep checking for data until there is nothing left.
  if( bResults )
  {
      // Check that we receiving what we asked for. Status code must be 200
      DWORD dwStatusCode = 0;
      DWORD dwSize = sizeof(DWORD);
      bResults = WinHttpQueryHeaders( hRequest, 
                                      WINHTTP_QUERY_STATUS_CODE |
                                      WINHTTP_QUERY_FLAG_NUMBER,
                                      NULL, 
                                      &dwStatusCode, 
                                      &dwSize, 
                                      NULL );
      if (dwStatusCode != 200) throw runtime_error("");

    int nDownloaded = 0;
    do 
    {
        if ( cancelDownload ) throw Cancel();
      // Check for available data.
      dwSize = 0;
      if( !WinHttpQueryDataAvailable( hRequest, &dwSize ) )
        throw runtime_error("");

      // Allocate space for the buffer.
      pszOutBuffer = new char[dwSize+1];
      if( !pszOutBuffer )
      {
        throw runtime_error("");
      }
      else
      {
        // Read the data.
        ZeroMemory( pszOutBuffer, dwSize+1 );

        if( !WinHttpReadData( hRequest, (LPVOID)pszOutBuffer, dwSize, &dwDownloaded ) )
          throw runtime_error("");
        else
        {
            ofil.write(pszOutBuffer,dwDownloaded);
            nDownloaded += dwDownloaded;
            if (nDownloaded > fileSize * 10000)
            {
                SendMessage(hwndPB, PBM_STEPIT, 0, 0); 
                nDownloaded = 0;
            }
        }

      }
    } while( dwSize > 0 );
  }

  // Close any open handles.
  if( hRequest ) WinHttpCloseHandle( hRequest );
  if( hConnect ) WinHttpCloseHandle( hConnect );
  if( hSession ) WinHttpCloseHandle( hSession );

  // Report any errors.
  if( !bResults )
    throw runtime_error("");

}

// Read version and product GUID from local file
void readVersion(const string& fn,int& ver_maj,int& ver_min, int& ver_build, string& guid, int& fSize)
{
    ifstream fil(fn.c_str());
    if (!fil) throw runtime_error("");
    string ver;
    fil >> ver >> guid >> fSize;
    for(size_t i=0;i<ver.size();i++)
        if (ver[i] == '.') ver[i] = ' ';
    istringstream istr(ver);
    istr >> ver_maj >> ver_min >> ver_build;
}

void readFile_tst(const string& url,const string& fn)
{
    int n = 0;
    ifstream ifil(url.c_str(),ios_base::binary);
    ofstream ofil(fn.c_str(),ios_base::binary);
    if (!ifil || !ofil) throw runtime_error("");
    while(!ifil.eof())
    {
        if ( cancelDownload ) throw Cancel();
        char buff[1000];
        ifil.read(buff,1000);
        int nread = ifil.gcount();
        ofil.write(buff,nread);
        n += nread;
        if (n > fileSize * 10000)
        {
            SendMessage(hwndPB, PBM_STEPIT, 0, 0); 
            n = 0;
        }
    }
}

void Launch(const string& cmd)
{
    char *ccmd = new char[cmd.size()+1];
    strcpy_s(ccmd,cmd.size()+1,cmd.c_str());
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si,sizeof(STARTUPINFO));
    si.cb = sizeof(STARTUPINFO);
    ZeroMemory(&pi,sizeof(PROCESS_INFORMATION));
    CreateProcessA(NULL,ccmd,NULL,NULL,FALSE,0,NULL,NULL,&si,&pi);
}


BOOL CALLBACK ProgressDialogProc(HWND hwndDlg, 
                             UINT message, 
                             WPARAM wParam, 
                             LPARAM lParam) 
{ 
    switch (message) 
    { 
    case WM_INITDIALOG:
        hwndDialog = hwndDlg;
        hwndPB = GetDlgItem(hwndDlg,IDC_PROGRESS1);
        SendMessage(hwndPB, PBM_SETRANGE, 0,
            MAKELPARAM(0, 100)); 
        SendMessage(hwndPB, PBM_SETSTEP, (WPARAM) 1, 0); 
        return FALSE;
    case WM_COMMAND: 
        switch (LOWORD(wParam)) 
        { 
            case IDCANCEL: 
                cancelDownload = true;
                EndDialog(hwndDlg, wParam); 
                return TRUE; 
        } 
    } 
    return FALSE; 
} 

// Runs the dialog in a separate thread
DWORD WINAPI ThreadProc(PVOID pParameter)
{
    InitCommonControls(); 

    int res  = DialogBox(hInst,MAKEINTRESOURCE(IDD_DIALOG2),0,ProgressDialogProc);

    return (DWORD) res;
}                  
    
// Returns true if HKLM\Software\Mantid exists meaning Mantid is installed
bool ReadMantidRegValue(const std::string& valueName,std::string& value)
{
    const int maxSize = 256;
    char name[maxSize];
    DWORD sizeOfName = maxSize;
    const int maxData = 1000;
    char data[maxData];
    DWORD sizeOfData;
    DWORD type;
    HKEY hKey = HKEY_LOCAL_MACHINE;
    LONG ret;
    char subKey[] = "Software\\Mantid";

    ret = RegOpenKeyExA( hKey, subKey, NULL, KEY_READ ,&hKey);

    if (ret != ERROR_SUCCESS)
    {
        return false;
    }

    DWORD i = 0;
    while(ret == ERROR_SUCCESS)
    {
        sizeOfData = maxData;
        ret =  RegEnumValueA( hKey, i, name, &sizeOfName,  NULL, &type, (LPBYTE)&data, &sizeOfData );
        if (ret != ERROR_SUCCESS) return false;
        if (std::string(name) == valueName) 
        {
            value = std::string(data);
            break;
        }
        i++;
    }

    return true;
}
