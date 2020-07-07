


#include <stdio.h>
#include <stdlib.h>
#include <zlib.h>
#include <vector>
#include <string>

using namespace std;

/*
*       libz GZIP stream compress example
*       Custom data format compress
*/
class ZFileEncode{
public:
    ZFileEncode(){
        memset(&stream_, 0, sizeof(stream_));
    }

    virtual ~ZFileEncode(){
        close();
    }

    bool open(const string& file){

        close();

        fp_ = fopen(file.c_str(), "wb");
        if(fp_ == nullptr)
            return false;

        stream_.zalloc = (alloc_func)0;
        stream_.zfree = (free_func)0;
        stream_.opaque = (voidpf)0;
        if(deflateInit2(&stream_, Z_DEFAULT_COMPRESSION, Z_DEFLATED, -MAX_WBITS, 8, Z_DEFAULT_STRATEGY) != Z_OK){
            close();
            return false;
        }

        output_buffer_.resize(4096 * 100);
        stream_.next_out = (Bytef*)output_buffer_.data();
        stream_.avail_out = output_buffer_.size();
        open_finish_ = true;
        return true;
    }

    bool write(const void* data, unsigned int length){

        if(!write_raw(&length, sizeof(length))) 
            return false;

        if(!write_raw(data, length))
            return false;
        
        return true;
    }

    void close(){

        if(open_finish_)
            write_end();

        if(fp_){
            fclose(fp_);
            fp_ = nullptr;
        }

        stream_.next_out = nullptr;
        stream_.avail_out = 0;
        output_buffer_.clear();
        open_finish_ = false;
    }

    size_t total_out() const{
        return stream_.total_out;
    }

private:
    bool write_raw(const void* data, size_t length){

        if(!open_finish_) return false;

        stream_.next_in = (Bytef*)data;
        stream_.avail_in = length;

        while (stream_.avail_in > 0){
            
            write_fragment();
            if(deflate(&stream_, Z_NO_FLUSH) != Z_OK) 
                return false;
        }
        return true;
    }

    void write_fragment(bool endof = false){

        if(!endof){
            if(stream_.avail_out == 0){
                fwrite(output_buffer_.data(), 1, output_buffer_.size(), fp_);
                stream_.next_out = (Bytef*)output_buffer_.data();
                stream_.avail_out = output_buffer_.size();
            }
        }else{
            int length = output_buffer_.size() - stream_.avail_out;
            if(length > 0){
                fwrite(output_buffer_.data(), 1, length, fp_);
            }
        }
    }

    bool write_end(){

        int err = 0;
        while(true){
            write_fragment();
            if((err = deflate(&stream_, Z_FINISH)) == Z_STREAM_END) 
                break;

            if(err != Z_OK) 
                return false;
        }

        write_fragment();
        if(deflateEnd(&stream_) != Z_OK) 
            return false;
        
        write_fragment(true);
        return true;
    }

private:
    z_stream stream_;
    FILE* fp_ = nullptr;
    vector<char> output_buffer_;
    bool open_finish_ = false;
};


class ZFileDecode{
public:
    virtual ~ZFileDecode(){
        close();
    }

    bool open(const string& file){

        close();

        fp_ = fopen(file.c_str(), "rb");
        if(fp_ == nullptr)
            return false;

        stream_.zalloc = (alloc_func)0;
        stream_.zfree = (free_func)0;
        stream_.opaque = (voidpf)0;
        if(inflateInit2(&stream_, -MAX_WBITS) != Z_OK){
            close();
            return false;
        }

        input_buffer_.resize(4096 * 100);
        open_finish_ = true;
        return true;
    }

