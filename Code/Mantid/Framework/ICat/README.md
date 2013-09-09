## Generating gSoap files

Run the following command in order to generate a source declartion file from the SOAP endpoint:

    wsdl2h -qNameOfNameSpace -o NameOfFile.h https://ICAT-uri?wsdl
    #Example: wsdl2h -qICat4 -o ICat4Service.h https://icatisis.esc.rl.ac.uk/ICATService/ICAT?wsdl

The following command will generate classes and headers based on the source file generated above:

    soapcpp2 -i -x -C -qNameOfNameSpace -I</path/to/gsoap/import> NameOfFile.h
    #Example: soapcpp2 -i -x -C -qICat4 -I/home/gsoap-2.8/gsoap/import ICat4Service.h
    
### Possible issues
#### *OSX warnings*

When running your code on OSX locally or on test servers you *will* come across problems with OSX trying to include windows.h files. To solve this issue, locate `windows.h` in `stdsoap2.h` and add the following code to the `WITH_OPENSSL` macro:

    # ifdef __APPLE__
      # undef OPENSSL_SYS_WINDOWS
      # undef OPENSSL_SYS_WIN32
    # endif

---------------------------------------

To prevent OSX errors when trying to compile (due to already included files) comment out the following two lines of code inside the second `#ifdef __APPLE__` macro: 

    extern "C" int isnan(double);
    extern "C" int isinf(double);

---------------------------------------
#### *Namespace and linker issues*

For namespaces to work correctly three new files were generated from an empty file. This was achieved using the following commands:

    touch soapserializers.h
    soapcpp2 -psoapserializers soapserializers.h
    
The generated files contained the default SOAP Header and Fault serialization codes. This was needed as the GSoap library could not access these methods from inside ICat3/4 namespaces.
