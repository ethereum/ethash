.PHONY: test go-test clean

test: go-test ./cpp-build/test/Test
	./cpp-build/test/Test

go-test:
	make -C go-ethash test

cpp-build/Makefile:
	mkdir -p cpp-build/
	cd cpp-build/ && cmake ..

cpp-build/test/Test: cpp-build/Makefile
	make -C cpp-build/ Test

clean:
	rm -rf cpp-build/
