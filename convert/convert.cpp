/**************************************************************************************************
 * Authors: 
 *   Zhiyuan Shao, Jian He
 *
 * Routines:
 *   Graph format conversion.
 *************************************************************************************************/

/*
 * The purpose of this small utility program is to convert existing snap (edge list or adj list) file to binary format.
 * This program will generate three output files:
 * 1) Description file (.desc suffix), which describes the graph in general (e.g., how many edges, vertices)
 * 2) Index file (binary, .index suffix), which gives the (output) edge offset for specific vertex with VID.
 *	The offset value (unsigned long long), can be obtained for a vertex with VID: index_map[VID],
 *	where Index_Map is the address that the Index file mmapped into memory.
 * 3) Edge file (binary, .edge suffix), which stores all the outgoing edges according to the sequence of 
 *	index of vertices (i.e., the source vertex ID of the edges). Entries are tuples of the form:
 --------------------------------------------------------------------------------
 COMPACT  | <4 byte dst, 4bytes weight>
 --------------------------------------------------------------------------------
 * Note:
 * 1) The first element of Edge file (array) is INTENTIONALLY left UNUSED! 
 *	This prevents the situation that the offset of some vertex is zero,
 *	but the vertex DO have outgoing edges.
 *	By doing this, the "offset" field of the vertices that have outging edges CAN NOT be ZERO!
 * 2) A "correct" way of finding the outgoing edges of some specific vertex VID is the follows:
 * 	a) find the range where the outgoing edges of vertex VID is stored in the edge list file, say [x, y];
 *	b) read the edges from the Edge file, from suffix x to suffix y
 * 3) The edge file does not store the source vertex ID, since it can be obtained from the beginning
 *	of indexing;
 * 4) The system can only process a graph, whose vertex ID is in range of [0, 4G] and 
 *	at the same time, the "weight value" of each edge is in "float" type.
 *	It is possible that the "weight value" can be "double", and the vertex ID beyonds the range of [0, 4G].
 *	In those cases, the format of edge file has to be changed, and the index file can remain the same.
 * TODO:
 * This program has not deal with the disorder of edges (in edge list format) or lines (in adjlist format) yet.
 */

#include "options_utils_convert.h"
#include <cassert>
#include <fstream>
#include <sys/mman.h>
#include "convert.h"
using namespace convert;
//boost::program_options::options_description desc;
//boost::program_options::variables_map vm;

//statistic data below
unsigned int min_vertex_id=100000, max_vertex_id=0;
unsigned long long num_edges=0;
unsigned long max_out_edges = 0;
unsigned long long mem_size;
std::fstream desc_file;
std::string out_desc_file_name; 

int main( int argc, const char**argv)
{
	unsigned int pos;
	//input files
	std::string input_graph_name, input_file_name, temp;
	//output files
	std::string out_dir, out_edge_file_name,
		vertex_file_name;
    std::string snap_type;	
	char* buffer;

	//setup options
	setup_options_convert( argc, argv );

	input_graph_name = temp = vm["graph"].as<std::string>();
	pos = temp.find_last_of( "/" );
	temp = temp.substr( pos+1 );
	pos = temp.find_last_of( "." );
	input_file_name = temp.substr(0,pos);

	out_dir = vm["destination"].as<std::string>();
	vertex_file_name = input_file_name +"_b20s-vertices.txt";
    out_desc_file_name = out_dir+ input_file_name +".desc";

	mem_size = (unsigned long long)512*1024*1024;
	std::cout << "Pre-allocation memory size is " << mem_size/(1024*1024) << "(MB)" << std::endl;
    buffer = process_in_edge(mem_size, input_file_name.c_str(), out_dir.c_str());

	std::cout << "Input file: " << input_file_name << "\n";

	snap_type = vm["type"].as<std::string>();
	if( snap_type == "edgelist" )
    {
	    out_edge_file_name = out_dir+ input_file_name +"_adjlist.txt";
        process_edgelist( input_graph_name.c_str(), 
				out_edge_file_name.c_str(), 
				out_dir.c_str(), 
                input_file_name.c_str());
        write_desc();
    }
	else if (snap_type == "adjlist" )
    {
        out_edge_file_name = out_dir+ input_file_name +"_adjlist.txt";
		process_adjlist( input_graph_name.c_str(), 
				out_edge_file_name.c_str(), 
				vertex_file_name.c_str());
        write_desc();
    }
	else if (snap_type == "edgelist_map" )
    {
        init_max_vertex_id();
        out_edge_file_name = out_dir+ input_file_name +"_b20s-edges.txt";
        //std::cout << "edgelist_map: " << vertex_file_name << std::endl;
		edgelist_map( input_graph_name.c_str(), 
				out_edge_file_name.c_str(), 
				vertex_file_name.c_str(),
				out_dir.c_str(), 
                input_file_name.c_str());
    }
	else{
		std::cout << "input parameter (type) error!\n";
		exit( -1 );
	}

	munlock( buffer, mem_size);
	munmap( buffer, mem_size);
}

void write_desc()
{
	//graph description
	desc_file.open( out_desc_file_name.c_str() );
	desc_file << "[description]\n";
	desc_file << "min_vertex_id = " << min_vertex_id << "\n";
	desc_file << "max_vertex_id = " << max_vertex_id << "\n";
	desc_file << "num_of_edges = " << num_edges << "\n";
	desc_file << "max_out_edges = " << max_out_edges << "\n";
	desc_file.close();
}

void init_max_vertex_id()
{
    char line_buffer[1000];
	desc_file.open( out_desc_file_name.c_str() );
    desc_file.getline( line_buffer, 1000 );
    desc_file.getline( line_buffer, 1000 );
    desc_file.getline( line_buffer, 1000 );
    sscanf(line_buffer, "max_vertex_id = %d", &max_vertex_id);
	desc_file.close();
}
