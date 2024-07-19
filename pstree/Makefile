NAME := $(shell basename $(PWD))
export MODULE := M1
all: $(NAME)-64 $(NAME)-32

include ../Makefile

# Test target to run the compiled binaries
ARGS1 :=  --version 
test1: $(NAME)-64 $(NAME)-32
	@echo "Testing $(NAME)-64..."
	@./$(NAME)-64 $(ARGS1)
	@echo "Testing $(NAME)-32..."
	@./$(NAME)-32  $(ARGS1)

ARGS2 := -p -n 
test2: $(NAME)-64 $(NAME)-32
	@echo "Testing $(NAME)-64..."
	@./$(NAME)-64 $(ARGS2)
	@echo "Testing $(NAME)-32..."
	@./$(NAME)-32  $(ARGS2)