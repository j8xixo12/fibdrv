  
set ylabel 'time(nsec)'
set xlabel 'input num'
set xtics 0,10
set style fill solid
set title 'Fibonacci with fast doubling'
set term png enhanced font 'Verdana, 10'
set output 'fibtime.png'
set format y
plot[:][:]'result.txt' \
using 1:2 with linespoints linewidth 2 title 'execution time'