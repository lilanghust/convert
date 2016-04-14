#!/bin/sh
if [ 0 -eq $# ];then
    echo please input graph file
    exit 1
fi
if [ 1 -eq $# ];then
    echo please input partition
    exit 1
fi
if [ ! -f $1.txt ];then
    echo $1.txt does not exist
    exit 1
fi
sudo ./bin/convert -t edgelist -g $1.txt -d ./data/ 
gpmetis data/$1_adjlist.txt $2 > data/output.info
awk '{print $0" "FNR-1}' data/$1_adjlist.txt.part.$2 > data/$1_tmp_1.txt
cat data/$1_tmp_1.txt | sort -n -k1 > data/$1_tmp_2.txt
awk '{print $1" "FNR-1" "$2}' data/$1_tmp_2.txt | sort -n -k3 > $1_b20s-vertices.txt
sudo ./bin/convert -t edgelist_map -g $1.txt -d ./
sort -k4n -k2n $1_b20s-edges.txt > $1_temp.txt
mv $1_temp.txt $1_b20s-edges.txt
