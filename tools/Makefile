export CC  = gcc
export CXX = g++
export CFLAGS = -Wall -O3 -msse2 -I..

# specify tensor path
INSTALL_PATH= ../bin
BIN = make_feature_buffer line_shuffle make_ugroup_buffer svdpp_randorder line_reorder combine_ugroup kddcup_combine_ugroup
OBJ = apex_svd_data.o
.PHONY: clean all

all: $(BIN)
export LDFLAGS= -pthread -lm 

apex_svd_data.o:../apex_svd_data.cpp ../apex_svd_data.h
make_feature_buffer:make_feature_buffer.cpp apex_svd_data.o ../apex_svd_data.h
make_ugroup_buffer:make_ugroup_buffer.cpp apex_svd_data.o ../apex_svd_data.h
line_shuffle:line_shuffle.cpp 
svdpp_randorder:svdpp_randorder.cpp 
line_reorder:line_reorder.cpp
combine_ugroup:combine_ugroup.cpp apex_svd_data.o
kddcup_combine_ugroup:kddcup_combine_ugroup.cpp apex_svd_data.o

$(BIN) : 
	$(CXX) $(CFLAGS) $(LDFLAGS) -o $@ $(filter %.cpp %.o %.c, $^)

$(OBJ) : 
	$(CXX) -c $(CFLAGS) -o $@ $(filter %.cpp %.c, $^)

install:
	cp -f -r $(BIN)  $(INSTALL_PATH)

clean:
	$(RM) $(OBJ) $(BIN) *~

