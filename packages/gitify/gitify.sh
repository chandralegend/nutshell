#!/bin/bash

# Check if we're in a git repository
if ! git rev-parse --is-inside-work-tree > /dev/null 2>&1; then
  echo "🥜 Error: Not in a git repository"
  exit 1
fi

# Show git status
echo "🥜 Current git status:"
git status -s

# Check if there are any changes
if [ -z "$(git status --porcelain)" ]; then
  echo "🥜 No changes to commit"
  exit 0
fi

# Ask for files to stage
echo "🥜 Select files to stage (space-separated list, or 'all' for all changes):"
read -r files_to_stage

if [ "$files_to_stage" = "all" ]; then
  git add .
  echo "🥜 All files staged"
else
  for file in $files_to_stage; do
    if git status -s "$file" > /dev/null 2>&1; then
      git add "$file"
      echo "🥜 Staged: $file"
    else
      echo "🥜 Warning: $file not found or not modified"
    fi
  done
fi

# Commit type selection
echo "🥜 Select commit type:"
commit_types=(
  "feat: A new feature"
  "fix: A bug fix"
  "docs: Documentation only changes"
  "style: Changes that do not affect the meaning of the code"
  "refactor: A code change that neither fixes a bug nor adds a feature"
  "perf: A code change that improves performance"
  "test: Adding missing tests or correcting existing tests"
  "build: Changes that affect the build system or external dependencies"
  "ci: Changes to CI configuration files and scripts"
  "chore: Other changes that don't modify src or test files"
)

select commit_type in "${commit_types[@]}"; do
  if [ -n "$commit_type" ]; then
    selected_type="${commit_type%%:*}"
    break
  fi
done

# Ask for commit scope (optional)
echo "🥜 Enter commit scope (optional, press enter to skip):"
read -r commit_scope

# Format scope if provided
if [ -n "$commit_scope" ]; then
  commit_scope="($commit_scope)"
fi

# Ask for commit message
echo "🥜 Enter commit message:"
read -r commit_message

# Build the full commit message
full_commit_message="$selected_type$commit_scope: $commit_message"

# Show the final commit message and confirm
echo "🥜 Final commit message: $full_commit_message"
echo "🥜 Proceed with commit? (y/n)"
read -r confirm

if [ "$confirm" = "y" ] || [ "$confirm" = "Y" ]; then
  git commit -m "$full_commit_message"
  echo "🥜 Changes committed successfully!"
else
  echo "🥜 Commit aborted"
fi
