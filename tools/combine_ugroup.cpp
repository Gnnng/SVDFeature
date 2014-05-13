#define _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_DEPRECATE

#include <ctime>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <algorithm>


#include "../apex-utils/apex_utils.h"
#include "../apex_svd_data.h"

using namespace apex_svd;

struct Header{
    FILE *fi;
    int   tmp_num;
    int   base;
    int   num_feat;
    // whether it's dense format
    bool  is_dense;
	bool  warned;

	Header( void ){ this->warned = false; this->is_dense = false; }
	inline void check_base( unsigned findex ){
		if( findex >= (unsigned)num_feat && ! warned ) {
			fprintf( stderr, "warning:some feature exceed bound, num_feat=%d\n", num_feat );
			warned = true;
		}
	}
};


inline int norm( std::vector<Header> &vec, int base = 0 ){
    int n = base;
    for( size_t i = 0; i < vec.size(); i ++ ){
        if( vec[i].is_dense ) vec[i].num_feat = 1;
        vec[i].base = n; n += vec[i].num_feat;
    }
    return n;        
}

inline void vclose( std::vector<Header> &vec ){
    for( size_t i = 0; i < vec.size(); i ++ ){
        fclose( vec[i].fi );
    }
}

inline int readnum( std::vector<Header> &vec ){
    int n = 0;
    for( size_t i = 0; i < vec.size(); i ++ ){
        if( !vec[i].is_dense ){
            apex_utils::assert_true( fscanf( vec[i].fi, "%d", &vec[i].tmp_num ) == 1, "load num" );
            n += vec[i].tmp_num;
        }else{
            n ++;
        }
    }
    return n;        
}

class FeatureBlockLoader: public IDataIterator<SVDPlusBlock>{
private:	
    // number of remaining lines to be loaded
    int nline_remain;
private:
    std::vector<unsigned> index_ufeedback;
    std::vector<float>    value_ufeedback; 
private:        
    std::vector<float> row_label;
    std::vector<int>   row_ptr;        
    std::vector<unsigned>   feat_index;
    std::vector<float>      feat_value;        
private:
    struct Elem{
        unsigned index;
        float    value;
        inline bool operator<( const Elem &b ) const{
            return index < b.index;
        }
    };

    inline void load( std::vector<Elem> &data, std::vector<Header> &vec ){
        data.clear();
        for( size_t i = 0; i < vec.size(); i ++ ){
            if( !vec[i].is_dense ) { 
                for( int j = 0; j < vec[i].tmp_num; j ++ ){
                    Elem e;
                    apex_utils::assert_true( fscanf ( vec[i].fi, "%u:%f", &e.index, &e.value ) == 2, "load feat" );  
                    vec[i].check_base( e.index );
                    e.index += vec[i].base;
                    data.push_back( e );
                }
            }else{
                Elem e;
                apex_utils::assert_true( fscanf ( vec[i].fi, "%f", &e.value ) == 1, "load feat" );  
                e.index = vec[i].base;
                data.push_back( e );
            }
        }
        sort( data.begin(), data.end() );
    }

