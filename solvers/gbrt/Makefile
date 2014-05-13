# Makefile for SVDFeature customization

# !!specify HOME PATH
PRJ=../..
export CC  = gcc
export CXX = g++
export CFLAGS = -Wall -O3 -msse2 

#!! change name to variant's name 
BIN = svdf_gbrt svdf_gbrt_infer 
OBJ = apex_svd.o apex_svd_data.o apex_reg_tree.o
.PHONY: clean all

all: $(BIN)
export LDFLAGS= -pthread -lm 


# !! apex_reg_tree
apex_reg_tree.o: apex_reg_tree.cpp apex_reg_tree.h 

# !! use apex_svd_lite.cpp for customized solver
apex_svd.o:apex_gbrt.cpp $(PRJ)/apex_svd.h $(PRJ)/apex_svd_model.h $(PRJ)/apex_svd_data.h 
# !! reuse input component of SVDFeature
apex_svd_data.o:$(PRJ)/apex_svd_data.cpp $(PRJ)/apex_svd_data.h

svdf_gbrt:$(PRJ)/svd_feature.cpp $(OBJ) $(PRJ)/apex_svd_data.h $(PRJ)/apex_svd.h 
svdf_gbrt_infer:$(PRJ)/svd_feature_infer.cpp $(OBJ) $(PRJ)/apex_svd_data.h $(PRJ)/apex_svd.h 

$(BIN) : 
	$(CXX) $(CFLAGS) $(LDFLAGS) -o $@ $(filter %.cpp %.o %.c, $^)

$(OBJ) : 
	$(CXX) -c $(CFLAGS) -o $@ $(filter %.cpp %.c, $^)

clean:
	$(RM) $(OBJ) $(BIN) *~
