.PHONY: help docs docs-serve docs-clean docs-check update-deps

help: ## Show this help message
	@echo "Available targets:"
	@grep -E '^[a-zA-Z_-]+:.*?## .*$$' $(MAKEFILE_LIST) | sort | awk 'BEGIN {FS = ":.*?## "}; {printf "  \033[36m%-20s\033[0m %s\n", $$1, $$2}'

docs: ## Build documentation (Doxygen + MkDocs)
	@./scripts/build-docs.sh

docs-serve: ## Build and serve documentation locally with auto-reload
	@./scripts/build-docs.sh --serve

docs-clean: ## Clean documentation build artifacts
	@./scripts/build-docs.sh --clean

docs-check: ## Check documentation for issues
	@./scripts/check-docs.sh

update-deps: ## Update Python dependencies (use 'make update-deps-upgrade' to upgrade)
	@./scripts/update-deps.sh

update-deps-upgrade: ## Update and upgrade Python dependencies to latest versions
	@./scripts/update-deps.sh --upgrade
