/**************************************************************************************************
 * Authors: 
 *   Jian He, Huiming Lv
 *
 * Routines:
 *   This method is borrowed from GraphChi.
 *   
 *************************************************************************************************/

#include <iostream>
#include <sstream>
#include <cassert>

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <limits.h>

#include "convert.h"
using namespace convert; 
#include <vector>

typedef unsigned int u32_t;
typedef unsigned long long u64_t;

u32_t vert_buffer_offset = 0;
u32_t edge_buffer_offset = 0;
u32_t edge_suffix = 0;
u32_t vert_suffix = 0;
u32_t recent_src_vert = UINT_MAX;

template <typename T>
class minheap
{
    T * nodes;
    int max_size;
    int curr_size;
    public:
    minheap(int max_size)
    {
        this->max_size = max_size;
        this->nodes = (T*)calloc(this->max_size, sizeof(T));
        this->curr_size = 0;
    }
    ~minheap()
    {
        delete nodes;
    }

    int parent(int i)
    {
        return (i+1)/2-1;
    }
    int left(int i)
    {
        return 2*i+1;
    }
    int right(int i)
    {
        return 2*i+2;
    }

    void inc_size()
    {
        curr_size++;
        assert(curr_size<=max_size);
    }
    void dec_size()
    {
        curr_size--;
        assert(curr_size>=0);
    }
    bool isempty()
    {
        return curr_size==0;
    }

    void insert(T element)
    {
        inc_size();
        int pos = curr_size-1;
        for(; pos>0&&element<nodes[parent(pos)]; pos=parent(pos))
        {
            nodes[pos] = nodes[parent(pos)];
        }

        nodes[pos] = element;

    }
    T get_min()
    {
        return nodes[0];
    }
    void pop_min()
    {
        nodes[0] = nodes[curr_size-1];
        dec_size();
        modify(0);
    }
    void modify(int i)
    {
        int l = left(i);
        int r = right(i);
        int small_pos = i;
        if(l<curr_size && nodes[l]<nodes[i])
        {
            small_pos = l;
        }
        if(r<curr_size && nodes[r]<nodes[small_pos])
        {
            small_pos = r;
        }
        if(small_pos != i)
        {
            T temp = nodes[i];
            nodes[i] = nodes[small_pos];
            nodes[small_pos] = temp;
            modify(small_pos);
        }
    }
};


struct merge_source
{
    unsigned long long buf_edges;
    std::string file_name;
    unsigned long long idx;
    unsigned long long buf_idx;
    tmp_in_edge * buffer;
    int f;
    unsigned long long file_edges;

    merge_source(tmp_in_edge * buf, unsigned long long buffer_edges, std::string filename, unsigned long long file_size) 
    {
        buffer = buf;
        //printf("init add of buffer is %llx\n", (u64_t)buffer);
        buf_edges = buffer_edges;
        file_name = filename;
        idx = 0;
        buf_idx = 0;
        //std::cout<<"file_name = "<<file_name<<std::endl;
        //std::cout<<"file_size = "<<file_size<<std::endl;
        file_edges = (unsigned long long)(file_size / sizeof(tmp_in_edge)); 
        //f = open(file_name.c_str(), O_RDONLY);
        //if (f < 0)
        //{
        //    assert(false);
        //}
        load_next();
    }

    ~merge_source()
    {
        //if (buffer != NULL) free(buffer);
        //buffer = NULL;
    }

    void finish()
    {
        //close(f);
        //free(buffer);
        // buffer = NULL;
    }

