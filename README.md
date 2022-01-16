# fakefile
Deliberately vulnerable toy Linux driver for testing fuzzers. Note that unlike a "real" filesytem driver, this one does not handle the user of file stream wrappers such as fopen/fread/fwrite etc. Clients should use the lower level open/read/write API instead.
