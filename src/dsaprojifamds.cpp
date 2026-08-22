// ============================================================
//   IFAMDS — Intelligent Forest Advisory & Multi-Structure
//            Decision System
//   CL2001 Data Structures Project 2026
//
//   COMPLETE BUILD  — All 10 Menus | All 5 Scenarios
//   Single-file implementation, no STL — all DS built manually
//
//   DATA STRUCTURES IMPLEMENTED
//   ────────────────────────────
//   Arrays      : A1 Static Baseline, A2 Dynamic Sensor Stream
//                 A3 Static Grid,     A4 Dynamic Grid
//   Linked Lists: L1-L3 Singly (raw/verified/anomaly)
//                 L4-L6 Doubly (forward/backward/sync)
//                 L7-L10 Circular (local/system/emergency/stability)
//   Stack       : Manual LIFO rollback (forest grid snapshots)
//   Queues      : Q1-Q2 FIFO, Q3 Min-Heap Priority, Q4 FIFO
//   Trees       : T1-T12 Decision Trees (zone/terrain/resource/
//                 incident/decision layers), all fully built
//   Graphs      : G1 Adjacency List, G2 Adjacency Matrix
//                 BFS fire-spread, DFS deep-path, safe-path,
//                 fire-aware dynamic cost update
//   Hash Tables : H1 Linear Probe, H2 Chaining, H3 FIFO Cache
//
//   SECTIONS
//   ─────────────────────
//   §0  Constants & Global Configuration
//   §1  Array Layer            (A1-A4)
//   §2  Linked List Layer      (L1-L10)
//   §3  Stack Layer            (rollback)
//   §4  Queue Layer            (Q1-Q4)
//   §5  Menu Handlers          (Menus 1-5)
//   §6  Graph Layer            (G1-G2, BFS, DFS)
//   §7  Decision Tree Layer    (T1-T12, fully populated)
//   §8  Hash Layer             (H1-H3)
//   §9  System Monitoring Layer
//   §10 Menu Handlers          (Menus 6-9)
//   §11 Scenarios              (10.1-10.5, all complete)
//   §12 Main Menu
// ============================================================

#include <iostream>
#include <cstring>   // strcpy, strcmp
#include <cmath>     // fabs
#include <ctime>     // time measurement

using namespace std;

// ============================================================
//  §0  CONSTANTS & GLOBAL CONFIGURATION
// ============================================================

const int MAX_ZONES        = 10;   // total forest zones
const int MAX_READINGS     = 50;   // max sensor readings stored
const int GRID_ROWS        = 5;    // 2-D forest grid dimensions
const int GRID_COLS        = 5;
const int MAX_EVENTS       = 300;  // linked-list event pool size
const int QUEUE_CAPACITY   = 50;   // max items per queue
const int STACK_CAPACITY   = 50;   // rollback stack depth
const int CIRC_LOOP_SIZE   = 8;    // circular monitor slots

// ── Anomaly thresholds (Section 2.1.3 of spec) ──────────────
const float TEMP_THRESHOLD     = 45.0f;   // C  → fire risk
const float SMOKE_THRESHOLD    = 70.0f;   // ppm → possible fire
const float HUMIDITY_THRESHOLD = 20.0f;   // %   → dry condition
const float NOISE_DELTA        = 15.0f;   // sudden spike filter
const float ANOMALY_THETA      = 20.0f;   // deviation from normal

// ── Static Baseline (A1) — normal forest conditions ─────────
// These values never change during execution.
// Time complexity of baseline lookup: O(1)
const float BASELINE_TEMP[MAX_ZONES]     = {25,25,25,25,25,25,25,25,25,25};
const float BASELINE_SMOKE[MAX_ZONES]    = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
const float BASELINE_HUMIDITY[MAX_ZONES] = {60,60,60,60,60,60,60,60,60,60};

// ============================================================
//  §1  ARRAY LAYER  (A1 – A4)
// ============================================================

// ── A2: Dynamic Sensor Stream — one row per zone ─────────────
// Stores live temperature / smoke / humidity readings.
// New readings are appended; count tracked per zone.
// Time complexity: insert O(1), full scan O(n)
struct SensorArray {
    float temperature[MAX_ZONES][MAX_READINGS];
    float smoke      [MAX_ZONES][MAX_READINGS];
    float humidity   [MAX_ZONES][MAX_READINGS];
    int   count      [MAX_ZONES];   // how many readings stored per zone

    void init() {
        for (int z = 0; z < MAX_ZONES; z++) {
            count[z] = 0;
            for (int r = 0; r < MAX_READINGS; r++) {
                temperature[z][r] = 0;
                smoke[z][r]       = 0;
                humidity[z][r]    = 0;
            }
        }
    }

    // Append one reading for a zone.  Returns false if zone full.
    bool addReading(int zone, float temp, float smk, float hum) {
        if (zone < 0 || zone >= MAX_ZONES) return false;
        if (count[zone] >= MAX_READINGS)   return false;
        int idx = count[zone];
        temperature[zone][idx] = temp;
        smoke      [zone][idx] = smk;
        humidity   [zone][idx] = hum;
        count[zone]++;
        return true;
    }

    // Return latest reading index for a zone (-1 if empty)
    int latestIdx(int zone) const {
        if (count[zone] == 0) return -1;
        return count[zone] - 1;
    }
};

// ── A3 / A4: 2-D Forest Grid Matrix ──────────────────────────
// Each cell holds the current environmental value of a zone.
// Static grid (A3) = baseline snapshot.
// Dynamic grid (A4) = updated live values.
// Time complexity: cell access O(1), full display O(rows*cols)
struct ForestGrid {
    float temperature[GRID_ROWS][GRID_COLS];
    float smoke      [GRID_ROWS][GRID_COLS];
    float humidity   [GRID_ROWS][GRID_COLS];

    void initBaseline() {
        for (int r = 0; r < GRID_ROWS; r++)
            for (int c = 0; c < GRID_COLS; c++) {
                temperature[r][c] = 25.0f;
                smoke[r][c]       =  0.0f;
                humidity[r][c]    = 60.0f;
            }
    }

    // Update one cell
    void setCell(int r, int c, float t, float s, float h) {
        if (r < 0 || r >= GRID_ROWS || c < 0 || c >= GRID_COLS) return;
        temperature[r][c] = t;
        smoke[r][c]       = s;
        humidity[r][c]    = h;
    }

    // Spatial interpolation for a missing cell (Section 2.1.3)
    // Uses average of up to 4 neighbours.
    // Time complexity: O(1) — fixed 4-neighbour check
    float interpolateTemp(int r, int c) const {
        float sum = 0; int cnt = 0;
        if (r > 0)            { sum += temperature[r-1][c]; cnt++; }
        if (r < GRID_ROWS-1)  { sum += temperature[r+1][c]; cnt++; }
        if (c > 0)            { sum += temperature[r][c-1]; cnt++; }
        if (c < GRID_COLS-1)  { sum += temperature[r][c+1]; cnt++; }
        return cnt > 0 ? sum / cnt : 25.0f;
    }
};

// ── Noise / Anomaly Filtering helpers ────────────────────────

// Returns true if a new reading is noisy (sudden spike) compared
// to the previous reading for that zone.
// Time complexity: O(1)
bool isNoise(float prev, float current) {
    return fabs(current - prev) >= NOISE_DELTA;
}

// Returns true if a value is outside physical limits.
// Time complexity: O(1)
bool isInvalid(float value, float minVal, float maxVal) {
    return (value < minVal || value > maxVal);
}

// Returns true if value deviates from normal by more than theta.
// Time complexity: O(1)
bool isAnomaly(float value, float normal) {
    return fabs(value - normal) > ANOMALY_THETA;
}

// Boundary detection between two adjacent zones.
// Returns true if there is a sharp boundary (possible fire spread).
// Time complexity: O(1)
bool isBoundary(float zoneA, float zoneB) {
    return fabs(zoneA - zoneB) > NOISE_DELTA;
}

// ── Global array instances ────────────────────────────────────
SensorArray sensorData;    // A2 — live sensor stream
ForestGrid  staticGrid;    // A3 — baseline snapshot
ForestGrid  dynamicGrid;   // A4 — live grid updated by sensors


// ============================================================
//  §2  LINKED LIST LAYER  (L1 – L10)
// ============================================================

// ── Event node — shared by all list variants ─────────────────
struct EventNode {
    float value;          // sensor reading
    int   timestamp;      // simulated time tick
    int   zone;           // forest zone index
    char  type[20];       // "TEMP" | "SMOKE" | "HUMIDITY" | "ANOMALY"
    bool  isAnomalous;
    bool  isVerified;

    EventNode* next;      // forward pointer  (all lists)
    EventNode* prev;      // backward pointer (doubly & circular only)
};

// ── Simple node pool to avoid dynamic allocation issues ──────
// Pool-based allocation: O(1) alloc, O(1) free
EventNode nodePool[MAX_EVENTS];
bool      poolUsed[MAX_EVENTS];
int       poolSize = MAX_EVENTS;

void initPool() {
    for (int i = 0; i < poolSize; i++) {
        poolUsed[i] = false;
        nodePool[i].next = nullptr;
        nodePool[i].prev = nullptr;
    }
}

EventNode* allocNode() {
    for (int i = 0; i < poolSize; i++) {
        if (!poolUsed[i]) {
            poolUsed[i] = true;
            nodePool[i].next = nullptr;
            nodePool[i].prev = nullptr;
            return &nodePool[i];
        }
    }
    return nullptr;  // pool exhausted
}

void freeNode(EventNode* n) {
    if (!n) return;
    for (int i = 0; i < poolSize; i++) {
        if (&nodePool[i] == n) { poolUsed[i] = false; return; }
    }
}

// ── Global timestamp counter ─────────────────────────────────
int globalTick = 0;

// ─────────────────────────────────────────────────────────────
//  L1 — Raw Event Stream (Singly Linked)
//  Stores direct sensor readings without filtering.
//  Traversal: forward only.  Time complexity: insert O(1) tail,
//  traverse O(n).
// ─────────────────────────────────────────────────────────────
struct SinglyList {
    EventNode* head;
    EventNode* tail;
    int        size;

    void init() { head = tail = nullptr; size = 0; }

    // Append to tail — O(1)
    void append(float val, int zone, const char* type,
                bool anomalous = false, bool verified = false) {
        EventNode* n = allocNode();
        if (!n) { cout << "[WARN] Event pool exhausted.\n"; return; }
        n->value       = val;
        n->timestamp   = globalTick++;
        n->zone        = zone;
        n->isAnomalous = anomalous;
        n->isVerified  = verified;
        strcpy(n->type, type);
        n->next = nullptr;
        if (!tail) { head = tail = n; }
        else       { tail->next = n; tail = n; }
        size++;
    }

    // Forward traversal — O(n)
    void traverseForward(const char* label) const {
        cout << "\n  [" << label << "] Forward Traverse (" << size << " events):\n";
        EventNode* cur = head;
        int i = 0;
        while (cur) {
            cout << "    [" << i++ << "] t=" << cur->timestamp
                 << "  Zone=" << cur->zone
                 << "  Type=" << cur->type
                 << "  Val="  << cur->value
                 << (cur->isAnomalous ? "  *** ANOMALY ***" : "")
                 << "\n";
            cur = cur->next;
        }
    }

    // Find last valid (non-anomalous, verified) node — O(n)
    EventNode* lastStable() const {
        EventNode* stable = nullptr;
        EventNode* cur    = head;
        while (cur) {
            if (!cur->isAnomalous && cur->isVerified) stable = cur;
            cur = cur->next;
        }
        return stable;
    }
};

// ─────────────────────────────────────────────────────────────
//  L2 — Verified Event Stream (Singly Linked)
//  Stores readings after noise removal.  Same structure as L1
//  but only verified nodes are appended.
// ─────────────────────────────────────────────────────────────
// (Re-uses SinglyList; separate instance declared below)

// ─────────────────────────────────────────────────────────────
//  L3 — Anomaly Event Stream (Singly Linked)
//  Stores only readings flagged as anomalous.
// ─────────────────────────────────────────────────────────────
// (Re-uses SinglyList; separate instance declared below)

// ─────────────────────────────────────────────────────────────
//  L4 – L6 — Doubly Linked Correction Chain
//  Supports both forward and backward traversal for corrections.
//  Time complexity: insert O(1) tail, traverse O(n),
//                   backward correction O(n)
// ─────────────────────────────────────────────────────────────
struct DoublyList {
    EventNode* head;
    EventNode* tail;
    int        size;

    void init() { head = tail = nullptr; size = 0; }

    // Append to tail — O(1)
    void append(float val, int zone, const char* type,
                bool anomalous = false, bool verified = false) {
        EventNode* n = allocNode();
        if (!n) { cout << "[WARN] Event pool exhausted.\n"; return; }
        n->value       = val;
        n->timestamp   = globalTick++;
        n->zone        = zone;
        n->isAnomalous = anomalous;
        n->isVerified  = verified;
        strcpy(n->type, type);
        n->next = nullptr;
        n->prev = tail;
        if (!tail) { head = tail = n; }
        else       { tail->next = n; tail = n; }
        size++;
    }

    // Forward traversal — O(n)
    void traverseForward(const char* label) const {
        cout << "\n  [" << label << "] Forward (doubly) (" << size << " events):\n";
        EventNode* cur = head;
        int i = 0;
        while (cur) {
            cout << "    [" << i++ << "] t=" << cur->timestamp
                 << "  Zone=" << cur->zone
                 << "  Type=" << cur->type
                 << "  Val="  << cur->value
                 << (cur->isAnomalous ? "  *** ANOMALY ***" : "")
                 << "\n";
            cur = cur->next;
        }
    }

    // Backward traversal (correction chain) — O(n)
    void traverseBackward(const char* label) const {
        cout << "\n  [" << label << "] Backward (correction) (" << size << " events):\n";
        EventNode* cur = tail;
        int i = size - 1;
        while (cur) {
            cout << "    [" << i-- << "] t=" << cur->timestamp
                 << "  Zone=" << cur->zone
                 << "  Val="  << cur->value
                 << "\n";
            cur = cur->prev;
        }
    }

    // Correct last anomalous node with a new value — O(n)
    bool correctLastAnomaly(float newVal) {
        EventNode* cur = tail;
        while (cur) {
            if (cur->isAnomalous) {
                cout << "    [CORRECT] Zone=" << cur->zone
                     << "  Old=" << cur->value
                     << "  New=" << newVal << "\n";
                cur->value       = newVal;
                cur->isAnomalous = false;
                cur->isVerified  = true;
                return true;
            }
            cur = cur->prev;
        }
        return false;
    }
};

// ─────────────────────────────────────────────────────────────
//  L7 – L10 — Circular Monitoring Loops
//  Last node points back to head for continuous monitoring.
//  Time complexity: insert O(1), one full cycle O(n)
// ─────────────────────────────────────────────────────────────
struct CircularList {
    EventNode* head;
    EventNode* tail;
    int        size;
    int        maxSize;   // capped at CIRC_LOOP_SIZE for display

    void init(int cap = CIRC_LOOP_SIZE) {
        head = tail = nullptr;
        size = 0;
        maxSize = cap;
    }

    // Append (overwrites oldest when full) — O(1)
    void append(float val, int zone, const char* type,
                bool anomalous = false) {
        if (size >= maxSize) {
            // Remove oldest (head) to keep loop bounded
            EventNode* old = head;
            if (size == 1) {
                head = tail = nullptr;
            } else {
                head       = head->next;
                tail->next = head;          // maintain circular link
            }
            freeNode(old);
            size--;
        }
        EventNode* n = allocNode();
        if (!n) return;
        n->value       = val;
        n->timestamp   = globalTick++;
        n->zone        = zone;
        n->isAnomalous = anomalous;
        n->isVerified  = true;
        strcpy(n->type, type);
        if (!head) {
            head = tail = n;
            n->next = n;   // self-loop
        } else {
            tail->next = n;
            n->next    = head;
            tail       = n;
        }
        size++;
    }

