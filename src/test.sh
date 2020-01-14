./smp_cache 8192 8 64 4 0 trace/canneal.04t.debug > output.txt
diff -iw ../val.v2/MSI_debug.val output.txt
./smp_cache 8192 8 64 4 0 trace/canneal.04t.longTrace > output.txt
diff -iw ../val.v2/MSI_long.val output.txt
./smp_cache 8192 8 64 4 1 trace/canneal.04t.debug > output.txt
diff -iw ../val.v2/MESI_debug.val output.txt
./smp_cache 8192 8 64 4 1 trace/canneal.04t.longTrace > output.txt
diff -iw ../val.v2/MESI_long.val output.txt
./smp_cache 8192 8 64 4 2 trace/canneal.04t.debug > output.txt
diff -iw ../val.v2/Dragon_debug.val output.txt
./smp_cache 8192 8 64 4 2 trace/canneal.04t.longTrace > output.txt
diff -iw ../val.v2/Dragon_long.val output.txt
