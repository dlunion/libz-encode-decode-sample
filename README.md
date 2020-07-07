# libz-encode-decode-sample
libz compress encode decode



# Startup

* Download libz source code.  [download](https://zlib.net/).  1.2.11 etc.

  ```bash
  > tar -zxvf zlib-1.2.11.tar.gz
  > cd zlib-1.2.11
  > mkdir build && cd build
  
  # cmake create makefile
  > cmake ..
  > make all -j64
  > sudo make install
  ```

  

* make

  ```bash
  cd libz-encode-decode-sample
  make run
  ```



# Encode Example

``` c++
ZFileEncode encoder;
encoder.open("encode.z");

int int_array[] = {1, 3, 3};
float float_array[] = {5, 5, 8};
string str_data = "hello libz compress, this is repeat string, this is repeat string";

encoder.write(int_array, sizeof(int_array));
encoder.write(float_array, sizeof(float_array));
encoder.write(str_data.data(), str_data.size());
encoder.close();
```



# Decode Example

```c++
ZFileDecode decoder;
decoder.open("encode.z");

int int_array[3];
vector<char> buffer;
decoder.read(buffer);
memcpy(int_array, buffer.data(), buffer.size());

for(int i = 0; i < 3; ++i)
  printf("int_array[%d] = %d\n", int_array[i]);
```