    // Traverse one full cycle — O(n)
    void traverseOneCycle(const char* label) const {
        if (!head) { cout << "  [" << label << "] Empty loop.\n"; return; }
        cout << "\n  [" << label << "] Circular loop (" << size << " slots):\n";
        EventNode* cur = head;
        for (int i = 0; i < size; i++) {
            cout << "    [" << i << "] t=" << cur->timestamp
                 << "  Zone=" << cur->zone
                 << "  Val="  << cur->value
                 << (cur->isAnomalous ? "  *** ANOMALY ***" : "")
                 << "\n";
            cur = cur->next;
        }
        cout << "    (- wraps back to head)\n";
    }
};

// ── Named list instances matching spec ───────────────────────
SinglyList L1_rawStream;          // raw events
SinglyList L2_verifiedStream;     // verified events
SinglyList L3_anomalyStream;      // anomaly events
DoublyList L4_forwardCorrect;     // forward correction chain
DoublyList L5_backwardCorrect;    // backward correction chain
DoublyList L6_syncChain;          // state synchronisation
CircularList L7_localLoop;        // local zone monitor loop
CircularList L8_systemLoop;       // system-wide monitor loop
CircularList L9_emergencyLoop;    // emergency monitor loop
CircularList L10_stabilityLoop;   // stability monitor loop

// ── Master event ingestion ────────────────────────────────────
// Routes one reading through L1 → L2/L3 → correction chains
// Time complexity: O(1) per reading
void ingestEvent(int zone, float val, const char* type, float baseline) {
    // L1 always receives raw reading
    L1_rawStream.append(val, zone, type, false, false);

    float prevVal = baseline;
    // Use last verified reading as prev if available — O(n) scan
    // (acceptable: called infrequently per sensor tick)
    {
        EventNode* cur = L2_verifiedStream.head;
        while (cur) {
            if (cur->zone == zone) prevVal = cur->value;
            cur = cur->next;
        }
    }

    bool noisy   = isNoise(prevVal, val);
    bool invalid = isInvalid(val, 0.0f, 120.0f);
    bool anomaly = isAnomaly(val, baseline);

    if (noisy || invalid) {
        // If noisy but also a clear physical anomaly, still log to L3/emergency
        if (!invalid && isAnomaly(val, baseline)) {
            L3_anomalyStream.append(val, zone, type, true, false);
            L4_forwardCorrect.append(val, zone, type, true, false);
            L5_backwardCorrect.append(val, zone, type, true, false);
            L9_emergencyLoop.append(val, zone, type, true);
            cout << "    [ANOMALY-SPIKE] Zone=" << zone << "  " << type
                 << "=" << val << " (spike + anomaly)\n";
        } else {
            cout << "    [FILTER] Noisy/invalid reading dropped: val=" << val
                 << "  zone=" << zone << "\n";
        }
        return;
    }

    if (anomaly) {
        // Goes to L3 (anomaly stream) and doubly chain for correction
        L3_anomalyStream.append(val, zone, type, true, false);
        L4_forwardCorrect.append(val, zone, type, true, false);
        L5_backwardCorrect.append(val, zone, type, true, false);
        L9_emergencyLoop.append(val, zone, type, true);
        cout << "    [ANOMALY] Zone=" << zone << "  " << type
             << "=" << val << " (baseline=" << baseline << ")\n";
    } else {
        // Clean reading goes to L2 and stability structures
        L2_verifiedStream.append(val, zone, type, false, true);
        L6_syncChain.append(val, zone, type, false, true);
        L7_localLoop.append(val, zone, type, false);
        L8_systemLoop.append(val, zone, type, false);
        L10_stabilityLoop.append(val, zone, type, false);
    }
}


// ============================================================
//  §3  STACK LAYER  (for rollback / execution control)
//      Used by Department 3 — Execution Control Layer
// ============================================================

// ── State snapshot stored on stack ───────────────────────────
struct ForestSnapshot {
    float gridTemp  [GRID_ROWS][GRID_COLS];
    float gridSmoke [GRID_ROWS][GRID_COLS];
    int   tick;
    char  reason[40];
};

// Manual stack — LIFO
// Push O(1), Pop O(1), Peek O(1)
struct SnapshotStack {
    ForestSnapshot data[STACK_CAPACITY];
    int            top;

    void init() { top = -1; }

    bool push(const ForestGrid& g, const char* reason) {
        if (top >= STACK_CAPACITY - 1) {
            cout << "[STACK] Overflow — cannot save snapshot.\n";
            return false;
        }
        top++;
        for (int r = 0; r < GRID_ROWS; r++)
            for (int c = 0; c < GRID_COLS; c++) {
                data[top].gridTemp [r][c] = g.temperature[r][c];
                data[top].gridSmoke[r][c] = g.smoke[r][c];
            }
        data[top].tick = globalTick;
        strcpy(data[top].reason, reason);
        cout << "  [STACK] Snapshot saved at tick=" << globalTick
             << "  reason=" << reason << "\n";
        return true;
    }

    bool pop(ForestGrid& g) {
        if (top < 0) {
            cout << "[STACK] Underflow — no saved state.\n";
            return false;
        }
        for (int r = 0; r < GRID_ROWS; r++)
            for (int c = 0; c < GRID_COLS; c++) {
                g.temperature[r][c] = data[top].gridTemp [r][c];
                g.smoke[r][c]       = data[top].gridSmoke[r][c];
            }
        cout << "  [STACK] State restored from tick=" << data[top].tick
             << "  reason=" << data[top].reason << "\n";
        top--;
        return true;
    }

    bool peek(ForestSnapshot& out) const {
        if (top < 0) return false;
        out = data[top];
        return true;
    }

    bool isEmpty() const { return top < 0; }
    int  depth()   const { return top + 1; }
};

SnapshotStack rollbackStack;


// ============================================================
//  §4  QUEUE LAYER  (Q1 – Q4)
//      Queue = FIFO.  Priority queue = min-heap (manual).
// ============================================================

// ── Task record ──────────────────────────────────────────────
struct Task {
    int   id;
    int   priority;    // 1 = highest, 5 = lowest
    int   zone;
    float value;
    char  description[50];
};

// ── Simple circular FIFO queue — O(1) enqueue/dequeue ────────
struct FifoQueue {
    Task data[QUEUE_CAPACITY];
    int  front, back, count;
    int  nextId;
    char name[20];

    void init(const char* qname) {
        front = back = count = 0;
        nextId = 1;
        strcpy(name, qname);
    }

    bool enqueue(int priority, int zone, float val, const char* desc) {
        if (count >= QUEUE_CAPACITY) {
            cout << "[" << name << "] Queue full.\n";
            return false;
        }
        data[back].id       = nextId++;
        data[back].priority = priority;
        data[back].zone     = zone;
        data[back].value    = val;
        strcpy(data[back].description, desc);
        back = (back + 1) % QUEUE_CAPACITY;
        count++;
        cout << "  [" << name << "] Enqueued: " << desc
             << "  zone=" << zone << "  val=" << val << "\n";
        return true;
    }

    bool dequeue(Task& out) {
        if (count == 0) {
            cout << "[" << name << "] Queue empty.\n";
            return false;
        }
        out   = data[front];
        front = (front + 1) % QUEUE_CAPACITY;
        count--;
        cout << "  [" << name << "] Dequeued: " << out.description
             << "  zone=" << out.zone << "\n";
        return true;
    }

    void display() const {
        cout << "\n  Queue [" << name << "]  count=" << count << ":\n";
        for (int i = 0; i < count; i++) {
            int idx = (front + i) % QUEUE_CAPACITY;
            cout << "    [" << i << "] id=" << data[idx].id
                 << "  pri=" << data[idx].priority
                 << "  zone=" << data[idx].zone
                 << "  desc=" << data[idx].description << "\n";
        }
    }

    bool isEmpty() const { return count == 0; }
};

// ── Manual Min-Heap Priority Queue (Q3) ──────────────────────
// Lower priority number = higher urgency.
// insert O(log n), extractMin O(log n)
struct PriorityQueue {
    Task data[QUEUE_CAPACITY];
    int  size;
    int  nextId;

    void init() { size = 0; nextId = 1; }

    // Swap helper
    void swapTasks(int i, int j) {
        Task tmp = data[i]; data[i] = data[j]; data[j] = tmp;
    }

    // Sift up after insert — O(log n)
    void siftUp(int i) {
        while (i > 0) {
            int parent = (i - 1) / 2;
            if (data[parent].priority > data[i].priority) {
                swapTasks(parent, i);
                i = parent;
            } else break;
        }
    }

    // Sift down after extract — O(log n)
    void siftDown(int i) {
        while (true) {
            int left  = 2*i + 1;
            int right = 2*i + 2;
            int smallest = i;
            if (left  < size && data[left ].priority < data[smallest].priority) smallest = left;
            if (right < size && data[right].priority < data[smallest].priority) smallest = right;
            if (smallest == i) break;
            swapTasks(i, smallest);
            i = smallest;
        }
    }

    bool insert(int priority, int zone, float val, const char* desc) {
        if (size >= QUEUE_CAPACITY) {
            cout << "[PQ] Priority queue full.\n";
            return false;
        }
        data[size].id       = nextId++;
        data[size].priority = priority;
        data[size].zone     = zone;
        data[size].value    = val;
        strcpy(data[size].description, desc);
        siftUp(size);
        size++;
        cout << "  [PQ-Emergency] Inserted: " << desc
             << "  pri=" << priority << "  zone=" << zone << "\n";
        return true;
    }

    bool extractMin(Task& out) {
        if (size == 0) {
            cout << "[PQ] Empty — nothing to process.\n";
            return false;
        }
        out       = data[0];
        data[0]   = data[size - 1];
        size--;
        siftDown(0);
        cout << "  [PQ-Emergency] Processed: " << out.description
             << "  pri=" << out.priority << "  zone=" << out.zone << "\n";
        return true;
    }

    void display() const {
        cout << "\n  [PQ-Emergency] " << size << " tasks pending:\n";
        for (int i = 0; i < size; i++) {
            cout << "    [" << i << "] id=" << data[i].id
                 << "  pri=" << data[i].priority
                 << "  zone=" << data[i].zone
                 << "  desc=" << data[i].description << "\n";
        }
    }

    bool isEmpty() const { return size == 0; }
};

// ── Queue instances ───────────────────────────────────────────
FifoQueue     Q1_routine;        // routine monitoring
FifoQueue     Q2_surveillance;   // continuous surveillance
PriorityQueue Q3_emergency;      // emergency response (priority)
FifoQueue     Q4_multiDecision;  // multi-factor decision tasks

// Pause/resume flag for Q1/Q2
bool q1Paused = false;
bool q2Paused = false;


// ============================================================
//  §5  MENU HANDLERS  (Menus 1 – 5)
// ============================================================

// ── Utility: print separator ─────────────────────────────────
void sep() {
    cout << "\n  ------------------------------------------\n";
}

void printHeader(const char* title) {
    cout << "\n  ____________________________________________\n";
    cout << "  |  " << title;
    // pad to fixed width
    int len = strlen(title);
    for (int i = len; i < 40; i++) cout << ' ';
    cout << "|\n";
    cout << "  ___________________________________________\n";
}

// ─────────────────────────────────────────────────────────────
//  MENU 1 — Input Environmental Data
// ─────────────────────────────────────────────────────────────
void menu1() {
    int choice;
    do {
        printHeader("1. Input Environmental Data");
        cout << "    1.1  Add Sensor Reading (Temp/Smoke/Wind)\n";
        cout << "    1.2  Store Data in Dynamic Array\n";
        cout << "    1.3  Compare with Static Baseline\n";
        cout << "    1.4  Validate and Filter Noise\n";
        cout << "    0    Back\n";
        cout << "  Choice: "; cin >> choice;

        if (choice == 1) {
            // ── 1.1  Add sensor reading ───────────────────
            // Also routes through linked-list ingestion.
            // Time complexity: O(n) due to L2 scan for prev value
            int zone;
            float temp, smoke, humidity;
            cout << "\n  Enter zone (0-" << MAX_ZONES-1 << "): "; cin >> zone;
            cout << "  Temperature (C): ";  cin >> temp;
            cout << "  Smoke level (ppm): "; cin >> smoke;
            cout << "  Humidity (%): ";      cin >> humidity;

            // Store in A2 dynamic sensor array — O(1)
            bool ok = sensorData.addReading(zone, temp, smoke, humidity);
            if (!ok) { cout << "  [ERROR] Zone out of range or array full.\n"; continue; }

            cout << "\n  [A2] Reading stored in dynamic array.\n";

            // Also update dynamic grid cell (map zone → row/col)
            // Simple mapping: zone = row*GRID_COLS + col
            int row = zone / GRID_COLS;
            int col = zone % GRID_COLS;
            dynamicGrid.setCell(row, col, temp, smoke, humidity);

            // Save snapshot before ingesting so rollback is possible
            rollbackStack.push(dynamicGrid, "pre-reading");

            // Route through linked list layer
            cout << "\n  [L-LAYER] Routing through event memory...\n";
            ingestEvent(zone, temp,     "TEMP",     BASELINE_TEMP[zone]);
            ingestEvent(zone, smoke,    "SMOKE",    BASELINE_SMOKE[zone]);
            ingestEvent(zone, humidity, "HUMIDITY", BASELINE_HUMIDITY[zone]);

        } else if (choice == 2) {
            // ── 1.2  Show dynamic array contents ─────────
            // Time complexity: O(zones * readings) = O(n)
            cout << "\n  [A2] Dynamic Sensor Array contents:\n";
            bool any = false;
            for (int z = 0; z < MAX_ZONES; z++) {
                if (sensorData.count[z] == 0) continue;
                any = true;
                cout << "  Zone " << z << "  (" << sensorData.count[z] << " readings):\n";
                for (int r = 0; r < sensorData.count[z]; r++) {
                    cout << "    [" << r << "]  T=" << sensorData.temperature[z][r]
                         << "  S=" << sensorData.smoke[z][r]
                         << "  H=" << sensorData.humidity[z][r] << "\n";
                }
            }
            if (!any) cout << "  (no data yet)\n";

        } else if (choice == 3) {
            // ── 1.3  Compare with static baseline (A1) ───
            // Time complexity: O(zones)
            cout << "\n  [A1 vs A2] Baseline Comparison:\n";
            cout << "  Zone | Temp  | Smoke | Humid | Status\n";
            cout << "  -----|-------|-------|-------|--------\n";
            bool any = false;
            for (int z = 0; z < MAX_ZONES; z++) {
                if (sensorData.count[z] == 0) continue;
                any = true;
                int i = sensorData.latestIdx(z);
                float dT = sensorData.temperature[z][i] - BASELINE_TEMP[z];
                float dS = sensorData.smoke[z][i]       - BASELINE_SMOKE[z];
                float dH = sensorData.humidity[z][i]    - BASELINE_HUMIDITY[z];
                bool alert = (sensorData.temperature[z][i] > TEMP_THRESHOLD  ||
                              sensorData.smoke[z][i]       > SMOKE_THRESHOLD ||
                              sensorData.humidity[z][i]    < HUMIDITY_THRESHOLD);
                cout << "   " << z
                     << "   |  " << dT
                     << "  |  "  << dS
                     << "  |  "  << dH
                     << "  |  "  << (alert ? "ALERT" : "OK") << "\n";
            }
            if (!any) cout << "  (no data yet)\n";

        } else if (choice == 4) {
            // ── 1.4  Validate and filter noise ───────────
            // Time complexity: O(zones * readings)
            cout << "\n  [FILTER] Noise / Anomaly Detection Report:\n";
            bool any = false;
            for (int z = 0; z < MAX_ZONES; z++) {
                int cnt = sensorData.count[z];
                if (cnt == 0) continue;
                any = true;
                cout << "  Zone " << z << ":\n";
                for (int r = 0; r < cnt; r++) {
                    float t = sensorData.temperature[z][r];
                    float s = sensorData.smoke[z][r];
                    float h = sensorData.humidity[z][r];
                    bool tAnomaly = isAnomaly(t, BASELINE_TEMP[z]);
                    bool sAnomaly = isAnomaly(s, BASELINE_SMOKE[z]);
                    bool hAnomaly = isAnomaly(h, BASELINE_HUMIDITY[z]);
                    cout << "    [" << r << "]  T=" << t << (tAnomaly ? "(!)" : "   ")
                         << "  S=" << s << (sAnomaly ? "(!)" : "   ")
                         << "  H=" << h << (hAnomaly ? "(!)" : "   ") << "\n";
                }
            }
            if (!any) cout << "  (no data yet)\n";
        }
    } while (choice != 0);
}