    void read_data(/*int f,*/ tmp_in_edge* tbuf, u64_t nbytes, u64_t off )
    {
        char * buf = (char*)tbuf;
        int read_finished = 0, remain = nbytes, res;
        //printf("buf address : %llx\n", (u64_t)buf);
        //printf("end buf address : %llx\n", (u64_t)((char*)buf1+mem_size));
        int fd = open(file_name.c_str(), O_RDWR, S_IRUSR | S_IRGRP | S_IROTH);
        if (fd < 0)
        {
            assert(false);
        }

        if (lseek(fd, off, SEEK_SET) < 0)
        {
            printf( "Cannot seek the attribute file!\n");
            exit(-1);
        }

        while(read_finished < (int)nbytes)
        {
            //hejian debug
            //std::cout << "nbytes = " << nbytes << std::endl;
            //std::cout << "read_finished = " << read_finished<< std::endl;
            //std::cout << "off = " << off << std::endl;

            //std::cout << "remain = " << remain << std::endl;

            if( (res = read(fd, buf, remain)) < 0 )
            {
                //std::cout << "res = " << res << std::endl;
                printf( "Cannot seek the attribute file!\n");
                exit(-1);
            }
            //std::cout<<"res = "<< res <<std::endl; 
            read_finished += res;
            remain -= res;

            //std::cout<<"nread: "<<read_finished <<" nbytes: "<<nbytes<<std::endl;
        }
        //std::cout<<"read_data ok"<<std::endl;
        assert(read_finished <= (int)nbytes);
        close(fd);
    }

    void load_next()
    {
        //std::cout<<"load_next()     buf_edges:"<<buf_edges<<std::endl;
        //std::cout<<"buf_size = "<<buf_edges*sizeof(tmp_in_edge)<<std::endl;
        //std::cout<<"un_read_size = "<<(file_edges-idx)*sizeof(tmp_in_edge)<<std::endl;
        unsigned long long len = std::min(buf_edges*sizeof(tmp_in_edge), (file_edges - idx)*sizeof(tmp_in_edge));
        read_data(/*f,*/ buffer, (u64_t)len, (u64_t)(idx * sizeof(tmp_in_edge)));
        buf_idx = 0;
    }

    bool has_more()
    {
        return idx < file_edges;
    }

    tmp_in_edge get_next()
    {
        if (buf_idx == buf_edges)
        {
            load_next();
        }
        idx++;
        if (idx == file_edges)
        {
            tmp_in_edge x = buffer[buf_idx++];
            finish();
            return x;
        }
        return buffer[buf_idx++];
    }
};

struct merge_sink
{
    u64_t buf_edges;
    FILE *file;
    u64_t buf_index;
    u64_t tmp_num_edges;
    u32_t buffer_offset;
    merge_sink(std::string file_name)
    {
        /*open the new sorted temp file*/
        file = fopen( file_name.c_str(), "w+" );
        std::cout << "src_merge_sink " << file_name << std::endl;
        if( file == NULL )
        {
            printf( "Cannot create edge list file:%s\nAborted..\n",
                    file_name.c_str() );
            exit( -1 );
        }

        tmp_num_edges = 0;
        buf_index = 0;
        buffer_offset = 0;
    }
    void Add(tmp_in_edge value)
    {
        buf_index = tmp_num_edges - (buffer_offset * each_buf_size);
        tmp_num_edges ++;
        (*(buf2 + buf_index)).src_vert = value.src_vert; 
        (*(buf2 + buf_index)).dest_vert = value.dest_vert;
        /*write back if necessary*/
        if (buf_index == (each_buf_size - 1))
        {
            tmp_in_edge *p = buf2; 
            for(unsigned int i = 0;i < each_buf_size; i++){
                fprintf( file, "%d %d %d %d %d\n", 0, (*p).src_vert, 0, (*p).dest_vert, 1);
                p++;
            }
            memset( (char*)buf2, 0, each_buf_size*sizeof(struct tmp_in_edge) );
            buffer_offset += 1;
            buf_index = 0;
        }
    }
    void finish()
    {
        tmp_in_edge *p = buf2; 
        for(unsigned int i = 0;i < buf_index+1; i++){
            fprintf( file, "%d %d %d %d %d\n", 0, (*p).src_vert, 0, (*p).dest_vert, 1);
            p++;
        }
        fclose(file);
    }
};

struct value_source
{
    int source_id;
    tmp_in_edge value;
    value_source(int id, tmp_in_edge val) : source_id(id), value(val){}
    bool operator< (value_source &obj2)
    {
        return (this->value.dest_vert < obj2.value.dest_vert);
    }
};

class kway_merge
{
    std::vector<merge_source*> sources;
    merge_sink * sink;
    minheap<value_source> m_heap;
    int merge_num;
    public:
    kway_merge(std::vector<merge_source*> sources, merge_sink* sink) : sources(sources),sink(sink),m_heap(int(sources.size()))
    {
        this->merge_num = int(sources.size());
    }

