# Operating Systems Design Programming Assignments

This GitHub repository contains the programming assignments (P1 and P2) for the "Operating Systems Design" course at the University Carlos III of Madrid, as part of the Bachelor's Degree in Computer Science and Engineering. This repository was created on February 27, 2017, at 11:00 AM.

## P1 - Process Scheduler

The aim of this programming assignment is to develop a process scheduler in the C language. This assignment is composed of several tasks, each corresponding to different schedulers, each with distinct policies and features. For each task, a C code fulfilling a set of given requirements was developed. The following schedulers were implemented:

1. **Round-robin Scheduler**: Implements a round-robin scheduling policy.
2. **FIFO Scheduler**: Implements a first-in-first-out scheduling policy.
3. **FIFO Scheduler with Voluntary Context Switching**: Extends the FIFO scheduler by incorporating voluntary context switching.

For detailed information about each scheduler's implementation and requirements, please refer to the respective assignment folder.

## P2 - Userspace File System

The aim of this programming assignment is to develop a simplified userspace file system using the C language (ISO C11) on the Debian 9.4 operating system. The storage for this file system is backed by a file in the host operating system. This assignment includes the following tasks:

1. **Design**: Design the architecture of the file system, its control structures (e.g., i-nodes, superblock, or file descriptors), and algorithms to comply with the specified requirements and features.
2. **Implementation**: Implement the design in C language and develop client programs that interact with the file system's interface to access and modify files.
3. **Justification**: Provide a detailed explanation of the design and implementation choices, emphasizing their relation to the theoretical concepts covered in the Operating Systems Design subject. Address implementation-dependent or undefined behaviors.
4. **Validation Plan**: Create a validation plan to ensure that the implemented design meets the given requirements.
5. **Report**: Summarize and discuss the design, implementation, and validation in a written report.

For detailed information about the userspace file system's architecture, implementation, and requirements, please refer to the P2 assignment folder.

> [!NOTE]
> This repository is for educational purposes and is not intended for production use. Please adhere to the university's academic integrity policies when using this code as a reference for your assignments.
