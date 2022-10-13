set terminal post eps color enh solid font "Times-Roman,18"


set key box top left font "Times-Roman,14"
set size 0.5,0.5

set output "cubicRate.eps"
set xlabel "Time (s)" offset 0.0,0.2
set ylabel "Rate (Mbps)" offset 1.5,0.0
set xrange [0:10]
set yrange [0:35]
plot "schedRate.txt" with line title "Capacity" lw 2 lc rgb "olive  #a08020 = 160 128  32","10.1.1.1_49153_10.1.1.2_5000_goodput.txt" with line title "Throughput" lw 2 lc rgb "web-green  #00c000 =   0 192   0"


