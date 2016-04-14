/**************************************************************************************************
 * Authors: 
 *   lang li,
 *
 * Routines:
 *   map the vertex.
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
#include <time.h>

#include "convert.h"
using namespace convert;
#define LINE_FORMAT		"%d\t%d\n"
#define LINE_FORMAT_VERTEX		"%d %d %d\n"

unsigned long long line_num=0;
FILE * in_vertex;
unsigned int *vertex_map;
/*
 * Regarding the vertex indexing:
 * We will assume the FIRST VERTEX ID SHOULD be 0!
 * Although in real cases, this will not necessarily be true,
 * (in many real graphs, the minimal vertex id may not be 0)
 * the assumption we made will ease the organization of the vertex
 * indexing! 
 * Since with this assumption, the out-edge offset of vertices
 * with vertex_ID can be easily accessed by using the suffix:
 * index_map[vertex_ID]
 */

void edgelist_map( const char* input_file_name,
        const char* edge_file_name, 
        const char* vertex_file_name,
        const char* out_dir,
        const char* origin_edge_file)
{
    printf( "Start Processing %s.\nWill generate %s in destination folder.\n", 
            input_file_name, edge_file_name );

    srand((unsigned int)time(NULL));

    in = fopen( input_file_name, "r" );
    if( in == NULL ){
        printf( "Cannot open the input graph file!\n" );
        exit(1);
    }

    in_vertex = fopen( vertex_file_name, "r" );
    if( in_vertex == NULL ){
        printf( "Cannot open the vertex file!\n" );
        exit(1);
    }
    vertex_map = (unsigned int *)map_anon_memory( sizeof(unsigned int) * (max_vertex_id+1), true, true  );
    init_vertex_map( vertex_map );
    //for(int i=0;i<max_vertex_id+1;i++)
    //    printf("vertex_map[%d]:%d\n",i,*(vertex_map+i));

    edge_file = fopen( edge_file_name, "w+" );
    if( NULL == edge_file ){
        printf( "Cannot create edge list file:%s\nAborted..\n",
                edge_file_name );
        exit( -1 );
    }

    memset( (char*)type2_edge_buffer, 0, EDGE_BUFFER_LEN*sizeof(struct type2_edge) );

    //init the global variable
    line_num = 0;
    num_edges = 0;

    //init the file pointer to  the head of the file
    fseek( in , 0 , SEEK_SET );	

    printf( "Generating _b20-edges.txt\n" );       
    while ( read_one_edge() != CUSTOM_EOF ){
        //jump the ##
        if (num_edges == 0)
            continue;

        (*(buf1 + current_buf_size)).src_vert = vertex_map[src_vert];
        (*(buf1 + current_buf_size)).dest_vert = vertex_map[dst_vert];
        //printf("src:%d; dest:%d\n", vertex_map[src_vert],vertex_map[dst_vert]);
        current_buf_size++;
        if (current_buf_size == each_buf_size)
        {
            //call function to sort and write back
            std::cout << "copy " << current_buf_size << " edges to radix sort process." << std::endl;
            wake_up_sort(file_id, current_buf_size, false);
            current_buf_size = 0;
            file_id++;
        }
    }//while EOF
    if (current_buf_size)
    {
        std::cout << "copy " << current_buf_size << " edges to radix sort process." << std::endl;
        wake_up_sort(file_id, current_buf_size, true);
        current_buf_size = 0;
    }

    //finished processing
    fclose( in );
    //fclose(out_txt);
    fclose( edge_file );
}

void init_vertex_map( unsigned int * vertex_map )
{
    unsigned int temp1;
    unsigned int temp2;
    char line_buffer[MAX_LINE_LEN];
    while(true)
    {
        char* res;

        if(( res = fgets( line_buffer, MAX_LINE_LEN, in_vertex )) == NULL )
            return;
        sscanf( line_buffer, LINE_FORMAT_VERTEX, &temp1, vertex_map+line_num, &temp2);
        ++line_num;
    }
}