// ─────────────────────────────────────────────────────────────
//  MENU 2 — View Forest Grid Status
// ─────────────────────────────────────────────────────────────
void menu2() {
    int choice;
    do {
        printHeader("2. View Forest Grid Status");
        cout << "    2.1  Display 1D Time Series Data\n";
        cout << "    2.2  Display 2D Forest Zone Matrix\n";
        cout << "    2.3  View Zone-wise Conditions\n";
        cout << "    0    Back\n";
        cout << "  Choice: "; cin >> choice;

        if (choice == 1) {
            // ── 2.1  1D time series for one zone ─────────
            // Time complexity: O(n) — n readings for that zone
            int zone;
            cout << "\n  Enter zone (0-" << MAX_ZONES-1 << "): "; cin >> zone;
            int cnt = sensorData.count[zone];
            if (cnt == 0) { cout << "  No data for zone " << zone << "\n"; continue; }
            cout << "\n  [A2] 1D Time Series - Zone " << zone << " (" << cnt << " readings):\n";
            cout << "  Idx | Temp  | Smoke | Humid\n";
            cout << "  ----|-------|-------|------\n";
            for (int r = 0; r < cnt; r++) {
                cout << "   " << r
                     << "  |  " << sensorData.temperature[zone][r]
                     << "  |  " << sensorData.smoke[zone][r]
                     << "  |  " << sensorData.humidity[zone][r] << "\n";
            }
            // Trend detection: rising temperature?
            if (cnt >= 2) {
                float first = sensorData.temperature[zone][0];
                float last  = sensorData.temperature[zone][cnt-1];
                cout << "  Trend: temperature " << (last > first ? "RISING " :
                                                    last < first ? "FALLING " : "STABLE -") << "\n";
            }

        } else if (choice == 2) {
            // ── 2.2  2D forest grid matrix ────────────────
            // Time complexity: O(rows * cols)
            cout << "\n  [A3] STATIC Baseline Grid (Temperature):\n  ";
            for (int c = 0; c < GRID_COLS; c++) cout << "  C" << c;
            cout << "\n";
            for (int r = 0; r < GRID_ROWS; r++) {
                cout << "  R" << r;
                for (int c = 0; c < GRID_COLS; c++)
                    cout << "  " << staticGrid.temperature[r][c];
                cout << "\n";
            }

            cout << "\n  [A4] DYNAMIC Live Grid (Temperature):\n  ";
            for (int c = 0; c < GRID_COLS; c++) cout << "  C" << c;
            cout << "\n";
            for (int r = 0; r < GRID_ROWS; r++) {
                cout << "  R" << r;
                for (int c = 0; c < GRID_COLS; c++) {
                    float t = dynamicGrid.temperature[r][c];
                    if (t > TEMP_THRESHOLD) cout << " [!]";
                    else                    cout << "  " << t;
                }
                cout << "\n";
            }

            // Boundary detection across entire grid — O(rows*cols)
            cout << "\n  [BOUNDARY] Sharp temperature boundaries detected:\n";
            bool found = false;
            for (int r = 0; r < GRID_ROWS; r++) {
                for (int c = 0; c < GRID_COLS; c++) {
                    // Check right neighbour
                    if (c+1 < GRID_COLS) {
                        if (isBoundary(dynamicGrid.temperature[r][c],
                                       dynamicGrid.temperature[r][c+1])) {
                            cout << "    R" << r << "C" << c
                                 << "  R" << r << "C" << (c+1) << "\n";
                            found = true;
                        }
                    }
                    // Check bottom neighbour
                    if (r+1 < GRID_ROWS) {
                        if (isBoundary(dynamicGrid.temperature[r][c],
                                       dynamicGrid.temperature[r+1][c])) {
                            cout << "    R" << r << "C" << c
                                 << " ↔ R" << (r+1) << "C" << c << "\n";
                            found = true;
                        }
                    }
                }
            }
            if (!found) cout << "    None detected.\n";

        } else if (choice == 3) {
            // ── 2.3  Zone-wise condition summary ─────────
            // Time complexity: O(zones)
            cout << "\n  Zone-wise Latest Conditions:\n";
            cout << "  Zone | Temp    | Smoke   | Humid  | Status\n";
            cout << "  -----|---------|---------|--------|--------\n";
            for (int z = 0; z < MAX_ZONES; z++) {
                int i = sensorData.latestIdx(z);
                if (i < 0) {
                    cout << "   " << z << "   | (no data)\n";
                    continue;
                }
                float t = sensorData.temperature[z][i];
                float s = sensorData.smoke[z][i];
                float h = sensorData.humidity[z][i];
                const char* status =
                    (t > TEMP_THRESHOLD || s > SMOKE_THRESHOLD) ? "FIRE RISK" :
                    (h < HUMIDITY_THRESHOLD)                     ? "DRY"       : "NORMAL";
                cout << "   " << z
                     << "   | " << t
                     << "  | "  << s
                     << "  | "  << h
                     << "  | "  << status << "\n";
            }
        }
    } while (choice != 0);
}

// ─────────────────────────────────────────────────────────────
//  MENU 3 — Event Memory System
// ─────────────────────────────────────────────────────────────
void menu3() {
    int choice;
    do {
        printHeader("3. Event Memory System");
        cout << "    3.1  Store Event (Linked List)\n";
        cout << "    3.2  Traverse Event History Forward\n";
        cout << "    3.3  Traverse Event History Backward\n";
        cout << "    3.4  Circular Event Monitoring\n";
        cout << "    3.5  Restore Last Stable State\n";
        cout << "    0    Back\n";
        cout << "  Choice: "; cin >> choice;

        if (choice == 1) {
            // ── 3.1  Manual event store ───────────────────
            int zone; float val; int typeChoice;
            cout << "\n  Zone (0-" << MAX_ZONES-1 << "): "; cin >> zone;
            cout << "  Value: "; cin >> val;
            cout << "  Type  1=TEMP  2=SMOKE  3=HUMIDITY: "; cin >> typeChoice;
            const char* types[] = {"TEMP","SMOKE","HUMIDITY"};
            const char* t = (typeChoice >= 1 && typeChoice <= 3) ? types[typeChoice-1] : "TEMP";
            float base = (typeChoice==1) ? BASELINE_TEMP[zone] :
                         (typeChoice==2) ? BASELINE_SMOKE[zone] :
                                           BASELINE_HUMIDITY[zone];
            cout << "\n";
            ingestEvent(zone, val, t, base);
            cout << "  Event stored and routed through L1-L10.\n";

        } else if (choice == 2) {
            // ── 3.2  Forward traversal ────────────────────
            // Time complexity: O(n) for each list
            L1_rawStream.traverseForward("L1 Raw");
            L2_verifiedStream.traverseForward("L2 Verified");
            L3_anomalyStream.traverseForward("L3 Anomaly");
            L4_forwardCorrect.traverseForward("L4 FwdCorrection");
            L6_syncChain.traverseForward("L6 Sync");

        } else if (choice == 3) {
            // ── 3.3  Backward traversal ───────────────────
            // Time complexity: O(n) — doubly lists only
            L4_forwardCorrect.traverseBackward("L4 BackwardScan");
            L5_backwardCorrect.traverseBackward("L5 BackwardCorrect");
            cout << "\n  [CORRECT] Attempting to fix last anomaly in L5...\n";
            // Use interpolated value from grid as corrected value
            float corrVal = dynamicGrid.interpolateTemp(2, 2);
            bool fixed = L5_backwardCorrect.correctLastAnomaly(corrVal);
            if (!fixed) cout << "  No anomaly found to correct.\n";

        } else if (choice == 4) {
            // ── 3.4  Circular monitoring loops ───────────
            // Time complexity: O(n) per loop traversal
            L7_localLoop.traverseOneCycle("L7 Local Monitor");
            L8_systemLoop.traverseOneCycle("L8 System Monitor");
            L9_emergencyLoop.traverseOneCycle("L9 Emergency Monitor");
            L10_stabilityLoop.traverseOneCycle("L10 Stability Monitor");

        } else if (choice == 5) {
            // ── 3.5  Restore last stable state ───────────
            // Uses stack — O(1) pop
            if (rollbackStack.isEmpty()) {
                cout << "\n  No saved state to restore.\n";
            } else {
                bool ok = rollbackStack.pop(dynamicGrid);
                if (ok) {
                    cout << "  [RESTORED] Dynamic grid rolled back to last stable state.\n";
                    // Mark all L3 anomaly entries as needing review
                    cout << "  [INFO] " << L3_anomalyStream.size
                         << " anomaly events still in L3 stream.\n";
                }
            }
        }
    } while (choice != 0);
}

// ─────────────────────────────────────────────────────────────
//  MENU 4 — Fire Detection and Control
//  Forward declaration — full body defined after graph/tree
//  layers are in scope (see §12 menu handlers section).
// ─────────────────────────────────────────────────────────────
void menu4();   // defined after graph + tree layers

// ─────────────────────────────────────────────────────────────
//  MENU 5 — Task Scheduling System
// ─────────────────────────────────────────────────────────────
void menu5() {
    int choice;
    do {
        printHeader("5. Task Scheduling System");
        cout << "    5.1  Add Routine Task (Q1)\n";
        cout << "    5.2  Add Surveillance Task (Q2)\n";
        cout << "    5.3  Add Emergency Task (Q3 Priority Queue)\n";
        cout << "    5.4  Process Tasks (FIFO / Priority)\n";
        cout << "    5.5  Pause and Resume Tasks\n";
        cout << "    0    Back\n";
        cout << "  Choice: "; cin >> choice;

        if (choice == 1) {
            // ── 5.1  Add routine task to Q1 ──────────────
            // Time complexity: enqueue O(1)
            int zone; float val;
            cout << "\n  Zone: "; cin >> zone;
            cout << "  Sensor value: "; cin >> val;
            if (q1Paused) {
                cout << "  [Q1 PAUSED] Task queued but not processed.\n";
            }
            Q1_routine.enqueue(5, zone, val, "RoutineMonitor");

        } else if (choice == 2) {
            // ── 5.2  Add surveillance task to Q2 ─────────
            // Time complexity: enqueue O(1)
            int zone; float val;
            cout << "\n  Zone: "; cin >> zone;
            cout << "  Sensor value: "; cin >> val;
            if (q2Paused) {
                cout << "  [Q2 PAUSED] Task queued but not processed.\n";
            }
            Q2_surveillance.enqueue(3, zone, val, "SurveillanceScan");

        } else if (choice == 3) {
            // ── 5.3  Add emergency task to Q3 ─────────────
            // Time complexity: insert O(log n) — min-heap
            int zone, priority; float val;
            cout << "\n  Zone: ";     cin >> zone;
            cout << "  Priority (1=highest, 5=lowest): "; cin >> priority;
            cout << "  Sensor value: "; cin >> val;
            Q3_emergency.insert(priority, zone, val, "EmergencyTask");

        } else if (choice == 4) {
            // ── 5.4  Process tasks ────────────────────────
            int qChoice;
            cout << "\n  Process from:  1=Q1-Routine  2=Q2-Surv  3=Q3-Emergency  4=Q4-Decision\n";
            cout << "  Choice: "; cin >> qChoice;
            Task t;
            if (qChoice == 1) {
                if (q1Paused) { cout << "  Q1 is paused.\n"; }
                else          { Q1_routine.dequeue(t); }
            } else if (qChoice == 2) {
                if (q2Paused) { cout << "  Q2 is paused.\n"; }
                else          { Q2_surveillance.dequeue(t); }
            } else if (qChoice == 3) {
                Q3_emergency.extractMin(t);
            } else if (qChoice == 4) {
                Q4_multiDecision.dequeue(t);
            }

            // Display all queue states
            Q1_routine.display();
            Q2_surveillance.display();
            Q3_emergency.display();
            Q4_multiDecision.display();

        } else if (choice == 5) {
            // ── 5.5  Pause / Resume ───────────────────────
            // Time complexity: O(1)
            int qChoice;
            cout << "\n  1=Pause Q1  2=Resume Q1  3=Pause Q2  4=Resume Q2\n";
            cout << "  Choice: "; cin >> qChoice;
            if      (qChoice == 1) { q1Paused = true;  cout << "  Q1 PAUSED.\n"; }
            else if (qChoice == 2) { q1Paused = false; cout << "  Q1 RESUMED.\n"; }
            else if (qChoice == 3) { q2Paused = true;  cout << "  Q2 PAUSED.\n"; }
            else if (qChoice == 4) { q2Paused = false; cout << "  Q2 RESUMED.\n"; }
        }
    } while (choice != 0);
}


// ============================================================
//  §6  SCENARIOS 1 & 2
//      scenario3–5 defined in §11 after graph/tree layers.
// ============================================================

// ─────────────────────────────────────────────────────────────
//  Scenario 1 — Cascading Fire and Resource Conflict
//  Zone 3 catches fire → spreads toward Zone 4 and Zone 6.
// ─────────────────────────────────────────────────────────────
void scenario1() {
    printHeader("SCENARIO 1: Cascading Fire & Resource Conflict");
    cout << "\n  Step 1: Injecting normal baseline readings...\n";
    for (int z = 0; z < 7; z++) {
        sensorData.addReading(z, 25.0f, 5.0f, 60.0f);
        ingestEvent(z, 25.0f, "TEMP",     BASELINE_TEMP[z]);
        ingestEvent(z, 5.0f,  "SMOKE",    BASELINE_SMOKE[z]);
        ingestEvent(z, 60.0f, "HUMIDITY", BASELINE_HUMIDITY[z]);
    }

    cout << "\n  Step 2: Fire ignites in Zone 3 — temperature spikes...\n";
    rollbackStack.push(dynamicGrid, "before-zone3-fire");
    sensorData.addReading(3, 75.0f, 85.0f, 15.0f);
    dynamicGrid.setCell(3/GRID_COLS, 3%GRID_COLS, 75.0f, 85.0f, 15.0f);
    ingestEvent(3, 75.0f, "TEMP",     BASELINE_TEMP[3]);
    ingestEvent(3, 85.0f, "SMOKE",    BASELINE_SMOKE[3]);
    ingestEvent(3, 15.0f, "HUMIDITY", BASELINE_HUMIDITY[3]);

    cout << "\n  Step 3: Fire spreads - Zone 4 readings rise...\n";
    sensorData.addReading(4, 50.0f, 60.0f, 22.0f);
    ingestEvent(4, 50.0f, "TEMP",  BASELINE_TEMP[4]);
    ingestEvent(4, 60.0f, "SMOKE", BASELINE_SMOKE[4]);

    cout << "\n  Step 4: Zone 6 starts showing smoke...\n";
    sensorData.addReading(6, 30.0f, 72.0f, 30.0f);
    ingestEvent(6, 72.0f, "SMOKE", BASELINE_SMOKE[6]);

    cout << "\n  Step 5: Priority queuing emergency tasks...\n";
    Q3_emergency.insert(1, 3, 75.0f, "FireZone3-Critical");
    Q3_emergency.insert(2, 4, 50.0f, "FireZone4-High");
    Q3_emergency.insert(3, 6, 72.0f, "SmokeZone6-Medium");

    cout << "\n  Step 6: Processing emergency response queue...\n";
    Task t;
    while (!Q3_emergency.isEmpty()) Q3_emergency.extractMin(t);

    cout << "\n  Step 7: Viewing anomaly event stream (L3)...\n";
    L3_anomalyStream.traverseForward("L3 Anomaly");

    cout << "\n  Step 8: Demonstrating rollback to pre-fire state...\n";
    rollbackStack.pop(dynamicGrid);

    cout << "\n  *** Scenario 1 Complete ***\n";
    sep();
}