    inline void add( const std::vector<Elem> &vec ){
        for( size_t i = 0; i < vec.size(); i ++ ){
            feat_index.push_back( vec[i].index );
            feat_value.push_back( vec[i].value );
        } 
    }
public:
    FILE *fp, *finfo, *frate, *fwlist;
    std::vector< Header > fg, fu, fi;
    // number of extra imfb
    int num_extra_imfb;
    std::vector< Header > fimfb;
private:
	int top_wlist;
	std::vector< bool > vec_wlist;
private:
    inline void load_imfb( std::vector<Header> &vec ){
        for( size_t i = 0; i < vec.size(); i ++ ){
            apex_utils::assert_true( fscanf( vec[i].fi, "%d", &vec[i].tmp_num ) == 1, "load num" );            
            for( int j = 0; j < vec[i].tmp_num; j ++ ){
                unsigned idx; float val;
                apex_utils::assert_true( fscanf ( vec[i].fi, "%u:%f", &idx, &val ) == 2, "load feat" );
                index_ufeedback.push_back( idx + vec[i].base ); 
                value_ufeedback.push_back( val );
            }
        }
    }
    inline bool load_imfb( int &nline_remain ){
        int num_ufeedback;
        index_ufeedback.clear(); value_ufeedback.clear();
        if( fscanf( finfo, "%d%d", &nline_remain, &num_ufeedback ) != 2 ) return false;
        this->load_imfb( fimfb );
        for( int i = 0; i < num_ufeedback; i ++ ){
            unsigned idx; float val;
            apex_utils::assert_true( fscanf( finfo, "%d:%f", &idx, &val ) == 2, 
                                     "can't load implict feedback file" );
            index_ufeedback.push_back( idx + num_extra_imfb );
            value_ufeedback.push_back( val );
        }
	
		if( fwlist != NULL ){
			vec_wlist.clear();
			top_wlist = 0;
			int n = nline_remain;
			while( n -- ){
				int pass;
				apex_utils::assert_true( fscanf( fwlist, "%d%*[^\n]\n", &pass ) == 1, "invalid white list" );
				vec_wlist.push_back( pass != 0 );
				if( pass == 0 ) nline_remain --;
			}
		}
        return true;
    }
public:
	int num_block;
    int block_max_line;
    float scale_score;
    FeatureBlockLoader(){
		num_block = 0;
        nline_remain = 0;
        scale_score = 1.0f;
        frate = NULL; fwlist = NULL;
        this->block_max_line = 10000; 
    }
    virtual void init( void ){}
    virtual void set_param( const char *name, const char *val ){}
    virtual void before_first(){}
	virtual bool next( SVDPlusBlock &e ){
		while( this->next_inner( e ) ){
			if( e.data.num_row != 0 || e.extend_tag != svdpp_tag::DEFAULT ){
				num_block++; return true;
			}
		}
		return false;
	}
    // load data into e, caller isn't responsible for space free
    // Loader will keep the space until next call of this function 
    inline bool next_inner( SVDPlusBlock &e ){        
        e.extend_tag = svdpp_tag::MIDDLE_TAG;
        // load from feedback file
        if( nline_remain == 0 ){
            if( !this->load_imfb( nline_remain ) ) return false;
            e.extend_tag = svdpp_tag::START_TAG;
        }
        
        // check lines to be loaded
        int num_line = nline_remain;
        if( nline_remain >= 0 ){
            if( nline_remain > block_max_line ){
                // smart arrangement to make data in similar size in each block
                int pc = ( nline_remain + block_max_line - 1 ) / block_max_line;
                num_line = ( nline_remain + pc - 1 ) / pc;
            }else{
                e.extend_tag = e.extend_tag & svdpp_tag::END_TAG;
            }
        }else{
            switch( nline_remain ){
            case -1: e.extend_tag = svdpp_tag::START_TAG; break;
            case -2: e.extend_tag = svdpp_tag::END_TAG;   break;
            default: apex_utils::error("unknown extend tag");
            }
            num_line = 0; nline_remain = 0; 
        }
        this->nline_remain -= num_line;
        // set data 
        if( e.extend_tag == svdpp_tag::MIDDLE_TAG ){        
            e.num_ufeedback = 0;
            e.index_ufeedback = NULL;
            e.value_ufeedback = NULL;
        }else{
            e.num_ufeedback = static_cast<int>( index_ufeedback.size() );
            if( index_ufeedback.size() != 0 ){
                e.index_ufeedback = &index_ufeedback[0];
                e.value_ufeedback = &value_ufeedback[0];
            }
        }
        
        int num_elem = 0, top = 0;
        row_label.resize( static_cast<size_t>( (num_line+1) ) );
        row_ptr.resize  ( static_cast<size_t>( (num_line+1)*3 + 1 ) );
        row_ptr[ 0 ] = 0;
        feat_index.clear();
        feat_value.clear();

		while( top < num_line || (nline_remain==0 && fwlist != NULL && top_wlist < (int)vec_wlist.size() ) ){ 
            apex_utils::assert_true( fscanf( fp, "%*d%*d%f%*[^\n]\n", &row_label[top] ) == 1, "invalid feature format" );  
            if( frate != NULL ){
                apex_utils::assert_true( fscanf( frate, "%f%*[^\n]\n", &row_label[top] ) == 1, "invalid response" );                  
            }
            row_label[ top ] /= scale_score;
            row_ptr[ top*3 + 1 ] = (num_elem += readnum(fg) );                
            row_ptr[ top*3 + 2 ] = (num_elem += readnum(fu) );
            row_ptr[ top*3 + 3 ] = (num_elem += readnum(fi) );
            std::vector<Elem> vg, vu, vi;
            this->load( vg, fg );
            this->load( vu, fu ); 
            this->load( vi, fi ); 

			if( fwlist != NULL ){
				apex_utils::assert_true( top_wlist < (int)vec_wlist.size(), "bug occurs" );
				if( !vec_wlist[ top_wlist ++ ] ){
					num_elem = row_ptr[ top*3 ]; continue;
				}else{
					apex_utils::assert_true( top < num_line, "BUG occurs" );
				}
			}
            this->add( vg );
            this->add( vu );
            this->add( vi );
            top ++;
        }          

        if( fwlist != NULL ){
            num_line = top; 
        }else{
            apex_utils::assert_true( num_line == top, "bug occurs" );
        }

        e.data.num_row   = num_line;
        e.data.num_val   = num_elem;
        e.data.row_ptr   = &row_ptr[0];
        if( num_line != 0 ){
            e.data.row_label = &row_label[0];
            e.data.feat_index= &feat_index[0];
            e.data.feat_value= &feat_value[0];
        }
        return true;
    }
};

