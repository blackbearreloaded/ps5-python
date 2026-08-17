PYTHON ?= python3
POWERSHELL ?= powershell.exe
PS5_PAYLOAD_SDK ?= /opt/ps5-payload-sdk
CPYTHON_SRC ?= upstream/cpython
PS5_JOBS ?= 2
SCRIPT ?= examples/main.py
APP ?= apps/hello

.PHONY: host-test host-suite host-lifetime host-app host-build source-fetch source-check ps5-check ps5-configure ps5-core ps5-run ps5-test ps5-suite ps5-lifetime ps5-app ps5-web clean

host-test:
	$(POWERSHELL) -NoProfile -ExecutionPolicy Bypass -File host/run_core_tests.ps1

host-suite:
	$(POWERSHELL) -NoProfile -ExecutionPolicy Bypass -File host/run_core_suite.ps1

host-lifetime:
	$(POWERSHELL) -NoProfile -ExecutionPolicy Bypass -File host/run_lifetime_suite.ps1

host-app:
	$(POWERSHELL) -NoProfile -ExecutionPolicy Bypass -File host/run_app.ps1 -AppPath "$(APP)"

host-build:
	bash tools/build_host.sh

source-fetch:
	$(POWERSHELL) -NoProfile -ExecutionPolicy Bypass -File tools/fetch_cpython.ps1

source-check:
	@test -f "$(CPYTHON_SRC)/Include/Python.h"
	@test -f "$(CPYTHON_SRC)/configure.ac"
	@echo "CPython source: $(CPYTHON_SRC)"
	@git -C "$(CPYTHON_SRC)" describe --tags --always

ps5-check:
	@test -f "$(PS5_PAYLOAD_SDK)/toolchain/prospero.mk"
	@test -f "$(CPYTHON_SRC)/Include/Python.h"
	@echo "PS5 SDK and CPython source are present."

ps5-configure: host-build
	bash tools/build_ps5.sh configure

ps5-core: host-build
	PS5_JOBS=$(PS5_JOBS) bash tools/build_ps5.sh core

ps5-run: host-build
	PS5_JOBS=$(PS5_JOBS) bash tools/run_ps5.sh "$(SCRIPT)"

ps5-test: host-build
	PS5_JOBS=$(PS5_JOBS) RUN_TIMEOUT=120 bash tools/run_ps5.sh tests/core_suite.py

ps5-suite: ps5-test
	PS5_JOBS=$(PS5_JOBS) bash tools/run_ps5_lifetime.sh

ps5-lifetime: host-build
	PS5_JOBS=$(PS5_JOBS) bash tools/run_ps5_lifetime.sh

ps5-app: host-build
	PS5_JOBS=$(PS5_JOBS) bash tools/run_ps5_app.sh "$(APP)"

ps5-web: host-build
	PS5_JOBS=$(PS5_JOBS) bash tools/run_ps5_web.sh

clean:
	rm -rf build
