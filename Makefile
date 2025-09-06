CXXFLAGS += -std=c++20
CPPFLAGS += -D SV_DEVELOPMENT=1

all: example_basic example_const example_different_lockables example_shared

test: all
	./example_basic
	./example_const
	./example_different_lockables
	./example_shared

clean:
	$(RM) example_basic example_const example_different_lockables example_shared
