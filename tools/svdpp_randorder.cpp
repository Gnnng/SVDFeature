#define _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_DEPRECATE

#include <vector>
#include <cstring>
#include <cstdlib>
#include <algorithm>

#include "../apex-tensor/apex_random.h"
#include "../apex-tensor/apex_tensor.h"
#include "../apex-utils/apex_utils.h"

struct Elem{
    unsigned uid, line;
    inline static bool cmp_uid( const Elem &a, const Elem &b ){
        return a.uid < b.uid;
    }
};
struct Group{
    Elem *header;
    unsigned num;
};
std::vector<Elem>  data;
std::vector<Group> group;

int main( int argc, char *argv[] ){
	if( argc < 3 ) {
        printf("Usage: filein out [seed] [column]\n"\
               "\trandom order the lines while grouping the same user together\n"\
               "\tinput file must contain [uid] in first column\n"\
               "\toutput a file with 2 columns, format: [order] [uid]\n"\
               ); return -1;
    }
    if( argc > 3 ){
        apex_random::seed( atoi(argv[3]) );
    }else{
        apex_random::seed( 10 );
    }
    int col = argc > 4 ? atoi( argv[4] ) : 0;

    
    Elem e;
    unsigned line = 0;
    FILE *fi = apex_utils::fopen_check( argv[1], "r" );
    while( true ){
        if( col == 0 ){
            if( fscanf( fi, "%u%*[^\n]\n", &e.uid ) != 1 ) break;
        }else{
            if( fscanf( fi, "%*u%u%*[^\n]\n", &e.uid ) != 1 ) break;
        }
        e.line = line ++;
        data.push_back( e );
    }
    fclose( fi );
    
    std::sort( data.begin(), data.end(), Elem::cmp_uid );
    // start segmentation
    for( size_t i = 0; i < data.size(); ){
        size_t j = i;
        while( j < data.size() && data[j].uid ==  data[i].uid ) j ++;
        Group g;
        g.header = &data[i];
        g.num    = static_cast<unsigned>(j - i);
        i = j;
        group.push_back( g );
    }
    // in group shuffle
    for( size_t i = 0; i < group.size(); i ++ ){
        apex_random::shuffle( group[i].header, static_cast<size_t>( group[i].num ) );
    }
    // group shuffle
    apex_random::shuffle( group );

    // output result
    FILE *fo = apex_utils::fopen_check( argv[2], "w" );
    for( size_t i = 0; i < group.size(); i ++ ){
        for( unsigned j = 0; j < group[i].num; j ++ )
            fprintf( fo, "%u\t%u\n", group[i].header[j].line , group[i].header[j].uid );
    }
    fclose( fo );
    return 0;
}
