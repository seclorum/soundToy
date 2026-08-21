#!/bin/bash

export LM_STUDIO_API_BASE=http://localhost:1234/v1
export LM_STUDIO_API_KEY=lm-studio   # any non-empty value
#  --model openai/qwen3.8-9b-mlx \
#  --model openai/deepseek-v4-pro-qwen3.5-9b-mtp \
#  --model openai/mlx-qwen3.5-9b-deepseek-v4-flash \
#  --openai-api-base http://localhost:1234/v1 \
#  --openai-api-key lm-studio \

aider \
  --model openai/mlx-qwen3.5-9b-deepseek-v4-flash \
  --architect \
  --edit-format diff \
  --no-auto-commits \
  toy.c


