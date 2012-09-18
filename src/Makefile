
main: bin_watermarking.o flippability.o shuffling.o watermark_f.o
	gcc -g watermark_f.o bin_watermarking.o flippability.o shuffling.o -o fbw -lnetpbm -lz -lfprint

watermark_f.o: watermark_f.c
	gcc -g -c watermark_f.c

bin_watermarking.o: bin_watermarking.c bin_watermarking.h
	gcc -g -c bin_watermarking.c

flippability.o: flippability.c flippability.h
	gcc -g -c flippability.c flippability.h

shuffling.o: shuffling.c shuffling.h
	gcc -g -c shuffling.c shuffling.h

clean:
	rm -f watermark_f.o
	rm -f bin_watermarking.o
	rm -f shuffling.o
	rm -f flippability.o
	rm -f tester
	rm -f test_bw.o
	rm -f fbw

tester: test_bw.o flippability.o shuffling.o bin_watermarking.o
	gcc -g test_bw.o flippability.o shuffling.o bin_watermarking.o -o tester -lnetpbm -lz

test_bw.o: test_bw.c
	gcc -g -c test_bw.c