// ─────────────────────────────────────────────────────────────
//  Scenario 2 — Sensor Failure and System Reconstruction
//  Zone 2 sensors send garbage → system reconstructs from
//  historical data and neighbouring zone interpolation.
// ─────────────────────────────────────────────────────────────
void scenario2() {
    printHeader("SCENARIO 2: Sensor Failure & Reconstruction");

    cout << "\n  Step 1: Normal readings stored for all zones...\n";
    for (int z = 0; z < MAX_ZONES; z++) {
        sensorData.addReading(z, 25.0f + z, 5.0f, 60.0f);
        ingestEvent(z, 25.0f + z, "TEMP", BASELINE_TEMP[z]);
        ingestEvent(z, 60.0f,     "HUMIDITY", BASELINE_HUMIDITY[z]);
    }
    rollbackStack.push(dynamicGrid, "pre-sensor-failure");

    cout << "\n  Step 2: Zone 2 sensor starts failing (invalid readings)...\n";
    // Simulate failed sensor — out-of-range or sudden spike
    float badReadings[] = {-5.0f, 999.0f, -1.0f};
    for (int i = 0; i < 3; i++) {
        cout << "  Attempting to store: " << badReadings[i] << " C\n";
        if (isInvalid(badReadings[i], 0.0f, 120.0f)) {
            cout << "  [REJECT] Value " << badReadings[i]
                 << " is outside physical limits.\n";
        }
    }

    cout << "\n  Step 3: Identify last stable state from L2 verified stream...\n";
    EventNode* stable = L2_verifiedStream.lastStable();
    if (stable) {
        cout << "  Last stable event - Zone=" << stable->zone
             << "  Val=" << stable->value
             << "  t="   << stable->timestamp << "\n";
    } else {
        cout << "  No stable event found.\n";
    }

    cout << "\n  Step 4: Reconstruct Zone 2 value via spatial interpolation...\n";
    // Zone 2 maps to row=0, col=2
    float reconstructed = dynamicGrid.interpolateTemp(0, 2);
    cout << "  Reconstructed temp for Zone 2 = " << reconstructed << " C\n";
    sensorData.addReading(2, reconstructed, 5.0f, 60.0f);
    ingestEvent(2, reconstructed, "TEMP", BASELINE_TEMP[2]);

    cout << "\n  Step 5: Forward correction applied to L4 chain...\n";
    L4_forwardCorrect.append(reconstructed, 2, "TEMP", false, true);
    L4_forwardCorrect.traverseForward("L4 After Reconstruction");

    cout << "\n  Step 6: System restored — Zone 2 operational.\n";
    cout << "  *** Scenario 2 Complete ***\n";
    sep();
}

// ============================================================
//  §8   GRAPH LAYER  (G1 – G2)
//       Adjacency List  +  Adjacency Matrix
//       BFS, DFS, path cost, fire-aware cost update
// ============================================================

const int MAX_GRAPH_NODES = 10;   // one node per forest zone

// ── G1: Adjacency List ───────────────────────────────────────
// Each zone stores a small fixed-size neighbour list.
// Space: O(V + E).  Edge lookup: O(degree).
struct AdjNode {
    int  neighbour;
    float weight;       // path cost = distance + danger
    bool  blocked;      // true if fire/smoke blocks this edge
};

const int MAX_NEIGHBOURS = 6;

struct AdjList {
    AdjNode edges[MAX_GRAPH_NODES][MAX_NEIGHBOURS];
    int     degree[MAX_GRAPH_NODES];   // number of neighbours per node
    int     numNodes;

    void init(int nodes) {
        numNodes = nodes;
        for (int i = 0; i < numNodes; i++) {
            degree[i] = 0;
            for (int j = 0; j < MAX_NEIGHBOURS; j++) {
                edges[i][j].neighbour = -1;
                edges[i][j].weight    = 0;
                edges[i][j].blocked   = false;
            }
        }
    }

    // Add undirected edge — O(1)
    void addEdge(int u, int v, float w) {
        if (u < 0 || u >= numNodes || v < 0 || v >= numNodes) return;
        if (degree[u] < MAX_NEIGHBOURS) {
            edges[u][degree[u]] = {v, w, false};
            degree[u]++;
        }
        if (degree[v] < MAX_NEIGHBOURS) {
            edges[v][degree[v]] = {u, w, false};
            degree[v]++;
        }
    }

    // Block/unblock an edge due to fire — O(degree)
    void setBlocked(int u, int v, bool state) {
        for (int i = 0; i < degree[u]; i++)
            if (edges[u][i].neighbour == v) edges[u][i].blocked = state;
        for (int i = 0; i < degree[v]; i++)
            if (edges[v][i].neighbour == u) edges[v][i].blocked = state;
    }

    // Fire-aware cost update: Cost = Distance * (1 + fireLevel)
    // Time complexity: O(degree)
    // Fire-aware cost: Updated Cost = Distance * (1 + fireLevel)  [spec §2.5.3]
    // Auto-block only when fireLevel > 0.95 (extreme fire — fully impassable)
    void updateFireCost(int zone, float fireLevel) {
        for (int i = 0; i < degree[zone]; i++) {
            float base = edges[zone][i].weight;
            edges[zone][i].weight = base * (1.0f + fireLevel);
            // Only block the edge when fire is catastrophic (>0.95)
            // Below that, path remains usable but expensive
            if (fireLevel > 0.95f) edges[zone][i].blocked = true;
        }
        cout << "  [GRAPH] Zone " << zone << " edges updated. fireLevel="
             << fireLevel << (fireLevel > 0.95f ? " (BLOCKED)" : " (HIGH COST)") << "\n";
    }

    void display() const {
        cout << "\n  [G1] Adjacency List:\n";
        for (int i = 0; i < numNodes; i++) {
            cout << "  Zone " << i;
            for (int j = 0; j < degree[i]; j++) {
                cout << "Z" << edges[i][j].neighbour
                     << "(w=" << edges[i][j].weight
                     << (edges[i][j].blocked ? ",BLK" : "") << ")  ";
            }
            cout << "\n";
        }
    }
};

// ── G2: Adjacency Matrix ─────────────────────────────────────
// Full V×V table.  Space: O(V²).  Edge lookup: O(1).
struct AdjMatrix {
    float matrix[MAX_GRAPH_NODES][MAX_GRAPH_NODES];
    int   numNodes;

    void init(int nodes) {
        numNodes = nodes;
        for (int i = 0; i < numNodes; i++)
            for (int j = 0; j < numNodes; j++)
                matrix[i][j] = 0.0f;
    }

    // Add undirected edge — O(1)
    void addEdge(int u, int v, float w) {
        if (u < 0 || u >= numNodes || v < 0 || v >= numNodes) return;
        matrix[u][v] = w;
        matrix[v][u] = w;
    }

    void display() const {
        cout << "\n  [G2] Adjacency Matrix (" << numNodes << "x" << numNodes << "):\n    ";
        for (int i = 0; i < numNodes; i++) cout << "  Z" << i;
        cout << "\n";
        for (int i = 0; i < numNodes; i++) {
            cout << "  Z" << i;
            for (int j = 0; j < numNodes; j++)
                cout << "  " << matrix[i][j];
            cout << "\n";
        }
    }
};

// ── BFS — Breadth First Search ───────────────────────────────
// Used for fire spread prediction (level-by-level expansion).
// Time complexity: O(V + E)
void bfs(const AdjList& g, int start) {
    bool visited[MAX_GRAPH_NODES] = {false};
    int  queue[MAX_GRAPH_NODES];
    int  front = 0, back = 0;

    cout << "\n  [BFS] Starting from Zone " << start << ":\n  ";
    visited[start]  = true;
    queue[back++]   = start;

    // BFS loop — dequeue, visit, enqueue unvisited neighbours
    while (front < back) {
        int cur = queue[front++];
        cout << "Z" << cur << " ";
        for (int i = 0; i < g.degree[cur]; i++) {
            int nb = g.edges[cur][i].neighbour;
            if (!visited[nb] && !g.edges[cur][i].blocked) {
                visited[nb]    = true;
                queue[back++]  = nb;
            }
        }
    }
    cout << "\n  (Blocked edges skipped - fire/smoke barriers)\n";
}

// ── DFS — Depth First Search ─────────────────────────────────
// Used for deep path analysis / full route exploration.
// Time complexity: O(V + E)
void dfsHelper(const AdjList& g, int cur, bool visited[]) {
    visited[cur] = true;
    cout << "Z" << cur << " ";
    for (int i = 0; i < g.degree[cur]; i++) {
        int nb = g.edges[cur][i].neighbour;
        if (!visited[nb] && !g.edges[cur][i].blocked)
            dfsHelper(g, nb, visited);
    }
}

void dfs(const AdjList& g, int start) {
    bool visited[MAX_GRAPH_NODES] = {false};
    cout << "\n  [DFS] Starting from Zone " << start << ":\n  ";
    dfsHelper(g, start, visited);
    cout << "\n";
}

// ── Safe Path — BFS-based shortest path (unweighted hops) ────
// BFS guarantees finding a path if one exists.
// Falls back to high-cost edges if preferred edges are blocked.
// Time complexity: O(V + E)
void safePath(const AdjList& g, int src, int dst) {
    cout << "\n  [SAFE PATH] Z" << src << " Z" << dst << ":\n  ";
    if (src == dst) { cout << "Z" << src << "  (already at destination)\n"; return; }

    // BFS with parent tracking
    int  parent[MAX_GRAPH_NODES];
    bool visited[MAX_GRAPH_NODES];
    int  bfsQ[MAX_GRAPH_NODES];
    for (int i = 0; i < MAX_GRAPH_NODES; i++) {
        parent[i]  = -1;
        visited[i] = false;
    }
    int front = 0, back = 0;
    visited[src]  = true;
    bfsQ[back++]  = src;
    bool found    = false;

    // First pass: prefer unblocked edges
    while (front < back && !found) {
        int cur = bfsQ[front++];
        for (int i = 0; i < g.degree[cur]; i++) {
            int nb = g.edges[cur][i].neighbour;
            if (!visited[nb] && !g.edges[cur][i].blocked) {
                visited[nb]  = true;
                parent[nb]   = cur;
                bfsQ[back++] = nb;
                if (nb == dst) { found = true; break; }
            }
        }
    }

    // Second pass (emergency): allow blocked edges if no clean path found
    if (!found) {
        for (int i = 0; i < MAX_GRAPH_NODES; i++) {
            visited[i] = false; parent[i] = -1;
        }
        front = back = 0;
        visited[src] = true;
        bfsQ[back++] = src;
        while (front < back && !found) {
            int cur = bfsQ[front++];
            for (int i = 0; i < g.degree[cur]; i++) {
                int nb = g.edges[cur][i].neighbour;
                if (!visited[nb]) {
                    visited[nb]       = true;
                    parent[nb]        = cur;
                    bfsQ[back++]      = nb;
                    if (nb == dst) { found = true; break; }
                }
            }
        }
    }

    if (!found) {
        cout << "\n  [NO PATH] Z" << dst
             << " is completely unreachable from Z" << src << "\n";
        return;
    }

    // Reconstruct path from parent array
    int  path[MAX_GRAPH_NODES];
    int  pathLen = 0;
    float totalCost = 0.0f;
    bool emergency  = false;
    int  cur = dst;
    while (cur != -1) { path[pathLen++] = cur; cur = parent[cur]; }

    // Reverse
    for (int i = 0; i < pathLen / 2; i++) {
        int tmp = path[i]; path[i] = path[pathLen-1-i]; path[pathLen-1-i] = tmp;
    }

    // Print and compute cost
    for (int i = 0; i < pathLen; i++) {
        cout << "Z" << path[i];
        if (i < pathLen - 1) {
            // Find edge cost
            int from = path[i], to = path[i+1];
            for (int e = 0; e < g.degree[from]; e++) {
                if (g.edges[from][e].neighbour == to) {
                    totalCost += g.edges[from][e].weight;
                    if (g.edges[from][e].blocked) emergency = true;
                    break;
                }
            }
            cout << " - ";
        }
    }
    cout << "  (cost=" << totalCost;
    if (emergency) cout << ", EMERGENCY ROUTE";
    cout << ")\n";
}

// ── Global graph instances ────────────────────────────────────
AdjList  graphList;    // G1
AdjMatrix graphMatrix; // G2

// Load default forest topology (called once at startup)
void loadDefaultGraph() {
    graphList.init(MAX_GRAPH_NODES);
    graphMatrix.init(MAX_GRAPH_NODES);
    // Forest zone connections: ring + cross links
    // Path cost = distance estimate
    int edges[][3] = {
        {0,1,3},{0,2,4},{1,2,2},{1,3,5},{2,3,3},{2,4,6},
        {3,4,2},{3,5,4},{4,5,3},{4,6,5},{5,6,2},{5,7,4},
        {6,7,3},{6,8,5},{7,8,2},{7,9,4},{8,9,3},{0,5,8}
    };
    int ne = 18;
    for (int i = 0; i < ne; i++) {
        graphList.addEdge(edges[i][0], edges[i][1], (float)edges[i][2]);
        graphMatrix.addEdge(edges[i][0], edges[i][1], (float)edges[i][2]);
    }
}


// ============================================================
//  §9   DECISION TREE LAYER  (T1 – T12)
//       Binary/N-ary trees for hierarchical decision-making
// ============================================================

// ── Generic tree node ────────────────────────────────────────
const int MAX_CHILDREN  = 4;
const int MAX_TREE_NODES = 64;

struct TreeNode {
    int   id;
    char  label[40];
    float score;
    bool  isLeaf;
    bool  triggered;
    int   children[MAX_CHILDREN];
    int   childCount;
};

// ── Fixed-size tree pool ─────────────────────────────────────
struct DecisionTree {
    TreeNode nodes[MAX_TREE_NODES];
    int      count;
    char     name[30];

    void init(const char* treeName) {
        count = 0;
        strcpy(name, treeName);
    }

    // Add a node, return its index — O(1)
    int addNode(const char* label, float score = 0.0f, bool leaf = false) {
        if (count >= MAX_TREE_NODES) return -1;
        int idx = count++;
        nodes[idx].id         = idx;
        nodes[idx].score      = score;
        nodes[idx].isLeaf     = leaf;
        nodes[idx].triggered  = false;
        nodes[idx].childCount = 0;
        strcpy(nodes[idx].label, label);
        for (int i = 0; i < MAX_CHILDREN; i++) nodes[idx].children[i] = -1;
        return idx;
    }

