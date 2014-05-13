#define _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_DEPRECATE

#include <vector>
#include <cstring>
#include <cstdlib>
#include <algorithm>

#include "../apex-utils/apex_utils.h"

std::vector<const char*> data, order;

int main( int argc, char *argv[] ){
	if( argc < 3 ) {
        printf("Usage: filein order out\n"\
               "\treorder filein using order provided in order file\n"\
               "\torder file is a file with order(start from 0) in first column\n" ); 
        
        return -1;
    }

    FILE *fi = apex_utils::fopen_check( argv[1], "rb" );
    fseek( fi, 0L, SEEK_END );
    size_t sz = (size_t)ftell( fi );
    char *ptr = new char[ sz + 1 ];
    fseek( fi, 0L, SEEK_SET );
    apex_utils::assert_true( fread( ptr, sizeof(char), sz, fi ) > 0, "load data" );            
    fclose( fi );

    // split
    data.push_back( ptr+0 );
    for( size_t i = 0; i < sz - 1; i ++ ){
        if( ptr[i] == '\n' || ptr[i] == '\r' ){
            ptr[i] = '\0'; 
            if( data.back() == ptr + i ){
                data.back() = ptr + i + 1;
            }else{
                data.push_back( ptr + i + 1 );
            }
        }        
    }
    if( ptr[ sz-1 ] == '\n' || ptr[ sz-1 ] == '\r' ){
        ptr[ sz-1 ] = '\0';
        if( data.back() == ptr + sz - 1 ) data.pop_back();
    }else{
        ptr[ sz ] = '\0';
        if( data.back() == ptr + sz ) data.pop_back();
    }

    printf("all the data loaed in, %u lines, start reorder\n", (unsigned)data.size() );    
    FILE *fp = apex_utils::fopen_check( argv[2], "r" );    
    unsigned oid;
    while( fscanf( fp, "%u%*[^\n]\n", &oid ) == 1 ){
        apex_utils::assert_true( static_cast<size_t>(oid) < data.size(), "invalid order file" );
        order.push_back( data[oid] );
                                 
    }
    fclose( fp );
    
    FILE *fo = apex_utils::fopen_check( argv[3], "w" );
    for( size_t i = 0; i < order.size(); i ++ ){
        fprintf( fo,"%s\n", order[i] ); 
    }
    fclose( fo );
    
    delete []ptr;
    return 0;
}
