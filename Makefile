## 
WXCONFIG=wx-config
ALCONFIG=pkg-config freealut

CXXFLAGS = $(shell $(WXCONFIG) --cxxflags) $(shell $(ALCONFIG) --cflags) -O2 -g
LIBS = $(shell $(WXCONFIG) --libs adv) $(shell $(ALCONFIG) --static --libs)
OUTFILE=klangwunder3000
#
#
#

OBJS=KW3KApp.o Klangset.o gui/FrameMain.o gui/MyFrameMain.o 

#
#

.PHONY : clean

all: $(OUTFILE) 

$(OUTFILE):	$(OBJS) 	 
	$(CXX) $(OBJS) -o $(OUTFILE) $(LIBS)


# clean me up, scotty
clean:
	$(RM) $(OUTFILE) $(OBJS)  *~ gui/*~


# deps
KW3KApp.o: KW3KApp.h
Klangset.o: Klangset.h
gui/FrameMain.o: gui/FrameMain.h
gui/MyFrameMain.o: gui/MyFrameMain.h gui/FrameMain.o


