This project contains one programs: "convert"

"convert": The program that converts SNAP (ordered directed edgelist) files to (ordered undirected edgelist) 
            and then converts (ordered undirected edgelist) to (ordered adjacency list ) 
            and then makes partition based on (ordered adjacency list) using gpmetis
            and then maps the vertices in the shell script in order to make the vertices in a partition is consecutive.
            and then generates mapped directed edgelist using edgelist_map.

output: 1. directed edgelist ordered by dstVId, each line format (srcPID(not used), srcVID, dstPID(not used), dstVID, cost)
        2. mapped vertices list, each line format (PID, vid, uuid(the original vid))

generating.sh : usage: ./generating graph_file_name partition_number
                one-stop service 

modify.sh     : generate the two file using the modified partitioned file
                
---------------------------------------  Prerequisites for building  ------------------------------
Linux (Ubuntu 12.04.4 LTS, 3.5.0-54-generic kernel, x86_64 for our case);
basic build tools (e.g., stdlibc, gcc, etc);
g++ (4.6.3 for our case);
libboost and libboost-dev 1.46.1 (our case) or higher.

---------------------------------------  Explanations to convert  ---------------------------------
1) Where is the source code?
	see {Project root}/convert/

2) How to build it?
	{Project root}$ make convert

3) How to use it?
	There are some parameters for "convert":

    -h Help messages

    -g The original SNAP file (in edgelist or edgelist_map). 
        They are assumed to be sorted by source vertex ID.

    -t The type of the original SNAP file, i.e., two possibilities: edgelist or adjlist

    -d Destination folder. Remember to add a slash at the end of your dest folder:
        e.g., "/home/yourname/data/" or "./data/"


4) Example usage:
    {Project root}$ sudo convert -g ../source-graph/twitter_rv.net -t edgelist -d ../dest-graph/ 

---------------------------------------------------------------------
The end. enjoy!

