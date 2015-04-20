#!/bin/sh

fail=0
for test_in in tests/*.in; do
	for test_cmd in tests/$(basename ${test_in} .in)_*.cmd; do
		cat $test_in | ./uclcmd $(cat $test_cmd) > test.out
		e=$?
		if [ $e -gt 0 ]; then
			echo Test[$(basename $test_cmd .cmd)] Failed. Error.
			fail=$(( $fail + 1 ))
		else
			res=$(diff -u tests/$(basename $test_cmd .cmd).res test.out)
			if [ $? -gt 0 ]; then
				echo Test[$(basename $test_cmd .cmd)] Failed. did not match.
				echo "$res"
				fail=$(( $fail + 1 ))
			else
				echo Test[$(basename $test_cmd .cmd)] Passed.
			fi
		fi

	done
done

exit $fail

