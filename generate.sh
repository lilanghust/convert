#!/bin/sh
#graphname partition destDir
if [ 3 -eq $# ];then
    echo the format of parameters is: input_graph_file partition destDir memSize\(MB\).
    exit 1
fi
if [ ! -f $1 ];then
    echo $1 does not exist
    exit 1
fi
if [ ! -d $3 ];then
    echo $3 does not exist
    exit 1
fi

graph=$1
part=$2
dir=$3
memory=$4
filename=${graph##*/}

#generate the adjacency list for gpmetis
sudo ./bin/convert -t edgelist -g $graph -d $dir -m $memory

#partition the graph into $2 partitions
gpmetis $dir/$filename'-adjlist' $part > $dir/output.info
mv $dir/$filename'-adjlist.part.'$part $dir/$filename'-adjlist.part'

#generate 2*2 matrix according to the result of gpmetis
sudo ./bin/convert -t edgelist_map -g $graph -d $dir -m $memory

