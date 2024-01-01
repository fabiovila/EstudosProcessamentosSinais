CC=gcc
LINK=gcc
CFLAGS=-g -Wall $(shell pkg-config --cflags glfw3 opengl gl) -O5 -mavx -Iinclude -Iotags
LDFLAGS=$(shell pkg-config --static --libs glfw3 opengl gl) -lSDL2_mixer -lm -lfftw3f 
STATIC=	otags/otag.a
SRC=$(wildcard src/*.c)
OBJS= $(patsubst %.c,%.o,$(SRC))
TARGET	=	src/main




rebuild:	clean $(TARGET)

build:	$(TARGET)

%.o: %.c
	@printf "\e[1;32mCompiling \e[1;33m%s\n\e[1;m" $^
	@$(CC)  $(CFLAGS) -c $^ -o $@
	
$(TARGET):	$(OBJS)

	@printf "\e[1;32mLinking   \e[1;33m%s\n\e[1;m" $^
	@$(LINK)  $^ -o $@ $(LDFLAGS) $(STATIC) 

all: $(PROGRAMS)

debug:
	@printf "\e[1;32m%s\n\e[1;m", $(SRC)
	@printf "\e[1;32m%s\n\e[1;m", $(OBJS)


check:
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --leak-check=yes ./$(PROGRAMS)

clean:
	@printf "\e[1;32mCleaning \e[1;33m%s\n\e[1;m" $^
	@rm -f $(TARGET) $(OBJS)