    // Link parent → child — O(1)
    void linkChild(int parent, int child) {
        if (parent < 0 || parent >= count) return;
        TreeNode& p = nodes[parent];
        if (p.childCount < MAX_CHILDREN) {
            p.children[p.childCount++] = child;
        }
    }

    // DFS display — Time complexity: O(V) nodes
    void displayDFS(int idx, int depth) const {
        if (idx < 0 || idx >= count) return;
        for (int i = 0; i < depth * 2; i++) cout << ' ';
        cout  << nodes[idx].id << "] " << nodes[idx].label;
        if (nodes[idx].score > 0) cout << "  score=" << nodes[idx].score;
        if (nodes[idx].triggered)  cout << "  *** TRIGGERED ***";
        cout << "\n";
        for (int c = 0; c < nodes[idx].childCount; c++)
            displayDFS(nodes[idx].children[c], depth + 1);
    }

    void display() const {
        cout << "\n  [TREE: " << name << "]\n";
        if (count > 0) displayDFS(0, 0);
    }

    // Evaluate leaf scores and trigger those above threshold
    // Time complexity: O(V)
    void evaluate(float threshold) {
        for (int i = 0; i < count; i++) {
            if (nodes[i].isLeaf && nodes[i].score >= threshold)
                nodes[i].triggered = true;
        }
    }

    // Find highest triggered leaf — O(V)
    int highestTriggeredLeaf() const {
        int   best  = -1;
        float bScore = -1.0f;
        for (int i = 0; i < count; i++) {
            if (nodes[i].isLeaf && nodes[i].triggered && nodes[i].score > bScore) {
                bScore = nodes[i].score;
                best   = i;
            }
        }
        return best;
    }
};

// ── Named tree instances (T1 – T12) ──────────────────────────
DecisionTree T1_zoneHierarchy;
DecisionTree T2_subZone;
DecisionTree T3_terrain;
DecisionTree T4_water;
DecisionTree T5_fireControl;
DecisionTree T6_equipment;
DecisionTree T7_fireClass;
DecisionTree T8_wildlife;
DecisionTree T9_human;
DecisionTree T10_localDecision;
DecisionTree T11_regional;
DecisionTree T12_global;

// Build all 12 trees once at startup
void buildAllTrees() {
    // T1 — Zone Hierarchy Tree
    T1_zoneHierarchy.init("T1-ZoneHierarchy");
    int root = T1_zoneHierarchy.addNode("Forest");
    for (int z = 0; z < MAX_ZONES; z++) {
        char buf[20]; buf[0]='Z'; buf[1]='o'; buf[2]='n'; buf[3]='e';
        buf[4] = '0' + z; buf[5] = '\0';
        int zNode = T1_zoneHierarchy.addNode(buf);
        T1_zoneHierarchy.linkChild(root, zNode);
        char sub[20]; sub[0]='Z'; sub[1]=(char)('0'+z); sub[2]='-'; sub[3]='S'; sub[4]='u'; sub[5]='b'; sub[6]='\0';
        int sNode = T1_zoneHierarchy.addNode(sub, 0, true);
        T1_zoneHierarchy.linkChild(zNode, sNode);
    }

    // T3 — Terrain Classification (Slope, Dryness, Density)
    T3_terrain.init("T3-TerrainClassify");
    int tRoot  = T3_terrain.addNode("Terrain-Root");
    int highR  = T3_terrain.addNode("HighRisk",  0.8f, true);
    int medR   = T3_terrain.addNode("MedRisk",   0.5f, true);
    int lowR   = T3_terrain.addNode("LowRisk",   0.2f, true);
    T3_terrain.linkChild(tRoot, highR);
    T3_terrain.linkChild(tRoot, medR);
    T3_terrain.linkChild(tRoot, lowR);

    // T7 — Fire Classification
    T7_fireClass.init("T7-FireClassify");
    int fRoot  = T7_fireClass.addNode("FireRoot");
    int major  = T7_fireClass.addNode("MajorFire",  0.9f, true);
    int moderate = T7_fireClass.addNode("ModFire",  0.6f, true);
    int minor  = T7_fireClass.addNode("MinorFire",  0.3f, true);
    T7_fireClass.linkChild(fRoot, major);
    T7_fireClass.linkChild(fRoot, moderate);
    T7_fireClass.linkChild(fRoot, minor);

    // T10 — Local Decision Tree
    T10_localDecision.init("T10-LocalDecision");
    int ldRoot  = T10_localDecision.addNode("LocalCheck");
    int respond = T10_localDecision.addNode("ActivateLocalResponse", 0.8f, true);
    int monitor = T10_localDecision.addNode("ContinueMonitor",       0.3f, true);
    T10_localDecision.linkChild(ldRoot, respond);
    T10_localDecision.linkChild(ldRoot, monitor);

    // T11 — Regional Escalation
    T11_regional.init("T11-RegionalEscalation");
    int rRoot   = T11_regional.addNode("RegionCheck");
    int escalate = T11_regional.addNode("EscalateRegion", 0.75f, true);
    int contain  = T11_regional.addNode("ContainLocal",   0.4f,  true);
    T11_regional.linkChild(rRoot, escalate);
    T11_regional.linkChild(rRoot, contain);

    // T12 — Global Emergency
    T12_global.init("T12-GlobalEmergency");
    int gRoot   = T12_global.addNode("GlobalCheck");
    int globalA = T12_global.addNode("GlobalAlert",       0.9f, true);
    int standby = T12_global.addNode("StandbyMode",       0.5f, true);
    T12_global.linkChild(gRoot, globalA);
    T12_global.linkChild(gRoot, standby);

    // T4 — Water Resources
    T4_water.init("T4-WaterResources");
    int wRoot = T4_water.addNode("WaterCheck");
    int wHigh = T4_water.addNode("Sufficient(>0.8)",  0.9f, true);
    int wMed  = T4_water.addNode("Limited(0.4-0.8)",  0.5f, true);
    int wLow  = T4_water.addNode("Critical(<0.4)",    0.1f, true);
    T4_water.linkChild(wRoot, wHigh);
    T4_water.linkChild(wRoot, wMed);
    T4_water.linkChild(wRoot, wLow);

    // ── T2: Sub-Zone Decomposition Tree ──────────────────────────
    // Each zone splits into 4 directional sub-zones: N, S, E, W
    // Time complexity: tree build O(zones * 4) = O(1) fixed
    T2_subZone.init("T2-SubZone");
    int szRoot = T2_subZone.addNode("ForestSubZones");
    for (int z = 0; z < MAX_ZONES; z++) {
        char zbuf[16];
        zbuf[0]='Z'; zbuf[1]=(char)('0'+z); zbuf[2]='\0';
        int zn = T2_subZone.addNode(zbuf);
        T2_subZone.linkChild(szRoot, zn);
        // 4 directional children per zone
        const char* dirs[] = {"N","S","E","W"};
        for (int d = 0; d < 4; d++) {
            char dbuf[16];
            dbuf[0]='Z'; dbuf[1]=(char)('0'+z); dbuf[2]='-';
            dbuf[3]=dirs[d][0]; dbuf[4]='\0';
            int dn = T2_subZone.addNode(dbuf, 0.0f, true);
            T2_subZone.linkChild(zn, dn);
        }
    }

    // ── T5: Fire Control Resource Tree ───────────────────────────
    // Tracks availability of fire response tools per resource type.
    // Leaf score = availability ratio (available / total units)
    T5_fireControl.init("T5-FireControlResources");
    int fcRoot    = T5_fireControl.addNode("FireControlRoot");
    int fcTrucks  = T5_fireControl.addNode("FireTrucks");
    int fcCrew    = T5_fireControl.addNode("FireCrew");
    int fcAir     = T5_fireControl.addNode("AerialSupport");
    T5_fireControl.linkChild(fcRoot, fcTrucks);
    T5_fireControl.linkChild(fcRoot, fcCrew);
    T5_fireControl.linkChild(fcRoot, fcAir);
    // Truck availability levels
    int fcT_high = T5_fireControl.addNode("Trucks-Available(>6)",  0.9f, true);
    int fcT_med  = T5_fireControl.addNode("Trucks-Limited(3-6)",   0.5f, true);
    int fcT_low  = T5_fireControl.addNode("Trucks-Critical(<3)",   0.1f, true);
    T5_fireControl.linkChild(fcTrucks, fcT_high);
    T5_fireControl.linkChild(fcTrucks, fcT_med);
    T5_fireControl.linkChild(fcTrucks, fcT_low);
    // Crew availability levels
    int fcC_high = T5_fireControl.addNode("Crew-Available(>20)",   0.9f, true);
    int fcC_med  = T5_fireControl.addNode("Crew-Limited(10-20)",   0.5f, true);
    int fcC_low  = T5_fireControl.addNode("Crew-Critical(<10)",    0.1f, true);
    T5_fireControl.linkChild(fcCrew, fcC_high);
    T5_fireControl.linkChild(fcCrew, fcC_med);
    T5_fireControl.linkChild(fcCrew, fcC_low);
    // Aerial support levels
    int fcA_high = T5_fireControl.addNode("Air-Available(>2)",     0.9f, true);
    int fcA_low  = T5_fireControl.addNode("Air-Unavailable",       0.0f, true);
    T5_fireControl.linkChild(fcAir, fcA_high);
    T5_fireControl.linkChild(fcAir, fcA_low);

    // ── T6: Equipment Allocation Tree ────────────────────────────
    // Assigns equipment to zones using: Priority = Risk * Impact
    // Leaves represent allocation decisions per priority band
    T6_equipment.init("T6-EquipmentAlloc");
    int eqRoot   = T6_equipment.addNode("EquipmentRoot");
    int eqHigh   = T6_equipment.addNode("HighPriority(>0.7)");
    int eqMed    = T6_equipment.addNode("MedPriority(0.4-0.7)");
    int eqLow    = T6_equipment.addNode("LowPriority(<0.4)");
    T6_equipment.linkChild(eqRoot, eqHigh);
    T6_equipment.linkChild(eqRoot, eqMed);
    T6_equipment.linkChild(eqRoot, eqLow);
    // High priority allocation actions
    int eqH1 = T6_equipment.addNode("Deploy-AllTrucks+Crew",   0.95f, true);
    int eqH2 = T6_equipment.addNode("RequestAerialSupport",    0.90f, true);
    T6_equipment.linkChild(eqHigh, eqH1);
    T6_equipment.linkChild(eqHigh, eqH2);
    // Medium priority allocation actions
    int eqM1 = T6_equipment.addNode("Deploy-2Trucks+HalfCrew", 0.60f, true);
    int eqM2 = T6_equipment.addNode("StandbyAlert",            0.50f, true);
    T6_equipment.linkChild(eqMed, eqM1);
    T6_equipment.linkChild(eqMed, eqM2);
    // Low priority allocation actions
    int eqL1 = T6_equipment.addNode("Monitor-Only",            0.25f, true);
    int eqL2 = T6_equipment.addNode("ScheduleInspection",      0.20f, true);
    T6_equipment.linkChild(eqLow, eqL1);
    T6_equipment.linkChild(eqLow, eqL2);

    // ── T8: Wildlife Activity Tree ────────────────────────────────
    // Detects animal movement patterns: Normal / Abnormal / Fleeing
    // Fleeing pattern may indicate approaching fire before sensors detect it
    T8_wildlife.init("T8-WildlifeActivity");
    int waRoot    = T8_wildlife.addNode("WildlifeRoot");
    int waLarge   = T8_wildlife.addNode("LargeAnimals");
    int waSmall   = T8_wildlife.addNode("SmallAnimals");
    int waBirds   = T8_wildlife.addNode("Birds");
    T8_wildlife.linkChild(waRoot, waLarge);
    T8_wildlife.linkChild(waRoot, waSmall);
    T8_wildlife.linkChild(waRoot, waBirds);
    // Large animal movement states
    int waL_norm  = T8_wildlife.addNode("Large-NormalGrazing",  0.1f, true);
    int waL_abn   = T8_wildlife.addNode("Large-UnusualMovement",0.5f, true);
    int waL_flee  = T8_wildlife.addNode("Large-Fleeing(ALERT)", 0.9f, true);
    T8_wildlife.linkChild(waLarge, waL_norm);
    T8_wildlife.linkChild(waLarge, waL_abn);
    T8_wildlife.linkChild(waLarge, waL_flee);
    // Small animal movement states
    int waS_norm  = T8_wildlife.addNode("Small-Normal",         0.1f, true);
    int waS_flee  = T8_wildlife.addNode("Small-Fleeing(ALERT)", 0.8f, true);
    T8_wildlife.linkChild(waSmall, waS_norm);
    T8_wildlife.linkChild(waSmall, waS_flee);
    // Bird movement states
    int waB_norm  = T8_wildlife.addNode("Birds-Normal",         0.1f, true);
    int waB_flock = T8_wildlife.addNode("Birds-MassEscape(ALERT)", 0.85f, true);
    T8_wildlife.linkChild(waBirds, waB_norm);
    T8_wildlife.linkChild(waBirds, waB_flock);

    // ── T9: Human Activity Tree ───────────────────────────────────
    // Detects human presence in forest zones
    // Human Risk Score = MovementScore * RestrictedAreaFactor
    T9_human.init("T9-HumanActivity");
    int haRoot    = T9_human.addNode("HumanActivityRoot");
    int haAuth    = T9_human.addNode("AuthorizedPersonnel");
    int haUnauth  = T9_human.addNode("UnauthorizedEntry");
    int haUnknown = T9_human.addNode("UnknownMovement");
    T9_human.linkChild(haRoot, haAuth);
    T9_human.linkChild(haRoot, haUnauth);
    T9_human.linkChild(haRoot, haUnknown);
    // Authorized personnel actions
    int haA_ok    = T9_human.addNode("Auth-RangerPatrol",       0.1f, true);
    int haA_log   = T9_human.addNode("Auth-LoggingCrew",        0.2f, true);
    T9_human.linkChild(haAuth, haA_ok);
    T9_human.linkChild(haAuth, haA_log);
    // Unauthorized entry — Risk = Movement(0.8) * RestrictedFactor(0.9) = 0.72
    int haU_warn  = T9_human.addNode("Unauth-SendWarning",      0.5f, true);
    int haU_alert = T9_human.addNode("Unauth-DispatchRanger",   0.72f,true);
    int haU_evac  = T9_human.addNode("Unauth-ForceEvacuate",    0.9f, true);
    T9_human.linkChild(haUnauth, haU_warn);
    T9_human.linkChild(haUnauth, haU_alert);
    T9_human.linkChild(haUnauth, haU_evac);
    // Unknown movement
    int haK_mon   = T9_human.addNode("Unknown-IncreaseSurveillance", 0.4f, true);
    int haK_inv   = T9_human.addNode("Unknown-Investigate",     0.6f, true);
    T9_human.linkChild(haUnknown, haK_mon);
    T9_human.linkChild(haUnknown, haK_inv);
}

// ── Risk score computation (spec formula) ────────────────────
// Score = w1*Fire + w2*Smoke + w3*Temperature  (normalised 0–1)
// Time complexity: O(1)
float computeRiskScore(int zone) {
    const float w1 = 0.4f, w2 = 0.3f, w3 = 0.3f;
    int i = sensorData.latestIdx(zone);
    if (i < 0) return 0.0f;
    float t  = sensorData.temperature[zone][i];
    float s  = sensorData.smoke[zone][i];
    float fireFlag = (t > TEMP_THRESHOLD || s > SMOKE_THRESHOLD) ? 1.0f : 0.0f;
    return w1 * fireFlag + w2 * (s / 100.0f) + w3 * (t / 100.0f);
}


