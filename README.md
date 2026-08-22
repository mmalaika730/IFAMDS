# IFAMDS | Intelligent Forest Advisory & Multi-Structure Decision System

A comprehensive C++ console-based forest management simulation system.

IFAMDS models a real-world forest management environment consisting of 10 forest zones arranged in a 5×5 grid. The system integrates multiple manually implemented data structures to process environmental data, detect fire risks, manage resources, coordinate emergency responses, and support intelligent decision-making.

## System Overview

The system operates through a menu-driven interface with 10 major modules:

1. Environmental Data Acquisition
2. Event Memory Management
3. Fire Detection & Control
4. Task Scheduling
5. Decision Intelligence
6. Spatial Routing
7. Hash-Based Fast Data Access
8. System Monitoring
9. Scenario Simulation
10. Forest Grid Monitoring

## Core Functionality

- Collects and validates environmental sensor readings including temperature, smoke, and humidity
- Stores timestamped environmental events
- Detects anomalies and potential fire risks
- Manages routine, surveillance, emergency, and multi-decision tasks
- Uses hierarchical decision trees for risk evaluation
- Predicts fire spread and computes safe evacuation paths
- Provides fast zone data retrieval using hash tables
- Monitors system performance and identifies bottlenecks
- Supports rollback and recovery of critical forest states
- Includes five scenario-based simulations for testing system behavior

## Data Structures

All major data structures were implemented manually without relying on external data-structure libraries.

### Arrays

- Static Array — baseline forest values
- Dynamic Array — live sensor streams for each zone
- 2D Static Array — baseline 5×5 forest grid
- 2D Dynamic Array — live forest grid

### Linked Lists

- Singly Linked List — raw event stream
- Singly Linked List — verified events
- Singly Linked List — anomaly events
- Doubly Linked List — forward correction
- Doubly Linked List — backward correction
- Doubly Linked List — synchronization chain
- Circular List — local monitoring
- Circular List — system monitoring
- Circular List — emergency monitoring
- Circular List — stability monitoring

### Stack

- Rollback Stack — stores forest grid snapshots for disaster recovery

### Queues

- FIFO Queue — routine tasks
- FIFO Queue — surveillance tasks
- Priority Queue / Min-Heap — emergency tasks
- FIFO Queue — multi-decision tasks

### Decision Trees

The system uses 12 hierarchical decision trees for:

- Zone hierarchy
- Sub-zone decomposition
- Terrain classification
- Water resources
- Fire control
- Equipment allocation
- Fire classification
- Wildlife activity
- Human activity
- Local decisions
- Regional escalation
- Global emergency response

### Graphs

- Adjacency List — forest zone connections and traversal
- Adjacency Matrix — fast connectivity checks
- BFS/DFS — spatial traversal and fire-spread analysis
- Safe evacuation path computation

### Hash Tables & Cache

- Linear probing hash table
- Chaining hash table
- FIFO cache for recently accessed records

## Scenarios

The system includes five complete scenarios demonstrating different aspects of the implementation.

### 1. Cascading Fire and Resource Conflict Resolution

Demonstrates fire detection, fire spread tracking, priority-based emergency response, anomaly tracking, and rollback recovery.

### 2. Sensor Failure and System Reconstruction

Demonstrates sensor validation, fault detection, reconstruction of missing data through spatial interpolation, and forward correction.

### 3. Multi-Factor Anomaly Escalation

Combines wildlife anomalies, fire risks, and human intrusion to demonstrate multi-factor risk evaluation and regional escalation.

### 4. System Overload and Load Redistribution

Demonstrates handling of high-volume sensor updates, emergency task prioritization, caching, load redistribution, and system recovery.

### 5. Global Multi-Zone Emergency Synchronization

Demonstrates coordinated emergency response across multiple forest zones, global risk evaluation, rollback, fire-spread analysis, and safe evacuation paths.

## Design Principles

### Multi-Layer Architecture

Data flows through independent processing layers, allowing different system components to handle specific responsibilities.

### Manual Data Structure Implementation

All major data structures are implemented manually as part of the Data Structures coursework.

### Time-Complexity Awareness

Major operations include time-complexity considerations and annotations.

### Scenario-Based Testing

Five complete scenarios are used to demonstrate and test the system's functionality under normal and emergency conditions.

## Technologies

- C++
- Data Structures & Algorithms
- Object-Oriented Programming
- SFML 3.0 for the graphical interface

## Project Structure

```text
IFAMDS/
│
├── src/
│   ├── ifamds_gui.cpp
│   └── dsaprojifamds.cpp
│
├── screenshots/
│   └── Project screenshots
│
├── report/
│   └── Project report
│
├── .gitignore
└── README.md