const char *folder = "features";
const char *fdsuffix = "imfb";
int main( int argc, char *argv[] ){
    if( argc < 3 ){
        printf("Combine different text feature files into a binary used grouped format buffer\n"\
               "Usage:<inname> <outname> [options] -g [gf1] [gf2]... -u [uf1]... -i [itemf1] .. -efd [extra ufeedback1] ...\n"\
               "options: -max_block max_block, -scale_score scale_score -fd feedback_suffix, -rt rating_file, -wlist whitelist\n");
        return 0; 
    }
    FeatureBlockLoader loader;

    time_t start = time( NULL );
    int mode = 0, ibase = 0, ubase = 0, gbase = 0, imfbbase = 0;
    for( int i = 3; i < argc; i ++ ){        
        if( !strcmp( argv[i], "-g") ){
            mode = 0; continue;
        }
        if( !strcmp( argv[i], "-u") ){
            mode = 1; continue;
        }
        if( !strcmp( argv[i], "-i") ){
            mode = 2; continue;
        }
        if( !strcmp( argv[i], "-efd") ){
            mode = 3; continue;
        }
        if( !strcmp( argv[i], "-gd") ){
            mode = 4; continue;
        }
        if( !strcmp( argv[i], "-max_block") ){
            loader.block_max_line = atoi( argv[++i] ); continue;
        }
        if( !strcmp( argv[i], "-scale_score") ){
            loader.scale_score = (float)atof( argv[++i] ); continue;
        }
        if( !strcmp( argv[i], "-fd") ){
            fdsuffix = argv[ ++i ]; continue;
        }
        if( !strcmp( argv[i], "-rt") ){
            loader.frate = apex_utils::fopen_check( argv[ ++i ], "r" ); continue;
        }        
        if( !strcmp( argv[i], "-wlist") ){
            loader.fwlist = apex_utils::fopen_check( argv[ ++i ], "r" ); continue;
        }
        if( !strcmp( argv[i], "-skip") ){
            int skip = atoi( argv[++i] );
            switch( mode ){
            case 0: ( loader.fg.size() == 0 ? gbase : loader.fg.back().num_feat ) += skip; break;
            case 1: ( loader.fu.size() == 0 ? ubase : loader.fu.back().num_feat ) += skip; break;
            case 2: ( loader.fi.size() == 0 ? ibase : loader.fi.back().num_feat ) += skip; break;
            case 3: ( loader.fimfb.size() == 0 ? imfbbase : loader.fimfb.back().num_feat ) += skip; break;
            }           
            continue;            
        }
        
        char name[ 256 ];
        sprintf( name, "%s/%s.%s", folder, argv[1], argv[i] );
        Header h;
        h.fi = apex_utils::fopen_check( name, "r" );
        if( mode == 4 ){
            h.is_dense = true; h.num_feat = 1;
            loader.fg.push_back( h );
        }else{
            apex_utils::assert_true( fscanf( h.fi, "%d", &h.num_feat ) == 1, "num feat" );
            switch( mode ){
            case 0: loader.fg.push_back( h ); break;
            case 1: loader.fu.push_back( h ); break;
            case 2: loader.fi.push_back( h ); break;            
            case 3: loader.fimfb.push_back( h ); break;
            default: ;
            }             
        }   
    }
    {
        char name[ 256 ];
        sprintf( name, "%s.%s", argv[1], fdsuffix );
        loader.finfo = apex_utils::fopen_check( name   , "r" );
        loader.fp    = apex_utils::fopen_check( argv[1], "r" );
    }
    printf("num_global=%d, num_user=%d, num_item=%d, num_extra_imfb=%d\n", 
           norm( loader.fg, gbase ), norm( loader.fu, ubase ), 
           norm( loader.fi, ibase ), loader.num_extra_imfb = norm( loader.fimfb, imfbbase ) );
    
    printf("start creating buffer...\n");
    create_binary_buffer( argv[2], &loader );
    fclose( loader.fp );
    fclose( loader.finfo );
    if( loader.frate  != NULL ) fclose( loader.frate );
    if( loader.fwlist != NULL ) fclose( loader.fwlist );
    vclose( loader.fg );
    vclose( loader.fu );
    vclose( loader.fi );
    vclose( loader.fimfb );
    printf("all generation end,%d blocks, %lu sec used\n", loader.num_block, (unsigned long)(time(NULL) - start) );
    return 0;
}