    ~kway_merge()
    {
        sink = NULL;
    }

    void merge()
    {
        int active_sources = (int)sources.size();
        for(int i=0; i<active_sources; i++)
        {
            m_heap.insert(value_source(i, sources[i]->get_next()));
        }

        while(active_sources>0 || !m_heap.isempty())
        {
            value_source v = m_heap.get_min();
            m_heap.pop_min();
            //std::cout<<"pop_min: "<<v.value.dest_vert<<std::endl;
            if(sources[v.source_id]->has_more())
            {
                m_heap.insert(value_source(v.source_id, sources[v.source_id]->get_next()));
            }
            else
            {
                active_sources--;
            }
            sink->Add(v.value);
        }
        sink->finish();
    }
};

void do_merge()
{
    //buf1 save src data, buf2 save sorted data, so divided by 2
    unsigned long long source_buf_size = ((mem_size/2/num_tmp_files)/sizeof(tmp_in_edge)) * sizeof(tmp_in_edge);
    unsigned long long buf_edges = source_buf_size/sizeof(tmp_in_edge);

    std::vector<merge_source* > sources;
    for (unsigned int i = 0; i < num_tmp_files; i++)
    {
        std::stringstream current_file_id;
        current_file_id << i;
        std::string current_file_name = std::string(prev_name_tmp_file) + current_file_id.str();
        tmp_in_edge * buf = (tmp_in_edge *)((char *)buf1 + i*source_buf_size);
        sources.push_back(new merge_source(buf, buf_edges, current_file_name, file_len[i]) );
    }
    /*init buf2*/
    memset( (char*)buf2, 0, each_buf_size*sizeof(struct tmp_in_edge));

    std::string in_edge_file_name = std::string(in_name_file);
    in_edge_file_name += "_b20s-edges.txt";
    merge_sink* sink = new merge_sink(in_edge_file_name);

    kway_merge k_merger(sources, sink);
    k_merger.merge();

    //debug end
    for (unsigned int i = 0; i < num_tmp_files; i++)
    {
        std::stringstream delete_current_file_id;
        delete_current_file_id << i;
        std::string delete_current_file_name = std::string(prev_name_tmp_file) + delete_current_file_id.str();

        std::cout << "delete tmp file "  << delete_current_file_name << std::endl;
        //lilang test
        printf("deleting temp\n");

        char tmp[1024];
        sprintf(tmp,"rm -rf %s", delete_current_file_name.c_str());
        int ret = system(tmp);
        if (ret < 0)
            assert(false);
    }
}

struct src_merge_sink
{
    u64_t buf_edges;
    FILE *src_temp_file;
    u64_t buf_index;
    u64_t tmp_num_edges;
    u64_t del_num_edges;
    u32_t buffer_offset;
    u32_t prev_last_src_vert;
    u32_t prev_last_dest_vert;
    src_merge_sink(std::string file_name)
    {
        /*open the new sorted temp file*/
        src_temp_file = fopen( file_name.c_str(), "w+" );
        std::cout << "src_merge_sink " << file_name << std::endl;
        if( src_temp_file == NULL )
        {
            printf( "Cannot create edge list file:%s\nAborted..\n",
                    file_name.c_str() );
            exit( -1 );
        }

        tmp_num_edges = 0;
        del_num_edges = 0;
        buf_index = 0;
        buffer_offset = 0;
        prev_last_src_vert = -1;
        prev_last_dest_vert = -1;
    }
    void Add(tmp_in_edge value)
    {
        buf_index = tmp_num_edges - (buffer_offset * each_buf_size);
        tmp_num_edges ++;
        (*(buf2 + buf_index)).src_vert = value.src_vert; 
        (*(buf2 + buf_index)).dest_vert = value.dest_vert;
        /*write back if necessary*/
        if (buf_index == (each_buf_size - 1))
        {
            tmp_in_edge *p = buf2; 
            for(unsigned int i = 0;i < each_buf_size; ++i, ++p){
                if( buffer_offset > 0 && i == 0 
                        && (*p).dest_vert == prev_last_dest_vert && (*p).src_vert == prev_last_src_vert )
                {
                    ++del_num_edges;
                    continue;
                }
                if( i > 0 && (*p).dest_vert == (*(p-1)).dest_vert && (*p).src_vert == (*(p-1)).src_vert )
                {
                    ++del_num_edges;
                    continue;
                }
                fprintf( src_temp_file, "%u\t%u\n", (*p).src_vert, (*p).dest_vert );
            }
            prev_last_src_vert = (*(p-1)).src_vert;
            prev_last_dest_vert = (*(p-1)).dest_vert;
            memset( (char*)buf2, 0, each_buf_size*sizeof(struct tmp_in_edge) );
            buffer_offset += 1;
            buf_index = 0;
        }
    }
    void finish()
    {
        tmp_in_edge *p = buf2; 
        for(unsigned int i = 0;i < buf_index+1; ++i, ++p){
            if( buffer_offset > 0 && i == 0 
                    && (*p).dest_vert == prev_last_dest_vert && (*p).src_vert == prev_last_src_vert )
            {
                ++del_num_edges;
                continue;
            }
            if( i > 0 && (*p).dest_vert == (*(p-1)).dest_vert && (*p).src_vert == (*(p-1)).src_vert )
            {
                ++del_num_edges;
                continue;
            }
            fprintf( src_temp_file, "%d\t%d\n", (*p).src_vert, (*p).dest_vert );
        }
        fclose(src_temp_file);
    }
};

