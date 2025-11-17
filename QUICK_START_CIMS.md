# CIMS System Quick Start Guide

## Problem Diagnosis

If you encounter `make: *** No rule to make target 'test'.  Stop.`

### Step 1: Confirm You're in the Correct Directory
```bash
# Check current directory
pwd

# Should show something like: /home/qd2090/.../concurrent_hash_table
# or contain your project path

# Check if Makefile exists
ls -la Makefile

# If not found, locate project directory
find ~ -name "Makefile" -path "*/concurrent_hash_table/*" 2>/dev/null
```

### Step 2: Enter Project Directory
```bash
# If project is in a subdirectory, enter it
cd concurrent_hash_table
# or
cd ~/concurrent_hash_table
# or adjust according to your actual path

# Check again
ls -la | grep -E "(Makefile|test.cpp|benchmark.cpp)"
```

### Step 3: Check Makefile Content
```bash
# View first few lines of Makefile, confirm test target exists
head -20 Makefile
```

### Step 4: Try Compilation
```bash
# Ensure in project root directory
pwd
ls Makefile

# Then compile
make test
```

## If Project Files Not in Current Directory

### Option A: Project in Subdirectory
```bash
# Find project
find . -name "test.cpp" -o -name "Makefile" 2>/dev/null

# Enter found directory
cd <found directory>
make test
```

### Option B: Need to Clone or Upload Project from Git
```bash
# If project is in git repository
git clone <your-repo-url>
cd concurrent_hash_table
make test
```

### Option C: Check if in Wrong Directory
```bash
# View current directory contents
ls -la

# If you see concurrent_hash_table directory
cd concurrent_hash_table
make test
```

## Complete Checklist

Run these commands and tell me the output:

```bash
# 1. Current directory
pwd

# 2. List all files
ls -la

# 3. Find Makefile
find . -maxdepth 2 -name "Makefile" 2>/dev/null

# 4. Find test.cpp
find . -maxdepth 2 -name "test.cpp" 2>/dev/null

# 5. Check if concurrent_hash_table directory exists
ls -d */ | grep concurrent
```
