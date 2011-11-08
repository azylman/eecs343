set style data lines
set term png
set output "kma_output.png"
set xlabel "allocation trace index"
set ylabel "bytes"
plot "kma_output.dat" using 1:2 title 'bytes allocated', '' using 1:3 title 'page bytes in use'

w(x, y) = (y - x) / x

set output "kma_waste.png"
set ylabel "inefficiency"
set logscale y
plot "kma_output.dat" using 1:(w($2,$3)) title 'inefficiency'
