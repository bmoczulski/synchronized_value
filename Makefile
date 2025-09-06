CXXFLAGS += -std=c++20
CPPFLAGS += -D SV_DEVELOPMENT=1

all: example_basic

test: all
	./example_basic

clean:
	$(RM) example_basic
