.PHONY: all build run test clean
all: build
build:
	./build.sh
run: build
	./run.sh
test: build
	./scripts/static-test.sh
clean:
	rm -rf build/*.o build/*.elf build/*.bin build/*.img
