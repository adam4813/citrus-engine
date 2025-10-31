.PHONY: help docs docs-serve docs-clean

help: ## Show this help message
	@echo "Available targets:"
	@grep -E '^[a-zA-Z_-]+:.*?## .*$$' $(MAKEFILE_LIST) | sort | awk 'BEGIN {FS = ":.*?## "}; {printf "  \033[36m%-20s\033[0m %s\n", $$1, $$2}'

docs: ## Build documentation (Doxygen + MkDocs)
	@./scripts/build-docs.sh

docs-serve: ## Build and serve documentation locally with auto-reload
	@./scripts/build-docs.sh --serve

docs-clean: ## Clean documentation build artifacts
	@./scripts/build-docs.sh --clean
