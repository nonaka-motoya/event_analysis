#TARGET=calc_momentum
#TARGET=ratio_ptrue_prec
TARGET=check_graph

#FEDRALIBS := -lEIO -lEdb -lEbase -lEdr -lScan -lAlignment -lEmath -lEphys -lvt -lDataConversion -lEDA -lShower -lScan -lMLP -lSpectrum
FEDRALIBS := -lEdr -lAlignment -lEIO -lEdb -lEbase -lScan  -lEmath -lEphys -lvt -lDataConversion -lEDA -lShower -lScan -lMLP -lSpectrum 

LIBS := ../include
SRC := ../src/*.cpp

$TARGET: $(TARGET).cpp
	g++ $(TARGET).cpp -w -I$(LIBS) $(SRC)  `root-config --cflags` -I$(FEDRA_ROOT)/include -L$(FEDRA_ROOT)/lib  $(FEDRALIBS) `root-config --libs` `root-config --glibs` `root-config --evelibs` -o $(TARGET)

clean:
	$(RM) $(TARGET)
