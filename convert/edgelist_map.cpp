/**************************************************************************************************
 * Authors: 
 *   lang li,
 *
 * Routines:
 *   map the vertex.
 *   
 *************************************************************************************************/

#include <iostream>
#include <sstream>
#include <algorithm>
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
using namespace std;

#define LINE_FORMAT		"%d\t%d\n"
#define LINE_FORMAT_VERTEX		"%u\t%u\t%u\n"
#define LINE_FORMAT_PARTITION "%u\n"

static FILE * in_vertex;
static struct vertex_map *vtx_map;

void edgelist_map( const char* input_graph_name,
        const char* edge_file_name, 
        const char* vertex_file_name,
        const char* out_dir,
        const char* input_file_name)
{
    cout << "Start Processing " << input_graph_name << "\nWill generate the grid files in destination folder.\n";

    in = fopen( input_graph_name, "r" );
    if( in == NULL ){
        cout << "Cannot open the input graph file!\n";
        exit(1);
    }

    in_vertex = fopen( vertex_file_name, "r" );
    if( in_vertex == NULL ){
        cout << "Cannot open the vertex file!\n";
        exit(1);
    }

    //mmap the partition file, add old_id according the line_num
    vtx_map = (struct vertex_map *)map_anon_memory( sizeof(struct vertex_map) * (max_vertex_id+1), true, true  );
    init_vertex_map( vtx_map );

    //sort the vtx_map by the first column(partition_id)
    std::sort(vtx_map, vtx_map + max_vertex_id + 1, comp_partition_id);
    int partition = vtx_map[max_vertex_id].partition_id + 1;

    //assign new_id by the order
    for(unsigned int i=0;i<=max_vertex_id;++i)
        vtx_map[i].new_id = min_vertex_id + i;

    //sort the vtx_map by the second column(old_id)
    std::sort(vtx_map, vtx_map + max_vertex_id + 1, comp_old_id);
    
    //generate the grid files
    std::ostringstream file_name;
    //do not need dual buffer, so mutiply 2
    int partition_size = each_buf_size * 2 / partition / partition;
    int ** size = new int*[partition];
    struct tmp_in_edge *** grid_buf = new struct tmp_in_edge **[partition];
    FILE *** grid_file = new FILE**[partition];
    for(int i=0;i<partition;++i){
        size[i] = new int[partition];
        grid_buf[i] = new struct tmp_in_edge *[partition];
        grid_file[i] = new FILE*[partition];
        for(int j=0;j<partition;++j){
            size[i][j] = 0;
            grid_buf[i][j] = buf1 + (i * partition + j) * partition_size;
            file_name.str("");
            file_name << out_dir << '/' << input_file_name << '-' << i << j;
            grid_file[i][j] = fopen(file_name.str().c_str(), "w+");
        }
    }


    file_name.str("");
    file_name << out_dir << '/' << input_file_name << "-vertex-map";
    FILE *vertex_map_file = fopen( file_name.str().c_str(), "w+" );
    if( NULL == vertex_map_file){
        cout << "Cannot create vertex_map_file:" << file_name.str().c_str() << "\nAborted..\n";
        exit( -1 );
    }

    //init
    num_edges = 0;
    fseek( in , 0 , SEEK_SET );	

    cout << "generating grid...\n";
    int row, col;
    while ( read_one_edge() != CUSTOM_EOF ){
        //jump the ##
        if (num_edges == 0)
            continue;

        row = vtx_map[src_vert-min_vertex_id].partition_id;
        col = vtx_map[dst_vert-min_vertex_id].partition_id;
        (grid_buf[row][col] + size[row][col])->src_vert = vtx_map[src_vert-min_vertex_id].new_id;
        (grid_buf[row][col] + size[row][col])->dest_vert = vtx_map[dst_vert-min_vertex_id].new_id;
        ++size[row][col];
        if (size[row][col] == partition_size)
        {
            //write back
            for(int i=0;i<partition_size;++i){
                fprintf(grid_file[row][col], "%u\t%u\n", (grid_buf[row][col] + i)->src_vert, (grid_buf[row][col] + i)->dest_vert);
                fflush(grid_file[row][col]);
            }
            size[row][col] = 0;
        }
    }//while EOF
    for(row=0;row<partition;++row){
        for(col=0;col<partition;++col){
            cout << "line[" << row << "][" << col << "]:" << size[row][col] << endl;
            for(int k=0;k<size[row][col];++k){
                fprintf(grid_file[row][col], "%u\t%u\n", (grid_buf[row][col] + k)->src_vert, (grid_buf[row][col] + k)->dest_vert);
                fflush(grid_file[row][col]);
            }
        }
    }

    //write back the vertex map file
    for(unsigned int i=0;i<=max_vertex_id;++i)
        fprintf(vertex_map_file, LINE_FORMAT_VERTEX, vtx_map[i].partition_id, vtx_map[i].old_id, vtx_map[i].new_id);
    fclose(vertex_map_file);

    //finished processing
    fclose( in );
    //fclose(out_txt);
    //fclose( edge_file );

    //close grid files
    for(int i=0;i<partition;++i){
        for(int j=0;j<partition;++j)
            fclose(grid_file[i][j]);
    }
}

bool comp_partition_id(const struct vertex_map &v1, const struct vertex_map &v2){
    return v1.partition_id < v2.partition_id || (v1.partition_id == v2.partition_id && v1.old_id < v2.old_id);
}

bool comp_old_id(const struct vertex_map &v1, const struct vertex_map &v2){
    return v1.old_id < v2.old_id;
}

void init_vertex_map( struct vertex_map * vtx_map )
{
    unsigned long long line_num=0;
    char line_buffer[MAX_LINE_LEN];
    while(true)
    {
        char* res;

        if(( res = fgets( line_buffer, MAX_LINE_LEN, in_vertex )) == NULL )
            return;
        sscanf( line_buffer, LINE_FORMAT_PARTITION, &vtx_map[line_num].partition_id);
        vtx_map[line_num].old_id = line_num;
        ++line_num;
    }
}
