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
unsigned int min_vertex_id=100000000, max_vertex_id=0;
unsigned long long num_edges=0;
unsigned long max_out_edges = 0;
unsigned long long mem_size;

int main( int argc, const char**argv)
{
	unsigned int pos;
	//input files
	std::string input_graph_name, input_file_name, temp;
	//output files
	std::string in_dir, out_dir, out_edge_file_name,
		vertex_file_name;
    std::string snap_type;	
	char* buffer;

	//setup options
	setup_options_convert( argc, argv );

	input_graph_name = temp = vm["graph"].as<std::string>();
	pos = temp.find_last_of("/");
    in_dir = temp.substr(0, pos+1);
	input_file_name = temp.substr(pos+1);

	out_dir = vm["destination"].as<std::string>();
	vertex_file_name = in_dir + input_file_name + "-adjlist.part";

	mem_size = (unsigned long long)(vm["memory"].as<int>())*1024l*1024l;
    buffer = process_in_edge(mem_size, input_file_name.c_str(), out_dir.c_str());

	std::cout << "Input file: " << input_file_name << "\n";

	snap_type = vm["type"].as<std::string>();
	if( snap_type == "edgelist" )
    {
	    out_edge_file_name = out_dir+ input_file_name +"-adjlist";
        process_edgelist( input_graph_name.c_str(), 
				out_edge_file_name.c_str(), 
				out_dir.c_str(), 
                input_file_name.c_str());
        write_desc(out_dir + input_file_name + ".desc");
    }
	else if (snap_type == "adjlist" )
    {
        out_edge_file_name = out_dir+ input_file_name +"-adjlist";
		process_adjlist( input_graph_name.c_str(), 
				out_edge_file_name.c_str(), 
				vertex_file_name.c_str());
        write_desc(out_dir + input_file_name + ".desc");
    }
	else if (snap_type == "edgelist_map" )
    {
        read_desc(in_dir + input_file_name + ".desc");
        out_edge_file_name = out_dir+ input_file_name +"-edges-map.txt";
        //std::cout << "edgelist_map: " << vertex_file_name << std::endl;
		edgelist_map( input_graph_name.c_str(), 
				out_edge_file_name.c_str(), 
				vertex_file_name.c_str(),
				out_dir.c_str(), 
                input_file_name.c_str());
    }
	else if (snap_type == "remap" )
    {
        read_desc(in_dir + input_file_name + ".desc");
        //std::cout << "edgelist_map: " << vertex_file_name << std::endl;
		remap( input_graph_name.c_str(), 
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

void write_desc(std::string path)
{
	//graph description
    std::ofstream desc_file( path.c_str() );
	desc_file << "[description]\n";
	desc_file << "min_vertex_id = " << min_vertex_id << "\n";
	desc_file << "max_vertex_id = " << max_vertex_id << "\n";
	desc_file << "num_of_edges = " << num_edges << "\n";
	desc_file << "max_out_edges = " << max_out_edges << "\n";
	desc_file.close();
}

void read_desc(std::string path)
{
    char line_buffer[1000];
    std::ifstream desc_file( path.c_str() );
    desc_file.getline( line_buffer, 1000 );
    desc_file.getline( line_buffer, 1000 );
    sscanf(line_buffer, "min_vertex_id = %u", &min_vertex_id);
    desc_file.getline( line_buffer, 1000 );
    sscanf(line_buffer, "max_vertex_id = %u", &max_vertex_id);
    max_vertex_id -= min_vertex_id;
	desc_file.close();
}
