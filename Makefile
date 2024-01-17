CC=gcc

compile :
	$(CC) cmin_tmp.c -o cmin
	$(CC) test_target.c -o test

#test case 
test :
	$(CC) test_target.c -o test

test_run : 
	./cmin -i input.txt -m "Hello" -o reduced ./test
rbal:
	./cmin -i ./target/balance/testcases/fail -m "AddressSanitizer: heap-buffer-overflow" -o reduced ./target/balance/balance

rjsmn:
	./cmin -i target/jsmn/testcases/crash.json -m "MemorySanitizer: use-of-uninitialized-value" -o reduced ./target/jsmn/jsondump
rlibp:
	./cmin -i ./target/libpng/crash.png -m "MemorySanitizer: use-of-uninitialized-value" -o reduced ./target/libpng/libpng/test_pngfix

rlibx:

libxml:
	target/libxml2/xmllint --recover --postvalid - < target/libxml2/testcases/crash.xml
