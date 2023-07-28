#JTARGET=calc_momentum
#TARGET=ratio_ptrue_prec
TARGET = mu_pi_ratio

#FEDRALIBS := -lEIO -lEdb -lEbase -lEdr -lScan -lAlignment -lEmath -lEphys -lvt -lDataConversion -lEDA -lShower -lScan -lMLP -lSpectrum
FEDRALIBS := -lEdr -lAlignment -lEIO -lEdb -lEbase -lScan  -lEmath -lEphys -lvt -lDataConversion -lEDA -lShower -lScan -lMLP -lSpectrum 

LIBS := ../../include
SRC := ../../src/*.cpp

MOMENTUM_LIBS := /home/mnonaka/environment/FASERSoft/FnuMomCoord/include
MOMENTUM_SRC := /home/mnonaka/environment/FASERSoft/FnuMomCoord/src/*.cpp

$TARGET: $(TARGET).cpp
	g++ $(TARGET).cpp -g3 -w -I$(LIBS) $(SRC) -I$(MOMENTUM_LIBS) $(MOMENTUM_SRC)  `root-config --cflags` -I$(FEDRA_ROOT)/include -L$(FEDRA_ROOT)/lib  $(FEDRALIBS) `root-config --libs` `root-config --glibs` `root-config --evelibs` -o $(TARGET)

clean:
	$(RM) $(TARGET)
