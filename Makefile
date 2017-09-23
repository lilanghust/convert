# paths
OBJECT_DIR = obj
BINARY_DIR = bin
HEADERS_PATH = headers

#compile/link options
#SYSLIBS = -L/usr/local/lib -L/usr/lib64  -lboost_system -lboost_program_options -lboost_thread -lz -lrt -lboost_thread-mt
SYSLIBS = -L/usr/local/lib -L/usr/lib  -lboost_system -lboost_program_options -lboost_thread -lz -lrt -lm -lpthread
CXX?= g++
#CXXFLAGS?= -O3 -DNDEBUG -Wall -Wno-unused-function -I./$(HEADERS_PATH)
CXXFLAGS?= -O3 -DDEBUG -Wall -Wno-unused-function -I./$(HEADERS_PATH)
CXXFLAGS+= -Wfatal-errors

# make selections
CONVERT_SRC = convert.o process_edgelist.o process_adjlist.o edgelist_map.o radix_sort.o process_in_edge.o k_way_merge.o remap.o
CONVERT_OBJS= $(addprefix $(OBJECT_DIR)/, $(CONVERT_SRC))
CONVERT_TARGET=$(BINARY_DIR)/convert

all: $(CONVERT_TARGET) 
#all: $(CONVERT_TARGET) $(TEST_TARGET)

#dependencies
$(OBJECT_DIR):
	mkdir ./obj

$(BINARY_DIR):
	mkdir ./bin

#following lines defined for convert
$(OBJECT_DIR)/convert.o:convert/convert.cpp 
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(OBJECT_DIR)/radix_sort.o:convert/radix_sort.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(OBJECT_DIR)/process_edgelist.o:convert/process_edgelist.cpp 
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(OBJECT_DIR)/process_adjlist.o:convert/process_adjlist.cpp 
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(OBJECT_DIR)/edgelist_map.o:convert/edgelist_map.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(OBJECT_DIR)/remap.o:convert/remap.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(OBJECT_DIR)/process_in_edge.o:convert/process_in_edge.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(OBJECT_DIR)/k_way_merge.o:convert/k_way_merge.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(BINARY_DIR)/convert: $(CONVERT_OBJS)
	$(CXX) -o $@ $(CONVERT_OBJS) $(SYSLIBS)

$(CONVERT_OBJS): |$(OBJECT_DIR)
$(CONVERT_TARGET): |$(BINARY_DIR)

.PHONY:

convert: $(CONVERT_TARGET)

# utilities
cscope:
	find ./ -name "*.cpp" > cscope.files
	find ./ -name "*.c" >> cscope.files
	find ./ -name "*.h" >> cscope.files
	find ./ -name "*.hpp" >> cscope.files
	cscope -bqk

clean: 
	rm -f $(CONVERT_TARGET) $(CONVERT_OBJS)
	rm -f $(BINARY_DIR)/*
	rm -f cscope.*

