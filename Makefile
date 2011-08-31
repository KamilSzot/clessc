CC = g++
CFLAGS = -Wall
SRCDIR = src
ODIR = build
OBJS = $(ODIR)/CssTokenizer.o $(ODIR)/CssParser.o $(ODIR)/Stylesheet.o \
  $(ODIR)/CssWriter.o $(ODIR)/main.o
EXEC = Interpreter
RM = rm

$(ODIR)/%.o : $(SRCDIR)/%.cpp $(SRCDIR)/%.h
	$(CC) $(CFLAGS) -c -o $@ $<

$(EXEC) : $(OBJS) 
	$(CC) $(CFLAGS) $^ -o $@

$(ODIR)/main.o : $(SRCDIR)/main.cpp
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	$(RM) $(ODIR)/*.o 
