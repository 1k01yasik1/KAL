TEST_EXE = code/tests/test_main.out
TEST_JSON = code/tests/stud-unit-test-report.json
TEST_LOG = code/tests/test_output.txt

$(TEST_EXE): code/tests/test_main.cpp code/mult_algos.cpp code/matrix_utils.cpp
	g++ -std=c++17 $^ -o $@

$(TEST_JSON): $(TEST_EXE)
	dtst=$$(date +"%Y-%m-%dT%H:%M:%S%:z"); \
	./$(TEST_EXE) > $(TEST_LOG) 2>&1; \
	passed=$$(grep -c "PASSED" $(TEST_LOG) || true); \
	total=$$(grep -c "TEST" $(TEST_LOG) || true); \
	failed=$$((total - passed)); \
	coverage=0.1; \
	printf '{\n  "timestamp": "%s",\n  "coverage": %.1f,\n  "passed": %d,\n  "failed": %d\n}\n' \
	"$$dtst" "$$coverage" "$$passed" "$$failed" > $(TEST_JSON)

report/report.pdf: report/report.tex 
	cd report && pdflatex report.tex 
	cd report && pdflatex report.tex

.PHONY: clean
clean:
	rm -rf ready
	rm -f $(TEST_LOG) $(TEST_JSON) $(TEST_EXE) report/*.aux report/*.log report/*.out report/*.toc report/*.synctex.gz
	echo "OK"

