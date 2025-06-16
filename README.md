# 🗃️ Distributed Student Record System using MPI in C++

This project implements a **parallel, distributed in-memory database engine** using **MPI (Message Passing Interface)** in C++. It simulates the execution of SQL-like queries (`INSERT`, `SELECT`) across multiple nodes, distributing records and processing search conditions in parallel.

Built for the **Parallel Computing course at Marist College**, this system is a strong demonstration of real-world **message-passing systems**, **query parsing**, and **inter-node communication**.

---

## 📖 Overview

- Each MPI process acts as a distributed database node
- Records are inserted and stored based on a custom hash function
- SELECT queries are broadcast, and matching records are aggregated by the root node
- Supports simple SQL-like syntax with parsing for:
  - `INSERT INTO students VALUES ('Name', 'Major', Year);`
  - `SELECT * FROM students WHERE major='...' AND graduation_year=...;`

---

## 🚀 Features

### 🧵 Multi-Node Query Processing (MPI)
- Uses `MPI_Bcast`, `MPI_Send`, and `MPI_Recv` for inter-process communication
- Broadcasts queries from root to all worker nodes
- SELECT results from all workers are sent back to the root node and printed

### 🧠 Manual SQL Parsing & Hash-Based Distribution
- Implements custom string utilities for parsing without `<string>` dependencies
- Student records are hashed by `major` to assign them to a specific process
- Distributed insertions ensure balanced storage across nodes

### 🧮 Memory Usage Tracking
- Each node calculates memory usage based on record count
- Aggregates average memory usage across all nodes using `MPI_Reduce`

### 📊 Performance Metrics
- Measures total runtime using `MPI_Wtime()`
- Efficient for SELECT queries even on large input files

---

## 📂 Project Structure
```bash
mpi-student-db/
├── Project3.cpp # Main MPI simulation logic
├── Project3.sln # Visual Studio solution
├── Input & Output Files/
│ ├── queries # SQL-like commands
│ ├── output_1node, 2node, ... # Result sets for benchmarking
├── Project3.vcxproj # Project config files
├── README.md
```

---

## 💻 How to Run

### 🔧 Requirements

- MPI (OpenMPI or MS-MPI)
- C++ compiler (g++, Visual Studio)
- Multiple logical cores or cluster nodes

### 🧪 Sample Execution

**Linux/Mac (OpenMPI):**
```bash
mpic++ -o distributed_db Project3.cpp
mpirun -np 4 ./distributed_db < queries > output_4node.txt
```

Windows (MS-MPI):

```bash
mpiexec -n 4 Project3.exe < queries > output_4node.txt
```
You can adjust -np to test performance scaling.

### 📥 Input Example (queries)
```bash

INSERT INTO students VALUES ('Alice', 'CS', 2025);
INSERT INTO students VALUES ('Bob', 'EE', 2024);
SELECT * FROM students WHERE major='CS' AND graduation_year=2025;
EXIT;
```

### 📤 Output Example
```bash
Alice | CS | 2025
Runtime: 0.0059 seconds.
Average Memory Usage across all ranks: 4560 bytes
```

---

## 🧠 Concepts Demonstrated

Concept	Application in Project

MPI Communication	Broadcast and reduction of distributed data

Query Parsing	Custom SQL-like parser without dependencies

Hash Distribution	Student records assigned via hash(major)

Memory Profiling	Tracks bytes used per process

Performance Timing	Benchmarks using MPI_Wtime()

Multi-node Coordination	Root-worker communication model

---

## 🧑‍💻 Author

Reem Ooka

Full Stack Java Developer| AI/ML Researcher
