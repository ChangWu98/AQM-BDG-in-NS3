set terminal post eps color enh solid font "Times-Roman,18"
set output "scream.eps"

set multiplot layout 2,1
set key box bottom right font "Times-Roman,14"
set size 0.5,0.5
set xlabel "Time (s)" offset 0.0,0.2
set ylabel "Rate (Mbps)" offset 1.5,0.0
set xrange [0:10]
set yrange [0:35]
plot "schedRate.txt" with line title "Capacity" lw 2 lc rgb "olive  #a08020 = 160 128  32","recRate.txt" with line title "Throughput" lw 2 lc rgb "web-green  #00c000 =   0 192   0"

unset key
set size 0.5,0.5
set xlabel "Time (s)" offset 0.0,0.2
set ylabel "Queue Delay (ms)"  offset 1.5,0.0
set xrange [0:10]
set yrange [0:160]
plot "delay.txt" with linespoints pointtype 3 pointsize 0.2 lc rgb "orange  #ffa500 = 255 165   0"


