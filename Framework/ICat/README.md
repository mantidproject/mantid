## Generating gSoap files

Previously, the SOAP endpoint would be used directly by the `wsdl2h` command. However, there is a [bug in glassfish](https://groups.google.com/forum/#!searchin/icatgroup/wsdl/icatgroup/Una77-JHVWY/OwoM59B-_R4J) that prevents certain SOAP data from being transmitted to ICAT correctly, for example, datasets related to a datafile.

The ICAT developers have released a [script](https://code.google.com/p/icatproject/source/browse/icat/trunk/client/wsdl.sh?r=2377) to address this, which generates a `wsdl` and `xsd` file with modified attributes. To generate these files run the script against the endpoint, for example:

```sh
$ ./wsdl.sh https://icatisis.esc.rl.ac.uk
```

To generate the source declartion file needed by `soapcpp2` you need to run `wsdl2h` against both the `xsd` file and the `wsdl`:

```sh
// Note: the "ICATCompat" related commands and files in the ICAT provided script are not required.
$ wsdl2h -qICat4 -o ICat4Service.h ICAT.xsd ICAT.wsdl
```

The following command will generate classes and headers based on the source file generated above:

```sh
$ soapcpp2 -i -x -C -qICat4 -I</path/to/gsoap/import> ICat4Service.h
```

### Possible issues

#### *OSX warnings*

When running your code on OSX locally or on test servers you *will* come across problems with OSX trying to include windows.h files. To solve this issue, locate `windows.h` in `stdsoap2.h` and add the following code to the `WITH_OPENSSL` macro:

```cpp
# ifdef __APPLE__
  # undef OPENSSL_SYS_WINDOWS
  # undef OPENSSL_SYS_WIN32
# endif
```

______________________________________________________________________

To prevent OSX errors when trying to compile (due to already included files) comment out the following two lines of code inside the second `#ifdef __APPLE__` macro:

```cpp
extern "C" int isnan(double);
extern "C" int isinf(double);
```

______________________________________________________________________

#### *Namespace and linker issues*

For namespaces to work correctly three new files were generated from an empty file. This was achieved using the following commands:

```sh
touch soapserializers.h
soapcpp2 -psoapserializers soapserializers.h
```

The generated files contained the default SOAP Header and Fault serialization codes. This was needed as the GSoap library could not access these methods from inside the ICat4 namespace.
