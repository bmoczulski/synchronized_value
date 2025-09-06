CXXFLAGS += -std=c++20
CPPFLAGS += -D SV_DEVELOPMENT=1

all: example_basic example_const

test: all
	./example_basic
	./example_const

clean:
	$(RM) example_basic example_const
