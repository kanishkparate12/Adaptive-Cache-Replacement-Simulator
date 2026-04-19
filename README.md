# Adaptive Cache Replacement Simulator

This repository contains a C-based simulation environment for evaluating different **Cache Replacement Policies** and **Cache Architectures**. It allows users to simulate how different cache configurations handle various memory access patterns (traces).

##  Features

- **Set-Associative Cache Simulation**  
  Implements standard set-associative mapping logic.

- **Adaptive/Dynamic Logic**  
  Includes a dynamic cache implementation (`dynamic_cache.c`) designed to adapt to workload characteristics.

- **Workload Analysis**  
  Pre-configured with multiple trace files to test different scenarios:
  - **Temporal Locality**: High reuse of recent data  
  - **Streaming**: Sequential data access with little reuse  
  - **Mixed**: A combination of different access patterns  

##  Repository Structure

| File | Description |
|------|------------|
| `set_assoc_cache.c` | Core logic for a Set-Associative cache |
| `dynamic_cache.c` | Implementation of an adaptive cache replacement strategy |
| `trace.txt` | Standard memory access trace file |
| `temporal_locality_trace.txt` | Trace designed to test temporal reuse |
| `streaming_trace.txt` | Trace designed to test sequential access handling |
| `mixed_trace.txt` | A hybrid workload trace |

##  Getting Started

### Prerequisites

- A C compiler (e.g., `gcc`)
- Terminal / command-line environment

### Compilation

```bash
# Compile the Set-Associative Simulator
gcc set_assoc_cache.c -o set_assoc_sim

# Compile the Dynamic Cache Simulator
gcc dynamic_cache.c -o dynamic_sim
```

### Running the Simulator

Run the executable and provide a trace file as input:

```bash
./set_assoc_sim trace.txt
```

(Modify based on your CLI arguments if needed.)

##  Evaluation Metrics

The simulator outputs key performance metrics such as:

- Total Hits / Misses  
- Miss Rate Percentage  
- Compulsory vs Capacity Misses (if implemented)  

##  Future Improvements

- Add more replacement policies (e.g., LFU, ARC)
- Visualization of cache performance
- Integration with real-world workload traces
- GUI-based simulation dashboard

##  Contributing

Contributions are welcome! Feel free to fork the repo and submit pull requests.

##  License

This project is open-source and available under the MIT License.
