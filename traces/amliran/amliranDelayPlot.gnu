set terminal post eps color enh solid font "Times-Roman,18"

unset key
set size 0.5,0.5


set output "amliranDelay.eps"
set xlabel "Time (s)" offset 0.0,0.2
set ylabel "Queue Delay (ms)"  offset 1.5,0.0
set xrange [0:10]
set yrange [0:20]
plot "delay.txt" with linespoints pointtype 3 pointsize 0.1 lc rgb "orange  #ffa500 = 255 165   0"

