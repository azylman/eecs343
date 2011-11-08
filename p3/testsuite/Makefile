all: testcases

testcases: 1.trace.new 2.trace.new 3.trace.new 4.trace.new 5.trace.new

1.trace.new:
	echo "$@: Short and sweet. Small allocations." >> README.traces.new
	./generate_trace 100 log 8 1000 uniform $@ >> README.traces.new
	echo "" >> README.traces.new

2.trace.new:
	echo "$@: Little bit longer. Larger allocations." >> README.traces.new
	./generate_trace 1000 log 8 4000 uniform $@ >> README.traces.new
	echo "" >> README.traces.new

3.trace.new:
	echo "$@: Even longer. Even larger allocations." >> README.traces.new
	./generate_trace 10000 log 8 8000 uniform $@ >> README.traces.new
	echo "" >> README.traces.new

4.trace.new:
	echo "$@: Same as 3.trace.new, but with linear allocation size distribution and smaller maximum size." >> README.traces.new
	./generate_trace 10000 linear 8 4000 uniform $@ >> README.traces.new
	echo "" >> README.traces.new

5.trace.new:
	echo "$@: Longest trace. High churn." >> README.traces.new
	./generate_trace 100000 log 8 8000 early $@ >> README.traces.new
	echo "" >> README.traces.new

clean:
	rm *.trace.new
	rm README.traces.new
	rm traceAllocation.*