struct src_value_source
{
    int source_id;
    tmp_in_edge value;
    src_value_source(int id, tmp_in_edge val) : source_id(id), value(val){}
    bool operator< (src_value_source &obj2)
    {
        return (this->value.src_vert < obj2.value.src_vert);
    }
};

class src_kway_merge
{
    std::vector<merge_source*> sources;
    src_merge_sink * sink;
    minheap<src_value_source> m_heap;
    int merge_num;
    public:
    src_kway_merge(std::vector<merge_source*> sources, src_merge_sink* sink) : sources(sources),sink(sink),m_heap(int(sources.size()))
    {
        this->merge_num = int(sources.size());
    }

    ~src_kway_merge()
    {
        sink = NULL;
    }

    void merge()
    {
        int active_sources = (int)sources.size();
        std::cout << "active_sources is " << active_sources << std::endl;
        for(int i=0; i<active_sources; i++)
        {
            m_heap.insert(src_value_source(i, sources[i]->get_next()));
        }

        while(active_sources>0 || !m_heap.isempty())
        {
            src_value_source v = m_heap.get_min();
            m_heap.pop_min();
            if(sources[v.source_id]->has_more())
            {
                m_heap.insert(src_value_source(v.source_id, sources[v.source_id]->get_next()));
            }
            else
            {
                active_sources--;
            }
            sink->Add(v.value);
        }
        sink->finish();
    }
};

u64_t do_src_merge(char *tmp_out_dir, char *origin_edge_file)
{
    //buf1 save src data, buf2 save sorted data, so divided by 2
    unsigned long long source_buf_size = ((mem_size/2/num_tmp_files)/sizeof(tmp_in_edge)) * sizeof(tmp_in_edge);
    unsigned long long buf_edges = source_buf_size/sizeof(tmp_in_edge);

    std::vector<merge_source* > sources;
    for (unsigned int i = 0; i < num_tmp_files; i++)
    {
        std::stringstream current_file_id;
        current_file_id << i;
        std::string current_file_name = std::string(prev_name_tmp_file) + current_file_id.str();
        tmp_in_edge * buf = (tmp_in_edge *)((char *)buf1 + i*source_buf_size);
        sources.push_back(new merge_source(buf, buf_edges, current_file_name, file_len[i]) );
    }
    /*init buf2*/
    memset( (char*)buf2, 0, each_buf_size*sizeof(struct tmp_in_edge));

    std::string tmp_file_name(tmp_out_dir) ;
    tmp_file_name += origin_edge_file;
    tmp_file_name += "-undirected-edgelist";
    src_merge_sink* sink = new src_merge_sink(tmp_file_name);

    src_kway_merge k_merger(sources, sink);
    k_merger.merge();
    //lilang test
    //std::cout << prev_name_tmp_file << std::endl;
    
    return sink->tmp_num_edges - sink->del_num_edges;
}

