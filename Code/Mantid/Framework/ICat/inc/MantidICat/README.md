## Generating gSoap files

Run the following command in order to generate a source declartion file from the SOAP endpoint:

    wsdl2h -o NameOfFile.h https://ICAT-uri?wsdl

The following command will generate classes and headers based on the source file generated above:

    soapcpp2 -i -C -I</path/to/gsoap/import> NameOfFile.h

### Possible issues

When running your code on OSX locally or on test servers you *will* come across problems with OSX trying to include windows.h files. To solve this issue, locate `windows.h` in `stdsoap2.h` and add the following code to the `WITH_OPENSSL` macro:

    # ifdef __APPLE__
      # undef OPENSSL_SYS_WINDOWS
      # undef OPENSSL_SYS_WIN32
    # endif
    
   
To prevent OSX errors when trying to compile (due to already included files) comment out the following two lines of code inside the second `#ifdef __APPLE__` macro: 

    extern "C" int isnan(double);
    extern "C" int isinf(double);