// ============================================================
//  §10  HASH LAYER  (H1 – H3)
//       Primary index, collision chaining, LRU-style cache
// ============================================================

const int HASH_TABLE_SIZE = 13;   // prime — reduces collisions
const int CACHE_SIZE      = 5;    // recently accessed entries

// ── Hash record ──────────────────────────────────────────────
struct HashRecord {
    int   key;          // zone ID (or composite key)
    float temperature;
    float smoke;
    float humidity;
    bool  occupied;
    char  zoneLabel[10];
};

// ── H1: Primary Index Table (open addressing — linear probing)
// Insert O(1) avg, O(n) worst.  Lookup O(1) avg.
struct HashTable {
    HashRecord table[HASH_TABLE_SIZE];
    int        count;

    void init() {
        count = 0;
        for (int i = 0; i < HASH_TABLE_SIZE; i++)
            table[i].occupied = false;
    }

    // Hash function: index = key mod tableSize — O(1)
    int hashFn(int key) const { return key % HASH_TABLE_SIZE; }

    // Insert with linear probing — O(1) avg
    bool insert(int key, float temp, float smk, float hum) {
        int idx   = hashFn(key);
        int start = idx;
        while (table[idx].occupied && table[idx].key != key) {
            idx = (idx + 1) % HASH_TABLE_SIZE;
            if (idx == start) {
                cout << "  [H1] Table full.\n"; return false;
            }
        }
        table[idx].key         = key;
        table[idx].temperature = temp;
        table[idx].smoke       = smk;
        table[idx].humidity    = hum;
        table[idx].occupied    = true;
        char* lbl = table[idx].zoneLabel;
        lbl[0]='Z'; lbl[1]=(char)('0'+(key/10)); lbl[2]=(char)('0'+(key%10)); lbl[3]='\0';
        if (!table[idx].occupied) count++;
        count++;
        cout << "  [H1] Inserted key=" << key
             << " at index=" << idx
             << "  T=" << temp << "  S=" << smk << "\n";
        return true;
    }

    // Lookup — O(1) avg
    bool lookup(int key, HashRecord& out) const {
        int idx   = hashFn(key);
        int start = idx;
        while (table[idx].occupied) {
            if (table[idx].key == key) { out = table[idx]; return true; }
            idx = (idx + 1) % HASH_TABLE_SIZE;
            if (idx == start) break;
        }
        return false;
    }

    void display() const {
        cout << "\n  [H1] Primary Hash Table (size=" << HASH_TABLE_SIZE << "):\n";
        cout << "  Idx | Key | Temp  | Smoke | Humid | Zone\n";
        cout << "  ----|-----|-------|-------|-------|-----\n";
        for (int i = 0; i < HASH_TABLE_SIZE; i++) {
            cout << "   " << i << "  | ";
            if (table[i].occupied)
                cout << table[i].key << "  |  " << table[i].temperature
                     << "  |  " << table[i].smoke
                     << "  |  " << table[i].humidity
                     << "  | "  << table[i].zoneLabel;
            else
                cout << "---  (empty)";
            cout << "\n";
        }
    }
};

// ── H2: Collision Handling Table (chaining with arrays) ──────
// Each bucket holds a small overflow array.
// Insert O(1), lookup O(chain length) ≈ O(1) avg
const int CHAIN_MAX = 5;

struct ChainBucket {
    HashRecord entries[CHAIN_MAX];
    int        count;
};

struct ChainHashTable {
    ChainBucket buckets[HASH_TABLE_SIZE];

    void init() {
        for (int i = 0; i < HASH_TABLE_SIZE; i++)
            buckets[i].count = 0;
    }

    int hashFn(int key) const { return key % HASH_TABLE_SIZE; }

    // Insert — O(1) avg, collision → same bucket chain
    void insert(int key, float temp, float smk, float hum) {
        int idx = hashFn(key);
        ChainBucket& b = buckets[idx];
        if (b.count >= CHAIN_MAX) { cout << "  [H2] Chain full.\n"; return; }
        b.entries[b.count].key         = key;
        b.entries[b.count].temperature = temp;
        b.entries[b.count].smoke       = smk;
        b.entries[b.count].humidity    = hum;
        b.entries[b.count].occupied    = true;
        b.count++;
        cout << "  [H2] Chained key=" << key
             << " bucket=" << idx
             << " (chain len=" << b.count << ")\n";
    }

    void display() const {
        cout << "\n  [H2] Chaining Collision Table:\n";
        for (int i = 0; i < HASH_TABLE_SIZE; i++) {
            if (buckets[i].count == 0) continue;
            cout << "  Bucket[" << i << "]:";
            for (int j = 0; j < buckets[i].count; j++)
                cout << " [k=" << buckets[i].entries[j].key
                     << " T="   << buckets[i].entries[j].temperature << "]";
            cout << "\n";
        }
    }
};

// ── H3: Fast Retrieval Cache (simple FIFO eviction) ──────────
// Stores last CACHE_SIZE accessed records.
// Insert/lookup O(cache size) — effectively O(1) for small cache
struct RetrievalCache {
    HashRecord entries[CACHE_SIZE];
    int        count;
    int        nextSlot;  // circular eviction pointer

    void init() { count = 0; nextSlot = 0; }

    // Add to cache (overwrite oldest on overflow) — O(1)
    void put(const HashRecord& rec) {
        entries[nextSlot] = rec;
        nextSlot = (nextSlot + 1) % CACHE_SIZE;
        if (count < CACHE_SIZE) count++;
        cout << "  [H3-CACHE] Cached key=" << rec.key << "\n";
    }

    // Check cache before hitting H1 — O(cache size)
    bool get(int key, HashRecord& out) const {
        for (int i = 0; i < count; i++) {
            if (entries[i].key == key) {
                out = entries[i];
                cout << "  [H3-CACHE HIT] key=" << key << "\n";
                return true;
            }
        }
        cout << "  [H3-CACHE MISS] key=" << key << "\n";
        return false;
    }

    void display() const {
        cout << "\n  [H3] Cache (" << count << "/" << CACHE_SIZE << " slots):\n";
        for (int i = 0; i < count; i++)
            cout << "    [" << i << "] key=" << entries[i].key
                 << "  T=" << entries[i].temperature
                 << "  S=" << entries[i].smoke << "\n";
    }
};

// ── Global hash instances ─────────────────────────────────────
HashTable      H1_primary;
ChainHashTable H2_collision;
RetrievalCache H3_cache;


// ============================================================
//  §11  SYSTEM MONITORING LAYER
//       Latency tracking, load analysis, bottleneck detection
// ============================================================

const int MAX_MODULES = 8;

struct ModuleMetrics {
    char  name[20];
    float latencyMs;      // simulated processing time
    int   activeTasks;
    int   capacity;
    bool  bottleneck;
};

struct SystemMonitor {
    ModuleMetrics modules[MAX_MODULES];
    int           count;
    float         totalLatency;
    int           tick;

    void init() {
        count = tick = 0;
        totalLatency = 0.0f;
        const char* names[] = {
            "ArrayLayer","LinkedList","Stack","Queue",
            "DecisionTree","GraphRouter","HashIndex","EventMemory"
        };
        int caps[] = {100, 80, 50, 50, 60, 40, 90, 100};
        for (int i = 0; i < MAX_MODULES; i++) {
            strcpy(modules[i].name, names[i]);
            modules[i].latencyMs   = 0.0f;
            modules[i].activeTasks = 0;
            modules[i].capacity    = caps[i];
            modules[i].bottleneck  = false;
            count++;
        }
    }

    // Simulate recording a processing event for a module — O(1)
    void record(int moduleIdx, float latencyMs, int tasks) {
        if (moduleIdx < 0 || moduleIdx >= count) return;
        modules[moduleIdx].latencyMs   = latencyMs;
        modules[moduleIdx].activeTasks = tasks;
        float load = (float)tasks / modules[moduleIdx].capacity;
        modules[moduleIdx].bottleneck  = (load > 0.75f || latencyMs > 8.0f);
        totalLatency += latencyMs;
        tick++;
    }

    // Detect the slowest module — O(modules)
    int detectBottleneck() const {
        int   worst    = 0;
        float maxLat   = 0.0f;
        for (int i = 0; i < count; i++) {
            if (modules[i].latencyMs > maxLat) {
                maxLat = modules[i].latencyMs;
                worst  = i;
            }
        }
        return worst;
    }

    void displayHealth() const {
        cout << "\n  [SYSTEM HEALTH]  Tick=" << tick
             << "  TotalLatency=" << totalLatency << " ms\n";
        cout << "  Module         | Latency(ms) | Tasks | Load%  | Status\n";
        cout << "  ---------------|-------------|-------|--------|--------\n";
        for (int i = 0; i < count; i++) {
            float load = (modules[i].capacity > 0) ?
                         100.0f * modules[i].activeTasks / modules[i].capacity : 0;
            cout << "  " << modules[i].name;
            int nl = 15 - strlen(modules[i].name);
            for (int j = 0; j < nl; j++) cout << ' ';
            cout << "|  " << modules[i].latencyMs
                 << "         |  "    << modules[i].activeTasks
                 << "    |  "         << (int)load
                 << "%    |  "        << (modules[i].bottleneck ? "OVERLOAD" : "OK")
                 << "\n";
        }
        int bn = detectBottleneck();
        cout << "\n  Bottleneck module: " << modules[bn].name
             << "  (" << modules[bn].latencyMs << " ms)\n";
    }

    // Optimise: reset latency on modules after load redistribution — O(modules)
    void optimize() {
        cout << "\n  [OPTIMIZE] Redistributing load...\n";
        for (int i = 0; i < count; i++) {
            if (modules[i].bottleneck) {
                modules[i].activeTasks = modules[i].capacity / 2;
                modules[i].latencyMs  *= 0.5f;
                modules[i].bottleneck  = false;
                cout << "    [" << modules[i].name << "] tasks halved - "
                     << modules[i].activeTasks << "\n";
            }
        }
        cout << "  Optimization complete.\n";
    }
};

SystemMonitor sysMonitor;


// ============================================================
//  §12  MENU HANDLERS  (Menus 6 – 9)
// ============================================================

// ─────────────────────────────────────────────────────────────
//  MENU 6 — Decision System
// ─────────────────────────────────────────────────────────────
void menu6() {
    int choice;
    do {
        printHeader("6. Decision System");
        cout << "    6.1  Compute Risk Score\n";
        cout << "    6.2  Zone-Level Decision Tree (T10)\n";
        cout << "    6.3  Regional Decision Processing (T11)\n";
        cout << "    6.4  Global Emergency Decision (T12)\n";
        cout << "    6.5  Execute Final Action\n";
        cout << "    6.6  View All Trees\n";
        cout << "    0    Back\n";
        cout << "  Choice: "; cin >> choice;

        if (choice == 1) {
            // ── 6.1  Compute risk score for all zones ─────
            // Time complexity: O(zones) — O(1) per zone
            cout << "\n  [RISK SCORES] w1=0.4(fire) w2=0.3(smoke) w3=0.3(temp)\n";
            cout << "  Zone | Score  | Level\n";
            cout << "  -----|--------|------\n";
            for (int z = 0; z < MAX_ZONES; z++) {
                float sc = computeRiskScore(z);
                const char* level = sc > 0.7f ? "CRITICAL" :
                                    sc > 0.4f ? "HIGH"     :
                                    sc > 0.0f ? "LOW"      : "(no data)";
                cout << "   " << z << "   |  " << sc << "  | " << level << "\n";
            }

        } else if (choice == 2) {
            // ── 6.2  Local zone decision tree ─────────────
            // Rule: if Risk > 0.6 → activate local response
            // Time complexity: O(V) tree evaluation
            int zone;
            cout << "\n  Zone to evaluate: "; cin >> zone;
            float score = computeRiskScore(zone);
            cout << "  Zone " << zone << " risk score = " << score << "\n";
            // Update leaf scores dynamically
            T10_localDecision.nodes[1].score = score;
            T10_localDecision.nodes[2].score = 1.0f - score;
            T10_localDecision.evaluate(0.6f);
            T10_localDecision.display();
            int best = T10_localDecision.highestTriggeredLeaf();
            if (best >= 0)
                cout << "  - Decision: " << T10_localDecision.nodes[best].label << "\n";
            else
                cout << "  - Decision: ContinueMonitor (score below threshold)\n";

        } else if (choice == 3) {
            // ── 6.3  Regional escalation ──────────────────
            // Fire spread rate > 0.5 → escalate
            // Time complexity: O(V)
            float spreadRate = 0.0f;
            for (int z = 0; z < MAX_ZONES; z++) spreadRate += computeRiskScore(z);
            spreadRate /= MAX_ZONES;
            cout << "\n  Average regional spread rate = " << spreadRate << "\n";
            T11_regional.nodes[1].score = spreadRate;
            T11_regional.nodes[2].score = 1.0f - spreadRate;
            T11_regional.evaluate(0.5f);
            T11_regional.display();

        } else if (choice == 4) {
            // ── 6.4  Global emergency decision ────────────
            // Sum of zone risks > threshold → global alert
            // Time complexity: O(zones + V)
            float totalRisk = 0.0f;
            for (int z = 0; z < MAX_ZONES; z++) totalRisk += computeRiskScore(z);
            cout << "\n  Total system risk = " << totalRisk
                 << "  (threshold=3.0)\n";
            if (totalRisk > 3.0f) {
                T12_global.nodes[1].score = 0.9f;
                T12_global.evaluate(0.8f);
                cout << "  *** GLOBAL ALERT ACTIVATED ***\n";
            } else {
                T12_global.nodes[2].score = 0.6f;
                T12_global.evaluate(0.8f);
                cout << "  System in standby mode.\n";
            }
            T12_global.display();

        } else if (choice == 5) {
            // ── 6.5  Execute final action ─────────────────
            // Find highest risk zone, enqueue emergency task
            int   worstZone  = 0;
            float worstScore = 0.0f;
            for (int z = 0; z < MAX_ZONES; z++) {
                float sc = computeRiskScore(z);
                if (sc > worstScore) { worstScore = sc; worstZone = z; }
            }
            cout << "\n  Highest risk: Zone " << worstZone
                 << "  score=" << worstScore << "\n";
            if (worstScore > 0.6f) {
                Q3_emergency.insert(1, worstZone, worstScore, "FinalAction-Emergency");
                cout << "  - Emergency task dispatched to Q3.\n";
                graphList.updateFireCost(worstZone, worstScore);
            } else {
                Q1_routine.enqueue(5, worstZone, worstScore, "FinalAction-Routine");
                cout << "  - Routine monitoring task dispatched to Q1.\n";
            }

        } else if (choice == 6) {
            // ── 6.6  Display all 12 trees by category ─────
            // Time complexity: O(V) per tree, O(12V) total
            int tChoice;
            cout << "\n  Tree category:\n";
            cout << "    1  Structural (T1 Zone Hierarchy, T2 Sub-Zone)\n";
            cout << "    2  Terrain    (T3 Terrain Classification)\n";
            cout << "    3  Resources  (T4 Water, T5 FireControl, T6 Equipment)\n";
            cout << "    4  Incidents  (T7 Fire Class, T8 Wildlife, T9 Human)\n";
            cout << "    5  Decisions  (T10 Local, T11 Regional, T12 Global)\n";
            cout << "    6  All trees\n";
            cout << "  Choice: "; cin >> tChoice;
            if (tChoice == 1 || tChoice == 6) {
                T1_zoneHierarchy.display();
                T2_subZone.display();
            }
            if (tChoice == 2 || tChoice == 6) {
                T3_terrain.display();
            }
            if (tChoice == 3 || tChoice == 6) {
                T4_water.display();
                T5_fireControl.display();
                T6_equipment.display();
            }
            if (tChoice == 4 || tChoice == 6) {
                T7_fireClass.display();
                T8_wildlife.display();
                T9_human.display();
            }
            if (tChoice == 5 || tChoice == 6) {
                T10_localDecision.display();
                T11_regional.display();
                T12_global.display();
            }
        }
    } while (choice != 0);
}

