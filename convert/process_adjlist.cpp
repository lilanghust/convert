/**************************************************************************************************
 * Authors: 
 *   Jian He, 
 *
 * Routines:
 *   Process graph whose format is adjacency list.
 *   
 *************************************************************************************************/

#include <iostream>
#include <cassert>
#include <limits.h>

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "convert.h"
using namespace convert;

float produce_random_weight()
{
    //printf("%f\n", ((float)(10.0 * rand()/(RAND_MAX + 1.0))));
    return ((float)(10.0 * rand()/(RAND_MAX + 1.0)));
    //return ((float)(rand()/(RAND_MAX + 1.0)));
}

void process_adjlist(const char * input_file_name, 
        const char * edge_file_name, 
        const char * vert_index_file_name)
{
    char * res;
    unsigned int edge_buffer_offset = 0;
    unsigned int edge_suffix = 0;
    unsigned int recent_src_vert = UINT_MAX;

    srand((unsigned int)time(NULL));

    printf( "Start Processing %s.\nWill generate %s and %s in destination folder.\n", 
            input_file_name, edge_file_name, vert_index_file_name );

    in = fopen( input_file_name, "r" );
    if( in == NULL ){
        printf( "Cannot open the input graph file!\n" );
        exit(1);
    }

    //out_txt = fopen(out_txt_file_name, "wt+");
    //if (out_txt == NULL)
    //{
    //    printf("Connnot open out_txt_file!\n");
    //    exit(1);
    //}

    edge_file = fopen(edge_file_name, "w+");
    if (NULL == edge_file){
        printf("Cannot create edge list file:%s\nAborted..\n", edge_file_name);
        exit(-1);
    }

    memset( (char*)type2_edge_buffer, 0, EDGE_BUFFER_LEN*sizeof(struct type2_edge) );

    while ((res = (char *) get_adjline()) != '\0')
    {
        unsigned int index = 0;
        unsigned int tmp_int = 0;
        unsigned int break_signal = 0;
        unsigned int num_out_edges = 0;
        unsigned int origin_num_edges = 0;
        while (res[0] != '#' && res[index] != '\0')
        {
            sscanf((res + index), "%d[^ ]", &tmp_int);

            if (index == 0){//means this is the src_vert
                src_vert = tmp_int;
            }
            else if (num_out_edges == 0 && index != 0 && origin_num_edges == 0)
            {
                assert(index >= 2);
                origin_num_edges = tmp_int;
                if (origin_num_edges != 0)
                {
                    if (src_vert < recent_src_vert){
                        if (num_edges > 1)
                        {
                            printf("Edge order is not correct at line:%lld.Edge processing terminated.\n", num_edges);
                            fclose(in);
                            exit(-1);
                        }
                    }
                    if (src_vert < min_vertex_id) min_vertex_id = src_vert;
                    if (src_vert > max_vertex_id) max_vertex_id = src_vert;


                }
            }
            else
            {
                assert(index >= 4);
                dst_vert = tmp_int;
                num_edges++;
                edge_suffix = num_edges - (edge_buffer_offset * EDGE_BUFFER_LEN);
                type2_edge_buffer[edge_suffix].dest_vert = dst_vert;

                (*(buf1 + current_buf_size)).src_vert = src_vert;
                (*(buf1 + current_buf_size)).dest_vert = dst_vert;
                current_buf_size++;
                if (current_buf_size == each_buf_size)
                {
                    //call function to sort and write back
                    std::cout << "copy " << current_buf_size << " edges to radix sort process." << std::endl;
                    wake_up_sort(file_id, current_buf_size, false);
                    current_buf_size = 0;
                    file_id++;
                }

                //fprintf(out_txt, "%d\t%d\t%f\n", src_vert, dst_vert, edge_buffer[edge_suffix].edge_weight);

                num_out_edges ++;
                if (edge_suffix == (EDGE_BUFFER_LEN - 1)){

                    edge_buffer_offset += 1;
                }

                if (dst_vert < min_vertex_id)  min_vertex_id = dst_vert;
                if (dst_vert > max_vertex_id)  max_vertex_id = dst_vert;
            }

            while (res[index] != ' ')
            {
                if (res[index] == '\n')
                {
                    free(res);
                    break_signal = 1; 
                    break;
                }
                index++;
            }
            //printf("%d\n", index);
            index++;
            if (break_signal == 1)
                break;
        }//while till EOL
        if (num_out_edges != origin_num_edges)
        {
            printf("num_out_edges = %d, origin_num_edges = %d\n", num_out_edges, origin_num_edges);
        }
        assert(num_out_edges == origin_num_edges);

        recent_src_vert = src_vert;
        if( num_out_edges > max_out_edges ) max_out_edges = num_out_edges;

    }//while till EOF

    if (current_buf_size)
    {
        std::cout << "copy " << current_buf_size << " edges to radix sort process." << std::endl;
        wake_up_sort(file_id, current_buf_size, true);
        current_buf_size = 0;
    }

    //flush the remaining data in vertex index and edge buffer to file

    if (res != NULL)
        free(res);
    fclose( in );
    //fclose(out_txt);
    fclose(edge_file);
}

char *get_adjline()
{  
    char *zLine;  
    int nLine;  
    int n;  
    int eol;  

    nLine = MAX_LINE_LEN;
    zLine = (char *)malloc( nLine );  
    if( zLine==0 ) return 0;  
    n = 0;  
    eol = 0;  
    while( !eol ){  
        if( (n+10) > nLine ){  
            nLine = nLine*2 + MAX_LINE_LEN/2;  
            zLine = (char *)realloc(zLine, nLine);  
            if( zLine == NULL ){
                printf("get_adjline: memory allocation error!\n");
                return NULL;
            }
        }  
        if( fgets(&zLine[n], nLine - n, in)==0 ){  
            if( n == 0 ){ //empty line?
                free(zLine);  
                return NULL;
            }  
            zLine[n] = 0;  
            eol = 1;  
            break;  
        }  
        //printf("%s\n",zLine);
        //printf("first_n = %d\n", n);
        while( zLine[n] ){ n++; }  
        //printf("second_n = %d\n", n);
        if( n>0 && zLine[n-1]=='\n' ){  
            //n--;  
            //zLine[n] = 0;  
            eol = 1;  
            //printf("%s", zLine);
            //zLine = (char *)realloc( zLine, n+1 );  
            return zLine;
        }  
    }  
    zLine = (char *)realloc( zLine, n+1 );  
    printf("%s\n", zLine);
    return zLine;  
} 

