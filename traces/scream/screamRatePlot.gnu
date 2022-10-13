set terminal post eps color enh solid font "Times-Roman,18"

set key box top left font "Times-Roman,14"
set size 0.5,0.5


set output "screamRate.eps"
set xlabel "Time (s)" offset 0.0,0.2
set ylabel "Rate (Mbps)" offset 1.5,0.0
set xrange [0:10]
set yrange [0:35]
plot "schedRate.txt" with line title "Capacity" lw 2 lc rgb "olive  #a08020 = 160 128  32","recRate.txt" with line title "Throughput" lw 2 lc rgb "web-green  #00c000 =   0 192   0"

#,"stream0_rate.txt" with line title "Stream 0" lw 2 lc rgb "gold #ffd700 = 255 215   0","stream1_rate.txt" with line title "Stream 1" lw 2 lc rgb "goldenrod #ffc020 = 255 192  32"

#,"send0_rate.txt" with line title "send 0" lc rgb "dark-yellow        #c8c800 = 200 200   0","send1_rate.txt" with line title "send 1" lc rgb "dark-yellow        #c8c800 = 200 200   0"