// ─────────────────────────────────────────────────────────────
//  MENU 7 — Spatial Routing System (Graph)
// ─────────────────────────────────────────────────────────────
void menu7() {
    int choice;
    do {
        printHeader("7. Spatial Routing System");
        cout << "    7.1  Load Graph (Adjacency List)\n";
        cout << "    7.2  Load Graph (Adjacency Matrix)\n";
        cout << "    7.3  BFS Traversal (Fire Spread)\n";
        cout << "    7.4  DFS Traversal (Deep Analysis)\n";
        cout << "    7.5  Compute Safe Path\n";
        cout << "    7.6  Update Blocked Routes\n";
        cout << "    0    Back\n";
        cout << "  Choice: "; cin >> choice;

        if (choice == 1) {
            // ── 7.1  Display adjacency list ───────────────
            // Time complexity: O(V + E)
            graphList.display();

        } else if (choice == 2) {
            // ── 7.2  Display adjacency matrix ─────────────
            // Time complexity: O(V²)
            graphMatrix.display();

        } else if (choice == 3) {
            // ── 7.3  BFS — fire spread simulation ─────────
            // Time complexity: O(V + E)
            int start;
            cout << "\n  Start zone: "; cin >> start;
            bfs(graphList, start);
            // Record latency in monitor
            sysMonitor.record(5, 3.5f, 8);

        } else if (choice == 4) {
            // ── 7.4  DFS — deep path analysis ─────────────
            // Time complexity: O(V + E)
            int start;
            cout << "\n  Start zone: "; cin >> start;
            dfs(graphList, start);
            sysMonitor.record(5, 2.8f, 6);

        } else if (choice == 5) {
            // ── 7.5  Safe path ────────────────────────────
            // Greedy lowest-cost path — O(V * degree)
            int src, dst;
            cout << "\n  Source zone: ";      cin >> src;
            cout << "  Destination zone: ";   cin >> dst;
            safePath(graphList, src, dst);

        } else if (choice == 6) {
            // ── 7.6  Update blocked routes ────────────────
            // Fire-aware cost update — O(degree)
            int zone;
            float fireLevel;
            cout << "\n  Zone with fire: "; cin >> zone;
            cout << "  Fire level (0.0-1.0): "; cin >> fireLevel;
            graphList.updateFireCost(zone, fireLevel);
            graphList.display();
        }
    } while (choice != 0);
}

// ─────────────────────────────────────────────────────────────
//  MENU 8 — Hash-Based Fast Access System
// ─────────────────────────────────────────────────────────────
void menu8() {
    int choice;
    do {
        printHeader("8. Hash-Based Fast Access");
        cout << "    8.1  Insert Data (H1 Hash Table)\n";
        cout << "    8.2  Retrieve Data (O(1) access)\n";
        cout << "    8.3  Handle Collisions (H2 Chaining)\n";
        cout << "    8.4  Update Cache (H3)\n";
        cout << "    8.5  View Index Table\n";
        cout << "    0    Back\n";
        cout << "  Choice: "; cin >> choice;

        if (choice == 1) {
            // ── 8.1  Insert into H1 ───────────────────────
            // Time complexity: O(1) avg (linear probe)
            int key; float temp, smoke, humidity;
            cout << "\n  Zone key (0-99): "; cin >> key;
            cout << "  Temperature: ";       cin >> temp;
            cout << "  Smoke: ";             cin >> smoke;
            cout << "  Humidity: ";          cin >> humidity;
            H1_primary.insert(key, temp, smoke, humidity);
            // Also insert into H2 to demonstrate collision handling
            H2_collision.insert(key, temp, smoke, humidity);
            sysMonitor.record(6, 0.5f, H1_primary.count);

        } else if (choice == 2) {
            // ── 8.2  Retrieve with cache ──────────────────
            // Cache check first O(cache), then H1 O(1) avg
            int key;
            cout << "\n  Zone key to retrieve: "; cin >> key;
            HashRecord rec;
            bool found = H3_cache.get(key, rec);
            if (!found) {
                found = H1_primary.lookup(key, rec);
                if (found) {
                    cout << "  [H1] Found: T=" << rec.temperature
                         << "  S=" << rec.smoke
                         << "  H=" << rec.humidity << "\n";
                    H3_cache.put(rec);   // promote to cache
                } else {
                    cout << "  [H1] Key " << key << " not found.\n";
                }
            } else {
                cout << "  [CACHE] T=" << rec.temperature
                     << "  S=" << rec.smoke
                     << "  H=" << rec.humidity << "\n";
            }

        } else if (choice == 3) {
            // ── 8.3  Collision demo ───────────────────────
            // Insert two keys that hash to same bucket
            cout << "\n  [H2] Demonstrating collision handling:\n";
            cout << "  Key=3  and Key=16  both hash to index "
                 << (3 % HASH_TABLE_SIZE) << " (3 mod 13).\n";
            H2_collision.insert(3,  28.0f, 10.0f, 55.0f);
            H2_collision.insert(16, 32.0f, 20.0f, 50.0f);
            H2_collision.display();

        } else if (choice == 4) {
            // ── 8.4  Update cache with current sensor data
            cout << "\n  [H3] Caching latest sensor readings...\n";
            for (int z = 0; z < MAX_ZONES; z++) {
                int i = sensorData.latestIdx(z);
                if (i < 0) continue;
                HashRecord rec;
                rec.key         = z;
                rec.temperature = sensorData.temperature[z][i];
                rec.smoke       = sensorData.smoke[z][i];
                rec.humidity    = sensorData.humidity[z][i];
                rec.occupied    = true;
                H3_cache.put(rec);
            }

        } else if (choice == 5) {
            // ── 8.5  View all hash tables ─────────────────
            H1_primary.display();
            H2_collision.display();
            H3_cache.display();
        }
    } while (choice != 0);
}

// ─────────────────────────────────────────────────────────────
//  MENU 9 — System Monitoring
// ─────────────────────────────────────────────────────────────
void menu9() {
    int choice;
    do {
        printHeader("9. System Monitoring");
        cout << "    9.1  Monitor System Load\n";
        cout << "    9.2  Track Execution Time\n";
        cout << "    9.3  Detect Bottlenecks\n";
        cout << "    9.4  Optimize Performance\n";
        cout << "    9.5  View System Health\n";
        cout << "    0    Back\n";
        cout << "  Choice: "; cin >> choice;

        if (choice == 1) {
            // ── 9.1  Simulate load recording ─────────────
            // Fills monitor with realistic simulated values
            // Time complexity: O(modules) = O(1)
            cout << "\n  [MONITOR] Simulating module loads...\n";
            sysMonitor.record(0, 1.2f, 45);   // Array
            sysMonitor.record(1, 2.1f, 60);   // LinkedList
            sysMonitor.record(2, 0.8f, 20);   // Stack
            sysMonitor.record(3, 3.5f, 40);   // Queue
            sysMonitor.record(4, 4.2f, 35);   // DecisionTree
            sysMonitor.record(5, 9.1f, 38);   // GraphRouter ← overloaded
            sysMonitor.record(6, 0.6f, 25);   // HashIndex
            sysMonitor.record(7, 1.8f, 55);   // EventMemory
            sysMonitor.displayHealth();

        } else if (choice == 2) {
            // ── 9.2  Track execution time of a BFS call ──
            // Latency = FinishTime - StartTime
            cout << "\n  [LATENCY] Running BFS from Zone 0 and measuring time...\n";
            // Simulated: we record before/after counts as proxy for time
            int before = globalTick;
            bfs(graphList, 0);
            int after = globalTick;
            float simLatency = (float)(after - before) * 0.3f + 1.5f;
            cout << "  Simulated BFS latency = " << simLatency << " ms\n";
            sysMonitor.record(5, simLatency, 10);

        } else if (choice == 3) {
            // ── 9.3  Bottleneck detection ─────────────────
            // Time complexity: O(modules)
            sysMonitor.displayHealth();
            int bn = sysMonitor.detectBottleneck();
            cout << "\n  >>> Primary bottleneck: "
                 << sysMonitor.modules[bn].name << " <<<\n";
            cout << "  Recommendation: reduce task load or increase processing capacity.\n";

        } else if (choice == 4) {
            // ── 9.4  Optimize ─────────────────────────────
            // Time complexity: O(modules)
            sysMonitor.optimize();
            sysMonitor.displayHealth();

        } else if (choice == 5) {
            // ── 9.5  Full health view ─────────────────────
            sysMonitor.displayHealth();
            cout << "\n  Q1 tasks pending: " << Q1_routine.count << "\n";
            cout << "  Q2 tasks pending: " << Q2_surveillance.count << "\n";
            cout << "  Q3 tasks pending: " << Q3_emergency.size << "\n";
            cout << "  L1 raw events:    " << L1_rawStream.size << "\n";
            cout << "  L2 verified:      " << L2_verifiedStream.size << "\n";
            cout << "  L3 anomalies:     " << L3_anomalyStream.size << "\n";
            cout << "  Stack depth:      " << rollbackStack.depth() << "\n";
        }
    } while (choice != 0);
}


// ============================================================
//  §13  COMPLETE SCENARIOS 3 – 5
// ============================================================

// ─────────────────────────────────────────────────────────────
//  Scenario 3 — Multi-Factor Anomaly Escalation
//  Wildlife, fire risk, and human movement detected simultaneously.
// ─────────────────────────────────────────────────────────────
void scenario3() {
    printHeader("SCENARIO 3: Multi-Factor Anomaly Escalation");

    cout << "\n  Step 1: Injecting baseline data for all zones...\n";
    for (int z = 0; z < MAX_ZONES; z++) {
        sensorData.addReading(z, 26.0f, 4.0f, 58.0f);
        H1_primary.insert(z, 26.0f, 4.0f, 58.0f);
    }

    cout << "\n  Step 2: Wildlife anomaly - Zone 1 (unusual movement pattern)...\n";
    ingestEvent(1, 48.0f, "TEMP", BASELINE_TEMP[1]);   // heat from herd
    T8_wildlife.nodes[0].score = 0.7f;
    cout << "  Wildlife activity score set to 0.7 in T8.\n";

    cout << "\n  Step 3: Fire risk spike - Zone 5 (temp=55, smoke=80)...\n";
    sensorData.addReading(5, 55.0f, 80.0f, 18.0f);
    ingestEvent(5, 55.0f, "TEMP",     BASELINE_TEMP[5]);
    ingestEvent(5, 80.0f, "SMOKE",    BASELINE_SMOKE[5]);
    ingestEvent(5, 18.0f, "HUMIDITY", BASELINE_HUMIDITY[5]);
    H1_primary.insert(5, 55.0f, 80.0f, 18.0f);

    cout << "\n  Step 4: Human intrusion detected — Zone 8...\n";
    ingestEvent(8, 30.0f, "TEMP", BASELINE_TEMP[8]);
    T9_human.nodes[0].score = 0.65f;
    cout << "  Human risk score set to 0.65 in T9.\n";

    cout << "\n  Step 5: Combined risk evaluation across all zones...\n";
    float totalRisk = 0.0f;
    for (int z = 0; z < MAX_ZONES; z++) totalRisk += computeRiskScore(z);
    cout << "  Total system risk = " << totalRisk << "\n";

    cout << "\n  Step 6: Regional escalation tree evaluation...\n";
    float spread = totalRisk / MAX_ZONES;
    T11_regional.nodes[1].score = spread;
    T11_regional.evaluate(0.4f);
    T11_regional.display();

    cout << "\n  Step 7: High-risk zones enqueued to Q3 and Q4...\n";
    Q3_emergency.insert(1, 5, 0.85f, "MultiAnomaly-Zone5-Fire");
    Q4_multiDecision.enqueue(2, 1, 0.7f, "MultiAnomaly-Zone1-Wildlife");
    Q4_multiDecision.enqueue(2, 8, 0.65f, "MultiAnomaly-Zone8-Human");

    cout << "\n  Step 8: BFS to predict anomaly spread from Zone 5...\n";
    graphList.updateFireCost(5, 0.85f);
    bfs(graphList, 5);

    cout << "\n  *** Scenario 3 Complete ***\n";
    sep();
}

// ─────────────────────────────────────────────────────────────
//  Scenario 4 — System Overload and Load Redistribution
//  Mass sensor updates cause queue saturation.
// ─────────────────────────────────────────────────────────────
void scenario4() {
    printHeader("SCENARIO 4: System Overload & Load Redistribution");

    cout << "\n  Step 1: Flooding Q1 and Q2 with mass sensor updates...\n";
    for (int i = 0; i < 15; i++) {
        int z = i % MAX_ZONES;
        Q1_routine.enqueue(5, z, 25.0f + i, "MassUpdate-Routine");
        Q2_surveillance.enqueue(3, z, 10.0f + i, "MassUpdate-Surv");
        sensorData.addReading(z, 25.0f + i, 5.0f + i, 60.0f - i);
    }

    cout << "\n  Step 2: System load recorded - detecting overload...\n";
    sysMonitor.record(3, 12.5f, 48);   // Queue module overloaded
    sysMonitor.record(5, 10.2f, 39);   // Graph also stressed
    sysMonitor.displayHealth();

    cout << "\n  Step 3: Save stable state snapshot before redistribution...\n";
    rollbackStack.push(dynamicGrid, "pre-overload");

    cout << "\n  Step 4: Pause Q1 routine — prioritise Q3 emergency tasks...\n";
    q1Paused = true;
    Q3_emergency.insert(1, 0, 0.9f, "OverloadPriority-Critical");
    Task t;
    Q3_emergency.extractMin(t);
    cout << "  Critical task processed: " << t.description << "\n";

    cout << "\n  Step 5: Cache frequently accessed zone data in H3...\n";
    for (int z = 0; z < 5; z++) {
        int i = sensorData.latestIdx(z);
        if (i < 0) continue;
        HashRecord rec;
        rec.key = z;
        rec.temperature = sensorData.temperature[z][i];
        rec.smoke       = sensorData.smoke[z][i];
        rec.humidity    = sensorData.humidity[z][i];
        rec.occupied    = true;
        H3_cache.put(rec);
    }

    cout << "\n  Step 6: Optimise module loads...\n";
    sysMonitor.optimize();

    cout << "\n  Step 7: Resume Q1 and process backlog in controlled batches...\n";
    q1Paused = false;
    int processed = 0;
    while (!Q1_routine.isEmpty() && processed < 5) {
        Q1_routine.dequeue(t);
        processed++;
    }
    cout << "  Processed " << processed << " backlog tasks from Q1.\n";

    cout << "\n  Step 8: System health after recovery...\n";
    sysMonitor.displayHealth();

    cout << "\n  *** Scenario 4 Complete ***\n";
    sep();
}

