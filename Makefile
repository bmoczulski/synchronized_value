CXXFLAGS += -std=c++20
CPPFLAGS += -D SV_DEVELOPMENT=1 -ggdb3

all: example_basic example_const example_different_lockables example_shared example_using

test: all
	./example_basic
	./example_const
	./example_different_lockables
	./example_shared
	./example_using

clean:
	$(RM) example_basic example_const example_different_lockables example_shared example_using