    bool read(vector<char>& output){

        if(!read_fragment())
            return false;
        
        unsigned int length = 0;
        stream_.next_out = (Bytef*)&length;
        stream_.avail_out = sizeof(length);

        int err = 0;
        while(true){
            if(!read_fragment())
                return false;

            err = inflate(&stream_, Z_NO_FLUSH);
            if(err != Z_OK){
                printf("decode length err = %d\n", err);
                return false;
            }
            
            if(stream_.avail_out == 0)
                break;
        }

        output.resize(length);
        stream_.next_out = (Bytef*)output.data();
        stream_.avail_out = output.size();

        while(true){
            if(!read_fragment())
                break;

            err = inflate(&stream_, Z_NO_FLUSH);
            if(err != Z_OK && err != Z_STREAM_END){
                printf("decode data err = %d\n", err);
                return false;
            }

            if(stream_.avail_out == 0)
                return true;
        }

        while(true){
            if((err = deflate(&stream_, Z_FINISH)) == Z_STREAM_END) 
                break;
        }
        inflateEnd(&stream_);

        if(stream_.avail_out == 0)
            return true;

        return false;
    }

    void close(){

        if(open_finish_)
            inflateEnd(&stream_);

        if(fp_){
            fclose(fp_);
            fp_ = nullptr;
        }

        stream_.next_in = nullptr;
        stream_.avail_in = 0;
        stream_.next_out = nullptr;
        stream_.avail_out = 0;
        input_buffer_.clear();
        open_finish_ = false;
    }

private:
    bool read_fragment(){
        if(stream_.avail_in == 0){
            int rlen = fread(input_buffer_.data(), 1, input_buffer_.size(), fp_);
            if(rlen == 0)
                return false;

            stream_.next_in = (Bytef*)input_buffer_.data();
            stream_.avail_in = rlen;
        }
        return true;
    }

private:
    z_stream stream_;
    FILE* fp_ = nullptr;
    vector<char> input_buffer_;
    bool open_finish_ = false;
};

void encode_test(){

    ZFileEncode encoder;

    if(!encoder.open("encode.z")){
        printf("encoder open fail.\n");
        return;
    }

    int int_array[] = {1, 3, 3};
    float float_array[] = {5, 5, 8};
    string str_data = "hello libz compress, this is repeat string, this is repeat string";

    encoder.write(int_array, sizeof(int_array));
    encoder.write(float_array, sizeof(float_array));
    encoder.write(str_data.data(), str_data.size());
    encoder.close();


    size_t raw_size = sizeof(int_array) + sizeof(float_array) + str_data.size();
    printf("Encode data, raw size: %d bytes, compress size: %d bytes\n", raw_size, encoder.total_out());

    for(int i = 0; i < 3; ++i)
        printf("int_array[%d] = %d\n", i, int_array[i]);

    for(int i = 0; i < 3; ++i)
        printf("float_array[%d] = %f\n", i, float_array[i]);

    printf("str_data = [%s]\n", str_data.c_str());
    printf("Encode finish.\n"
        "============================================="
        "\n");
}

void decode_test(){


    ZFileDecode decoder;

    if(!decoder.open("encode.z")){
        printf("decoder open fail.\n");
        return;
    }

    int int_array[3];
    float float_array[3];
    string str_data;

    vector<char> buffer;
    for(int i = 0; i < 3; ++i){
        if(!decoder.read(buffer)){
            printf("decode item fail.");
            return;
        }

        switch(i){
        case 0:
            memcpy(int_array, buffer.data(), buffer.size());
            break;
        case 1:
            memcpy(float_array, buffer.data(), buffer.size());
            break;
        case 2:
            str_data.resize(buffer.size());
            memcpy((char*)str_data.data(), buffer.data(), buffer.size());
            break;
        }
    }

    printf("Decode result.\n");
    for(int i = 0; i < 3; ++i)
        printf("int_array[%d] = %d\n", i, int_array[i]);

    for(int i = 0; i < 3; ++i)
        printf("float_array[%d] = %f\n", i, float_array[i]);

    printf("str_data = [%s]\n", str_data.c_str());
}

int main(){

    encode_test();
    decode_test();
    return 0;
}





