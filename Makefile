SHELL := /usr/bin/env bash
export GOCACHE := $(CURDIR)/.cache/go-build

.PHONY: bootstrap format format-check lint test coverage e2e check dev

bootstrap:
	cmake -S cpp -B cpp/build -DCMAKE_BUILD_TYPE=Debug
	go -C backend mod download
	npm --prefix web ci
	npm --prefix web exec playwright install chromium

format:
	gofmt -w backend
	cmake -S cpp -B cpp/build -DCMAKE_BUILD_TYPE=Debug
	cmake --build cpp/build --target format
	npm --prefix web run format

format-check:
	./scripts/check-go-format.sh
	cmake -S cpp -B cpp/build -DCMAKE_BUILD_TYPE=Debug
	cmake --build cpp/build --target format-check
	npm --prefix web run format:check

lint:
	go -C backend vet ./...
	cmake --build cpp/build --target tidy
	npm --prefix web run lint
	npm --prefix web run typecheck

test:
	go -C backend test -race ./...
	cmake --build cpp/build --parallel
	ctest --test-dir cpp/build --output-on-failure
	npm --prefix web test

coverage:
	./scripts/coverage.sh

e2e:
	./scripts/e2e.sh

check: format-check lint coverage
	./scripts/hygiene.sh

dev:
	./scripts/dev.sh
