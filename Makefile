export CC  = gcc
export CXX = g++
export CFLAGS = -Wall -O3 -msse2 

# specify tensor path
INSTALL_PATH= ../bin
BIN = svd_feature svd_feature_infer
OBJ = apex_svd_data.o apex_svd.o apex_reg_tree.o
.PHONY: clean all

all: $(BIN)
export LDFLAGS= -pthread -lm 

svd_feature: svd_feature.cpp $(OBJ) apex_svd_data.h 
svd_feature_infer: svd_feature_infer.cpp $(OBJ) apex_svd_data.h 
apex_svd.o: apex_svd.cpp apex_svd.h apex_svd_model.h apex_svd_data.h solvers/*/*.h 
apex_svd_data.o: apex_svd_data.cpp apex_svd_data.h
apex_reg_tree.o: solvers/gbrt/apex_reg_tree.cpp solvers/gbrt/apex_reg_tree.h

$(BIN) : 
	$(CXX) $(CFLAGS) $(LDFLAGS) -o $@ $(filter %.cpp %.o %.c, $^)

$(OBJ) : 
	$(CXX) -c $(CFLAGS) -o $@ $(firstword $(filter %.cpp %.c, $^) )

install:
	cp -f -r $(BIN)  $(INSTALL_PATH)

clean:
	$(RM) $(OBJ) $(BIN) *~
