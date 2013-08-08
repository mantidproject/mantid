## Generating gSoap files

Run the following command in order to generate a source declartion file from the SOAP endpoint:

    wsdl2h -o NameOfFile.h https://ICAT-uri?wsdl

The following command will generate classes and headers based on the source file generated above:

    soapcpp2 -i -C -I</path/to/gsoap/import> NameOfFile.h

