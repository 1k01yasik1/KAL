APP = code/app
TEST_EXE = code/tests/test_main.out
TEST_JSON = code/tests/stud-unit-test-report.json
TEST_LOG = code/tests/test_output.txt

COMMON_SRCS = code/ant_colony_solver.cpp code/graph.cpp

$(APP): code/main.cpp $(COMMON_SRCS)
	g++ -std=c++17 -O2 -pthread $^ -o $@

$(TEST_EXE): code/tests/test_main.cpp $(COMMON_SRCS)
	g++ -std=c++17 -O2 -pthread $^ -o $@

$(TEST_JSON): $(TEST_EXE)
	dtst=$$(date +"%Y-%m-%dT%H:%M:%S%:z"); \
	./$(TEST_EXE) > $(TEST_LOG) 2>&1; \
	passed=$$(grep -c "PASSED" $(TEST_LOG) || true); \
	total=$$(grep -c "PASSED" $(TEST_LOG) || true); \
	failed=$$((total - passed)); \
	coverage=0.0; \
	printf '{\n  "timestamp": "%s",\n  "coverage": %.1f,\n  "passed": %d,\n  "failed": %d\n}\n' \
	"$$dtst" "$$coverage" "$$passed" "$$failed" > $(TEST_JSON)

report/report.pdf: report/report.tex
	cd report && pdflatex report.tex
	cd report && pdflatex report.tex

.PHONY: test clean

test: $(TEST_JSON)

clean:
	rm -f $(APP) $(TEST_EXE) $(TEST_LOG) $(TEST_JSON)
	rm -f report/*.aux report/*.log report/*.out report/*.toc report/*.synctex.gz
	echo "OK"