// ─────────────────────────────────────────────────────────────
//  Scenario 5 — Global Multi-Zone Emergency Synchronisation
//  Simultaneous emergencies across all zones with asynchronous
//  updates and conflicting regional states.
// ─────────────────────────────────────────────────────────────
void scenario5() {
    printHeader("SCENARIO 5: Global Multi-Zone Emergency Sync");

    cout << "\n  Step 1: Simultaneous fire events across Zones 2, 5, 7...\n";
    rollbackStack.push(dynamicGrid, "global-pre-emergency");

    float fireZones[][3] = {{2, 68.0f, 90.0f}, {5, 72.0f, 85.0f}, {7, 60.0f, 75.0f}};
    for (int f = 0; f < 3; f++) {
        int   z = (int)fireZones[f][0];
        float t = fireZones[f][1];
        float s = fireZones[f][2];
        sensorData.addReading(z, t, s, 12.0f);
        dynamicGrid.setCell(z/GRID_COLS, z%GRID_COLS, t, s, 12.0f);
        ingestEvent(z, t, "TEMP",  BASELINE_TEMP[z]);
        ingestEvent(z, s, "SMOKE", BASELINE_SMOKE[z]);
        H1_primary.insert(z, t, s, 12.0f);
        Q3_emergency.insert(1, z, t, "GlobalEmergency-Zone");
    }

    cout << "\n  Step 2: Detect regional inconsistencies...\n";
    // Zones 2 and 5 are connected — check if their states are consistent
    float r2 = computeRiskScore(2);
    float r5 = computeRiskScore(5);
    float r7 = computeRiskScore(7);
    cout << "  Zone2 risk=" << r2
         << "  Zone5 risk=" << r5
         << "  Zone7 risk=" << r7 << "\n";
    if (fabs(r2 - r5) > 0.2f)
        cout << "  [INCONSISTENCY] Zone2 and Zone5 diverge — isolating conflict.\n";

    cout << "\n  Step 3: Identify last globally consistent state and rollback...\n";
    rollbackStack.pop(dynamicGrid);

    cout << "\n  Step 4: Reconstruct global state from verified history (L2)...\n";
    L2_verifiedStream.traverseForward("L2 Verified - Global Sync");

    cout << "\n  Step 5: Global emergency decision tree evaluation...\n";
    float totalRisk = r2 + r5 + r7;
    cout << "  Combined risk of emergency zones = " << totalRisk << "\n";
    T12_global.nodes[1].score = (totalRisk > 1.5f) ? 0.95f : 0.4f;
    T12_global.evaluate(0.8f);
    T12_global.display();

    cout << "\n  Step 6: BFS fire spread from all 3 epicentres...\n";
    graphList.updateFireCost(2, r2);
    graphList.updateFireCost(5, r5);
    graphList.updateFireCost(7, r7);
    bfs(graphList, 2);
    bfs(graphList, 5);
    bfs(graphList, 7);

    cout << "\n  Step 7: Compute safe evacuation paths...\n";
    // Reload fresh graph topology for evacuation routing —
    // evacuation paths must be computed on current-scenario costs
    // only, not cumulative damage from prior scenarios.
    loadDefaultGraph();
    graphList.updateFireCost(2, r2);
    graphList.updateFireCost(5, r5);
    graphList.updateFireCost(7, r7);
    // Evacuation routes away from fire zones (2,5,7):
    // Z2 → Z6 (outward, away from Z2 fire)
    // Z5 → Z4 (Z4 directly adjacent to Z5, lower cost route)
    // Z7 → Z9 (outward escape through Z8)
    safePath(graphList, 2, 6);
    safePath(graphList, 5, 4);
    safePath(graphList, 7, 9);

    cout << "\n  Step 8: Process all emergency tasks and dispatch resources...\n";
    Task t;
    while (!Q3_emergency.isEmpty()) Q3_emergency.extractMin(t);

    cout << "\n  Step 9: System-wide health check post-emergency...\n";
    sysMonitor.record(0, 1.5f, 80);
    sysMonitor.record(3, 5.0f, 45);
    sysMonitor.record(5, 8.0f, 38);
    sysMonitor.displayHealth();

    cout << "\n  Step 10: L6 sync chain - verify all zones consistent...\n";
    L6_syncChain.traverseForward("L6 Global Sync Final");

    cout << "\n  *** Scenario 5 Complete ***\n";
    sep();
}


// ============================================================
//  §13b  MENU 4 — Fire Detection and Control  (full body)
//        Placed here so graph, tree, and monitor globals are
//        all in scope. Forward-declared earlier in the file.
// ============================================================

void menu4() {
    int choice;
    do {
        printHeader("4. Fire Detection and Control");
        cout << "    4.1  Detect Fire Risk (Threshold Check)\n";
        cout << "    4.2  Trigger Emergency Alert\n";
        cout << "    4.3  Priority-Based Fire Response\n";
        cout << "    4.4  Simulate Fire Spread (Graph BFS)\n";
        cout << "    4.5  Allocate Firefighting Resources (T4/T5/T6)\n";
        cout << "    0    Back\n";
        cout << "  Choice: "; cin >> choice;

        if (choice == 1) {
            // ── 4.1  Threshold-based fire risk detection ──────────
            // Decision Score = w1*Fire + w2*Smoke + w3*Temp
            // Spec Section 2.4 — Time complexity: O(zones)
            const float w1 = 0.4f, w2 = 0.3f, w3 = 0.3f;
            const float SCORE_THRESHOLD = 0.6f;
            cout << "\n  [FIRE RISK] Decision Score per Zone:\n";
            cout << "  Zone | Temp  | Smoke | Score | Status\n";
            cout << "  -----|-------|-------|-------|--------\n";
            bool any = false;
            for (int z = 0; z < MAX_ZONES; z++) {
                int i = sensorData.latestIdx(z);
                if (i < 0) continue;
                any = true;
                float t  = sensorData.temperature[z][i];
                float s  = sensorData.smoke[z][i];
                float tN = t / 100.0f;
                float sN = s / 100.0f;
                float fireFlag = (t > TEMP_THRESHOLD || s > SMOKE_THRESHOLD)
                                 ? 1.0f : 0.0f;
                float score = w1 * fireFlag + w2 * sN + w3 * tN;
                bool  risk  = score >= SCORE_THRESHOLD;
                cout << "   " << z
                     << "   | " << t << " | " << s
                     << " | "   << score
                     << " | "   << (risk ? "** FIRE RISK **" : "OK") << "\n";
                if (risk) {
                    Q3_emergency.insert(1, z, score, "FireRisk-Auto");
                }
            }
            if (!any) cout << "  (no data yet — add readings via Menu 1 first)\n";

        } else if (choice == 2) {
            // ── 4.2  Trigger emergency alert ──────────────────────
            // O(1) grid update + O(log n) heap insert
            int zone; float temp, smoke;
            cout << "\n  Zone triggering emergency: "; cin >> zone;
            cout << "  Current temperature: ";  cin >> temp;
            cout << "  Current smoke level: ";  cin >> smoke;
            rollbackStack.push(dynamicGrid, "pre-emergency");
            int row = zone / GRID_COLS, col = zone % GRID_COLS;
            dynamicGrid.setCell(row, col, temp, smoke,
                                dynamicGrid.humidity[row][col]);
            Q3_emergency.insert(1, zone, temp, "EmergencyAlert-Manual");
            ingestEvent(zone, temp,  "TEMP",  BASELINE_TEMP[zone]);
            ingestEvent(zone, smoke, "SMOKE", BASELINE_SMOKE[zone]);
            cout << "\n  *** EMERGENCY ALERT TRIGGERED - Zone " << zone << " ***\n";
            cout << "  Grid snapshot saved. Event routed through L1/L3/L9.\n";

        } else if (choice == 3) {
            // ── 4.3  Priority-based fire response ─────────────────
            // Extract all Q3 tasks highest-priority first — O(k log n)
            cout << "\n  [Q3] Processing Emergency Response Queue:\n";
            if (Q3_emergency.isEmpty()) {
                cout << "  No emergency tasks pending.\n";
            } else {
                Task t;
                int processed = 0;
                while (!Q3_emergency.isEmpty()) {
                    Q3_emergency.extractMin(t);
                    cout << "     Zone " << t.zone
                         << "  score=" << t.value
                         << "  desc=" << t.description << "\n";
                    processed++;
                }
                cout << "  " << processed << " tasks processed.\n";
            }

        } else if (choice == 4) {
            // ── 4.4  Fire spread simulation via BFS ───────────────
            // BFS expands level-by-level from fire origin.
            // Blocked edges (fire level > 0.7) are skipped.
            // Time complexity: O(V + E)
            int startZone;
            cout << "\n  Enter fire origin zone (0-"
                 << MAX_GRAPH_NODES - 1 << "): ";
            cin >> startZone;
            // Compute fire level from latest sensor data
            int si = sensorData.latestIdx(startZone);
            if (si >= 0) {
                float t = sensorData.temperature[startZone][si];
                float s = sensorData.smoke[startZone][si];
                float fireLevel = 0.0f;
                if (t > TEMP_THRESHOLD || s > SMOKE_THRESHOLD)
                    fireLevel = 0.4f * (t > TEMP_THRESHOLD ? 1.0f : 0.0f)
                              + 0.3f * (s / 100.0f)
                              + 0.3f * (t / 100.0f);
                cout << "  Computed fire level at Zone " << startZone
                     << " = " << fireLevel << "\n";
                // Update graph edge costs fire-awaredly
                graphList.updateFireCost(startZone, fireLevel);
            }
            cout << "\n  [BFS] Fire spread prediction from Zone "
                 << startZone << ":\n";
            bfs(graphList, startZone);
            cout << "  Each zone above is reachable from the fire origin.\n";
            cout << "  Zones with blocked edges are protected by firebreaks.\n";
            sysMonitor.record(5, 3.5f, 10);

        } else if (choice == 5) {
            // ── 4.5  Allocate firefighting resources ───────────────
            // Uses T4 (water), T5 (fire control), T6 (equipment).
            // Priority = Risk * Impact  (spec §2.4.2 T6)
            // Water availability = Available / Required (spec §2.4.2 T4)
            // Time complexity: O(zones) scan + O(V) tree display
            cout << "\n  [RESOURCE ALLOCATION] Zone-by-zone assignment:\n";
            cout << "  Zone | Risk  | Priority | Water       | Allocation\n";
            cout << "  -----|-------|----------|-------------|---------------------\n";

            const float TOTAL_WATER = 1000.0f;  // litres available
            const float IMPACT      = 0.8f;     // fixed impact factor (spec T6)
            float waterRemain = TOTAL_WATER;

            for (int z = 0; z < MAX_ZONES; z++) {
                float risk = computeRiskScore(z);
                if (risk == 0.0f) continue;

                // T6: Priority = Risk * Impact
                float priority = risk * IMPACT;

                // T4: water needed scales with risk level
                float waterNeeded = risk * 200.0f;
                float waterGiven  = (waterRemain >= waterNeeded)
                                    ? waterNeeded : waterRemain;
                float waterRatio  = (waterNeeded > 0)
                                    ? waterGiven / waterNeeded : 0.0f;
                waterRemain -= waterGiven;

                // T4 leaf status
                const char* wStat = waterRatio >= 0.8f ? "Sufficient" :
                                    waterRatio >= 0.4f ? "Limited"    : "Critical";
                // T6 leaf action
                const char* alloc = priority > 0.7f
                                    ? "AllTrucks+Crew+Air" :
                                    priority > 0.4f
                                    ? "2Trucks+HalfCrew"   : "Monitor-Only";
                // T5 crew status
                const char* crew  = priority > 0.7f ? "FullCrew" :
                                    priority > 0.4f ? "PartialCrew" : "Standby";

                cout << "   " << z
                     << "   | " << risk
                     << " | "   << priority
                     << "    | "  << waterGiven << "L(" << wStat << ")"
                     << " | "   << alloc << " / " << crew << "\n";

                // Dispatch to Q4 multi-decision queue
                if (priority > 0.4f)
                    Q4_multiDecision.enqueue(
                        (int)(priority * 5), z, priority, "ResourceAlloc");
            }

            cout << "\n  Water used: " << (TOTAL_WATER - waterRemain)
                 << "L / " << TOTAL_WATER << "L  remaining=" << waterRemain << "L\n";

            // Show all three resource trees
            cout << "\n"; T4_water.display();
            cout << "\n"; T5_fireControl.display();
            cout << "\n"; T6_equipment.display();

            cout << "\n  Tasks dispatched to Q4: " << Q4_multiDecision.count << "\n";
        }
    } while (choice != 0);
}


// ============================================================
//  §14  MASTER MENU
// ============================================================

void menuScenarios() {
    int choice;
    do {
        printHeader("10. Scenario Simulation");
        cout << "    10.1  Cascading Fire Scenario\n";
        cout << "    10.2  Sensor Failure Scenario\n";
        cout << "    10.3  Multi-Factor Anomaly Scenario\n";
        cout << "    10.4  System Overload Scenario\n";
        cout << "    10.5  Global Emergency Scenario\n";
        cout << "    10.6  Run ALL 5 Scenarios (Full Demo)\n";
        cout << "    0     Back\n";
        cout << "  Choice: "; cin >> choice;
        if      (choice == 1) scenario1();
        else if (choice == 2) scenario2();
        else if (choice == 3) scenario3();
        else if (choice == 4) scenario4();
        else if (choice == 5) scenario5();
        else if (choice == 6) {
            scenario1(); scenario2(); scenario3();
            scenario4(); scenario5();
            cout << "\n  ===== FULL SYSTEM SIMULATION COMPLETE =====\n";
        }
    } while (choice != 0);
}

int main() {
    // ── Initialise all data structures ───────────────────────
    initPool();
    sensorData.init();
    staticGrid.initBaseline();
    dynamicGrid.initBaseline();
    rollbackStack.init();

    L1_rawStream.init();
    L2_verifiedStream.init();
    L3_anomalyStream.init();
    L4_forwardCorrect.init();
    L5_backwardCorrect.init();
    L6_syncChain.init();
    L7_localLoop.init(CIRC_LOOP_SIZE);
    L8_systemLoop.init(CIRC_LOOP_SIZE);
    L9_emergencyLoop.init(CIRC_LOOP_SIZE);
    L10_stabilityLoop.init(CIRC_LOOP_SIZE);

    Q1_routine.init("Q1-Routine");
    Q2_surveillance.init("Q2-Surveillance");
    Q3_emergency.init();
    Q4_multiDecision.init("Q4-MultiDecision");

    H1_primary.init();
    H2_collision.init();
    H3_cache.init();

    sysMonitor.init();
    loadDefaultGraph();
    buildAllTrees();

    cout << "\n  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n";
    cout << "  |   IFAMDS - Forest Advisory & Decision System  |\n";
    cout << "  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n";

    int choice;
    do {
        cout << "\n  ------------ MAIN MENU --------------------\n";
        cout << "    1.  Input Environmental Data\n";
        cout << "    2.  View Forest Grid Status\n";
        cout << "    3.  Event Memory System\n";
        cout << "    4.  Fire Detection and Control\n";
        cout << "    5.  Task Scheduling System\n";
        cout << "    6.  Decision System\n";
        cout << "    7.  Spatial Routing (Graph)\n";
        cout << "    8.  Hash-Based Fast Access\n";
        cout << "    9.  System Monitoring\n";
        cout << "    10. Scenario Simulation\n";
        cout << "    0.  Exit\n";
        cout << "  Choice: "; cin >> choice;

        switch (choice) {
            case 1:  menu1(); break;
            case 2:  menu2(); break;
            case 3:  menu3(); break;
            case 4:  menu4(); break;
            case 5:  menu5(); break;
            case 6:  menu6(); break;
            case 7:  menu7(); break;
            case 8:  menu8(); break;
            case 9:  menu9(); break;
            case 10: menuScenarios(); break;
            case 0:  cout << "\n  Exiting IFAMDS. Goodbye.\n\n"; break;
            default: cout << "\n  Invalid choice.\n";
        }
    } while (choice != 0);

    return 0;
}