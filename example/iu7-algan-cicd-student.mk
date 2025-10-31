ready/report.pdf:
	make -f makefile report/report.pdf
	mkdir -p ./ready
	cp report/report.pdf ready/report.pdf


ready/stud-unit-test-report.json:
	make -f makefile code/tests/stud-unit-test-report.json
	mkdir -p ./ready
	cp code/tests/stud-unit-test-report.json ready/stud-unit-test-report.json

.PHONY: clean
clean:
	make $@

