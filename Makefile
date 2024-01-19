CC=gcc

compile :
	$(CC) cmin.c -o cmin
	$(CC) test_target.c -o test

#test case 
test :
	$(CC) test_target.c -o test

test_run : 
	./cmin -i input.txt -m "Hello" -o reduced ./test
rbal:
	./cmin -i ./target/balance/testcases/fail -m "AddressSanitizer: heap-buffer-overflow" -o reduced ./target/balance/balance

rjsmn:
	./cmin -i target/jsmn/testcases/crash.json -m "AddressSanitizer: heap-buffer-overflow" -o reduced target/jsmn/jsondump
jsmn:
	target/jsmn/jsondump < target/jsmn/testcases/crash.json

test_jsmn :
	target/jsmn/jsondump < reduced
rjsmn_j : 
	./run -i  target/jsmn/testcases/crash.json -m "heap-buffer-overflow" -o reduced target/jsmn/jsondump

rlibp:
	./cmin -i crash.png -m "MemorySanitizer: use-of-uninitialized-value" -o reduced ./libpng/test_pngfix

test_libx :
	target/libxml2/xmllint --recover --postvalid < reduced
rlibx:
	./cmin -i target/libxml2/testcases/crash.xml -m "SEGV" -o reduced "target/libxml2/xmllint --recover --postvalid -"
libxml:
	target/libxml2/xmllint --recover --postvalid - < target/libxml2/testcases/crash.xml

libpng : 
	./target/libpng/libpng/test_pngfix < ./target/libpng/crash.png