# STQL Online Monitoring Algorithm - Comprehensive Analysis

## Overview

This document synthesizes the STQL (Spatio-Temporal Quality Logic) monitoring algorithm from the PerceMon paper and codebase. STQL enables runtime monitoring of perception systems by evaluating formal specifications over streaming perception data (frames containing detected objects).

## 1. Core Monitoring Framework

### 1.1 Streaming Data Model

**Input Stream**: A sequence of frames ξ = (ξ₀, ξ₁, ξ₂, ...) where each frame ξᵢ contains:
- Timestamp: t(ξᵢ) - real-valued time
- Frame number: i - integer frame index
- Objects: Set of detected objects, each with:
  - Object ID: unique identifier
  - Class/Category: string classification
  - Confidence/Probability: detection confidence score
  - Bounding Box: spatial region (coordinates)

**Evaluation Model**:
- Current frame index: i (integer, 0-indexed)
- Current timestamp: t(ξᵢ) (real-valued)
- Evaluation context maps:
  - ε: V_t ∪ V_f → ℕ ∪ {NaN}  (time/frame variables to frame indices)
  - ζ: V_o → ℕ (object variables to actual object IDs)

### 1.2 Online Monitoring Strategy

**Key Principle**: Use buffering to transform future-time formulas into past-time formulas.

1. **Horizon**: Number of future frames that must be buffered before evaluation
   - Future-time operators (Next, Always, Eventually, Until) require looking ahead
   - At frame i, must buffer frames [i, i+horizon] before computing verdict
   
2. **History**: Number of past frames that must be retained
   - Past-time operators (Previous, Since, Holds, Sometimes) require looking back
   - At frame i, must retain frames [i-history, i]

3. **Buffer Management**:
   - Maintain FIFO buffer of size (history + 1 + horizon)
   - Drop oldest frames when buffer exceeds size
   - Delay verdict for future-time formulas by horizon frames

**Online-Monitorable Condition**: A formula is online-monitorable if and only if:
- Horizon is finite and computable
- Can be evaluated on sliding window of frames
- Allows continuous monitoring without unbounded delay

## 2. Boolean Satisfaction Semantics

STQL evaluation returns **boolean truth values** (⊤ = true, ⊥ = false), not robustness degrees.

### 2.1 Evaluation Function Definition

For formula φ, data stream ξ, current frame i, variable mappings (ε, ζ):
```
Quality[φ](ξ, i, ε, ζ) ∈ {⊤, ⊥}
```

### 2.2 Core Evaluation Rules

#### Boolean Constants
```
Quality[⊤](ξ, i, ε, ζ) = ⊤
Quality[⊥](ξ, i, ε, ζ) = ⊥
```

#### Propositional Logic (LTL base)
```
Quality[¬φ](ξ, i, ε, ζ) = ¬Quality[φ](ξ, i, ε, ζ)

Quality[φ₁ ∨ φ₂](ξ, i, ε, ζ) = Quality[φ₁](ξ, i, ε, ζ) ∨ Quality[φ₂](ξ, i, ε, ζ)

Quality[φ₁ ∧ φ₂](ξ, i, ε, ζ) = Quality[φ₁](ξ, i, ε, ζ) ∧ Quality[φ₂](ξ, i, ε, ζ)
```

### 2.3 Temporal Operators (Future-Time)

#### Next: ○φ (look one frame ahead)
**Semantics**: φ holds at the next frame
```
Quality[○φ](ξ, i, ε, ζ) = Quality[φ](ξ, i+1, ε, ζ)
```
**Horizon Impact**: +1 frame
**Evaluation**: Requires buffering frame (i+1)

**Pseudocode**:
```python
def evaluate_next(formula, stream, current_frame_idx, mappings):
    next_idx = current_frame_idx + 1
    if next_idx >= len(stream):
        return DELAYED  # Need future frame
    return evaluate(formula.arg, stream, next_idx, mappings)
```

#### Always (Generalized Always): □φ or □[a,b]φ
**Semantics**: φ holds in all future frames (unbounded) or in interval [a,b]
```
Quality[□φ](ξ, i, ε, ζ) = ∀j ≥ i: Quality[φ](ξ, j, ε, ζ)

Quality[□[a,b]φ](ξ, i, ε, ζ) = ∀i ≤ j ≤ i+b: Quality[φ](ξ, j, ε, ζ)
```
**Horizon Impact**: 
- Unbounded (∞) if no interval specified
- b frames if bounded by interval [a,b]

**Pseudocode**:
```python
def evaluate_always(formula, stream, current_frame_idx, mappings):
    if formula.has_interval():
        a, b = formula.interval
        for j in range(current_frame_idx, current_frame_idx + b + 1):
            if j >= len(stream):
                return DELAYED  # Need more frames
            if not evaluate(formula.arg, stream, j, mappings):
                return False
        return True
    else:
        # Unbounded - cannot evaluate without infinite lookahead
        return UNBOUNDED
```

#### Eventually (Generalized Eventually): ◇φ or ◇[a,b]φ
**Semantics**: φ holds in at least one future frame (unbounded) or in interval [a,b]
```
Quality[◇φ](ξ, i, ε, ζ) = ∃j ≥ i: Quality[φ](ξ, j, ε, ζ)

Quality[◇[a,b]φ](ξ, i, ε, ζ) = ∃i ≤ j ≤ i+b: Quality[φ](ξ, j, ε, ζ)
```
**Horizon Impact**: 
- Unbounded (∞) if no interval
- b frames if bounded

**Pseudocode**:
```python
def evaluate_eventually(formula, stream, current_frame_idx, mappings):
    if formula.has_interval():
        a, b = formula.interval
        for j in range(current_frame_idx, current_frame_idx + b + 1):
            if j >= len(stream):
                return DELAYED  # Need more frames
            if evaluate(formula.arg, stream, j, mappings):
                return True
        return False
    else:
        # Unbounded - cannot evaluate
        return UNBOUNDED
```

#### Until: φ₁ U φ₂
**Semantics**: φ₁ holds until φ₂ becomes true
```
Quality[φ₁ U φ₂](ξ, i, ε, ζ) = ∃j ≥ i: (
    Quality[φ₂](ξ, j, ε, ζ) ∧ 
    ∀i ≤ k ≤ j: Quality[φ₁](ξ, k, ε, ζ)
)
```
**English**: "Eventually φ₂ becomes true, and until then φ₁ holds at all intermediate frames"

**Horizon Impact**: Unbounded (∞) unless bounded by constraint

**Pseudocode - Greedy (Online) Algorithm**:
```python
def evaluate_until(lhs, rhs, stream, current_frame_idx, mappings):
    """
    Online evaluation: commit to first occurrence of rhs
    """
    for j in range(current_frame_idx, len(stream)):
        # Check if rhs is true at frame j
        if evaluate(rhs, stream, j, mappings):
            # Now verify lhs holds in [current_frame_idx, j]
            for k in range(current_frame_idx, j + 1):
                if not evaluate(lhs, stream, k, mappings):
                    continue  # Failed witness, try next rhs
            return True  # Found valid witness
        
        # Check lhs at current position before continuing
        if not evaluate(lhs, stream, j, mappings):
            return False  # lhs violated before rhs
    
    return DELAYED  # Need more frames
```

#### Release: φ₁ R φ₂ (dual of Until)
**Semantics**: φ₂ holds until (and including) φ₁ becomes true
```
Quality[φ₁ R φ₂](ξ, i, ε, ζ) = ∀j ≥ i: (
    Quality[φ₂](ξ, j, ε, ζ) ∨ 
    ∃i ≤ k < j: Quality[φ₁](ξ, k, ε, ζ)
)
```
**English**: "Either φ₁ never happens (and φ₂ always holds), or φ₂ holds until φ₁ happens"

**Horizon Impact**: Unbounded (∞)

### 2.4 Temporal Operators (Past-Time)

#### Previous: ◦φ (look one frame back)
**Semantics**: φ held at the previous frame
```
Quality[◦φ](ξ, i, ε, ζ) = Quality[φ](ξ, i-1, ε, ζ)
```
**History Impact**: +1 frame
**Evaluation**: Requires retaining frame (i-1)

**Pseudocode**:
```python
def evaluate_previous(formula, history_buffer, current_idx, mappings):
    if current_idx == 0:
        return False  # No previous frame
    return evaluate(formula.arg, history_buffer, current_idx - 1, mappings)
```

#### Holds (Historically): ■φ
**Semantics**: φ held at all past frames
```
Quality[■φ](ξ, i, ε, ζ) = ∀j ≤ i: Quality[φ](ξ, j, ε, ζ)
```
**History Impact**: Unbounded (∞)

#### Sometimes (Once): ⧇φ
**Semantics**: φ held at some point in the past
```
Quality[⧇φ](ξ, i, ε, ζ) = ∃j ≤ i: Quality[φ](ξ, j, ε, ζ)
```
**History Impact**: Unbounded (∞)

#### Since: φ₁ S φ₂
**Semantics**: φ₂ held at some point in the past, and φ₁ held continuously since then
```
Quality[φ₁ S φ₂](ξ, i, ε, ζ) = ∃j ≤ i: (
    Quality[φ₂](ξ, j, ε, ζ) ∧ 
    ∀j ≤ k ≤ i: Quality[φ₁](ξ, k, ε, ζ)
)
```
**English**: "Something triggered (φ₂) in the past, and φ₁ has held ever since"

**History Impact**: Unbounded (∞) unless bounded by constraint

**Pseudocode - Incremental Online Algorithm**:
```python
def evaluate_since(lhs, rhs, stream, current_idx, history_buffer):
    """
    Look back to find rhs, verify lhs held continuously since
    """
    for j in range(current_idx, -1, -1):  # Search backwards
        frame_j = history_buffer[j] if j < len(history_buffer) else None
        
        if frame_j and evaluate(rhs, frame_j):
            # Found trigger, verify lhs in [j, current_idx]
            for k in range(j, current_idx + 1):
                if not evaluate(lhs, history_buffer[k]):
                    break
            else:
                return True  # Valid witness
    
    return False
```

#### BackTo: φ₁ B φ₂ (dual of Since)
**Semantics**: φ₁ held back until (and including) φ₂ becomes true in past
```
Quality[φ₁ B φ₂](ξ, i, ε, ζ) = ∀j ≤ i: (
    Quality[φ₁](ξ, j, ε, ζ) ∨ 
    ∃i ≤ k < j: Quality[φ₂](ξ, k, ε, ζ)
)
```
**History Impact**: Unbounded (∞)

## 3. Constraint Evaluation

### 3.1 Time-Based Constraints: (x - C_TIME ∼ t)

**Context**: Used with freeze quantifiers to measure elapsed time
- x ∈ V_t: frozen time variable
- C_TIME: current frame's timestamp
- ∼ ∈ {<, ≤, >, ≥}: comparison operator
- t: time constant (real-valued seconds)

**Evaluation**:
```
Quality[(x - C_TIME) ∼ t](ξ, i, ε, ζ) = 
    ε(x) - t(ξᵢ) ∼ t
```
Where ε(x) is the frozen timestamp.

**Scope Direction Analysis**:
- **Past Scope** (when evaluating inside Past operators like Since):
  - Meaningful form: (x - C_TIME ≤ t) – time in future relative to frozen past
  - Trivial form: (C_TIME - x ≤ t) – always true since C_TIME ≥ x
  
- **Future Scope** (when evaluating inside Future operators like Until):
  - Meaningful form: (C_TIME - x ≤ t) – elapsed time since freeze
  - Trivial form: (x - C_TIME ≤ t) – always true (x always ≤ C_TIME)

**Pseudocode**:
```python
def evaluate_time_bound(time_diff_lhs, time_diff_rhs, current_time, frozen_time, op, bound):
    """
    time_diff_lhs - time_diff_rhs ∼ bound
    """
    if is_c_time(time_diff_lhs) and not is_c_time(time_diff_rhs):
        # Form: C_TIME - frozen_var
        lhs_val = current_time
        rhs_val = frozen_time
    elif not is_c_time(time_diff_lhs) and is_c_time(time_diff_rhs):
        # Form: frozen_var - C_TIME
        lhs_val = frozen_time
        rhs_val = current_time
    else:
        return False  # Invalid form
    
    return compare(lhs_val - rhs_val, op, bound)
```

### 3.2 Frame-Based Constraints: (f - C_FRAME ∼ n)

**Context**: Discrete version of time constraints
- f ∈ V_f: frozen frame variable
- C_FRAME: current frame number
- n: frame count (integer)

**Evaluation**:
```
Quality[(f - C_FRAME) ∼ n](ξ, i, ε, ζ) = 
    ε(f) - i ∼ n
```

**Semantics**: Same as time-based constraints but in frame domain.

### 3.3 Quantitative Predicates

#### Class Comparison: Class(id) = c
**Evaluation**:
```
Quality[Class(id) = c](ξ, i, ε, ζ) = 
    Objects(ξᵢ)[ζ(id)].class = c
```

#### Probability Comparison: P(id) ∼ r
**Evaluation**:
```
Quality[P(id) ∼ r](ξ, i, ε, ζ) = 
    Objects(ξᵢ)[ζ(id)].confidence ∼ r
```

#### Euclidean Distance: ED(id₁, CRT₁, id₂, CRT₂) ∼ r
**Evaluation**:
```
Quality[ED(id₁, CRT₁, id₂, CRT₂) ∼ r](ξ, i, ε, ζ) =
    distance(
        bbox_point(Objects(ξᵢ)[ζ(id₁)], CRT₁),
        bbox_point(Objects(ξᵢ)[ζ(id₂)], CRT₂)
    ) ∼ r
```

## 4. Quantifiers

### 4.1 Existential Object Quantifier: ∃{id₁, ...}@φ

**Semantics**: "There exist objects in current frame such that φ holds"
```
Quality[∃{id₁, ..., idₙ}@φ](ξ, i, ε, ζ) = 
    ⋁_{k₁, ..., kₙ ∈ Objects(ξᵢ)} Quality[φ](ξ, i, ε, ζ[id₁ ← k₁, ...])
```

**Evaluation Strategy**:
```python
def evaluate_exists(object_vars, body, frame, mappings):
    """
    Try to find object assignments that satisfy body
    """
    for obj_id in frame.object_ids:
        new_mappings = mappings.copy()
        for var in object_vars:
            new_mappings[var] = obj_id
        
        if evaluate(body, new_mappings):
            return True
    
    return False
```

### 4.2 Universal Object Quantifier: ∀{id₁, ...}@φ

**Semantics**: "All objects in current frame satisfy φ"
```
Quality[∀{id₁, ..., idₙ}@φ](ξ, i, ε, ζ) = 
    ⋀_{k₁, ..., kₙ ∈ Objects(ξᵢ)} Quality[φ](ξ, i, ε, ζ[id₁ ← k₁, ...])
```

### 4.3 Freeze Quantifier: {x, f}.φ

**Semantics**: "Pin current time/frame for measuring durations in φ"
```
Quality[{x, f}.φ](ξ, i, ε, ζ) = 
    Quality[φ](ξ, i, ε[x ← t(ξᵢ), f ← i], ζ)
```

**Effect**: Allows later constraints like (x - C_TIME ≤ t) to measure elapsed time since freeze point.

**Pseudocode**:
```python
def evaluate_freeze(time_var, frame_var, body, stream, current_idx, mappings):
    """
    Capture current time/frame for later comparison
    """
    new_mappings = mappings.copy()
    if time_var:
        new_mappings[time_var] = stream[current_idx].timestamp
    if frame_var:
        new_mappings[frame_var] = current_idx
    
    return evaluate(body, stream, current_idx, new_mappings)
```

## 5. Spatial Operators

### 5.1 Bounding Box Operations

**Core Operations on Spatial Expressions (Ω)**:

```
Ω ∈ {
    ∅              (empty set),
    U              (universe - entire image),
    BB(id)         (bounding box of object),
    ¬Ω             (complement),
    Ω₁ ⊔ Ω₂        (union),
    Ω₁ ⊓ Ω₂        (intersection)
}
```

**Spatial Evaluation Function**:
```
SpEval(∅, ξ, i, ζ) = ∅

SpEval(U, ξ, i, ζ) = Universe

SpEval(BB(id), ξ, i, ζ) = Objects(ξᵢ)[ζ(id)].bbox

SpEval(¬Ω, ξ, i, ζ) = U \ SpEval(Ω, ξ, i, ζ)

SpEval(Ω₁ ⊔ Ω₂, ξ, i, ζ) = SpEval(Ω₁, ξ, i, ζ) ∪ SpEval(Ω₂, ξ, i, ζ)

SpEval(Ω₁ ⊓ Ω₂, ξ, i, ζ) = SpEval(Ω₁, ξ, i, ζ) ∩ SpEval(Ω₂, ξ, i, ζ)
```

### 5.2 Spatial Existence: ∃Ω

**Semantics**: "The region Ω is non-empty"
```
Quality[∃Ω](ξ, i, ε, ζ) = 
    SpEval(Ω, ξ, i, ζ) ≠ ∅
```

### 5.3 Area Predicate: Area(Ω) ∼ r

**Semantics**: "The area of region Ω satisfies comparison with r"
```
Quality[Area(Ω) ∼ r](ξ, i, ε, ζ) = 
    compute_area(SpEval(Ω, ξ, i, ζ)) ∼ r
```

### 5.4 Position Predicates

#### Lateral Position: Lat(id, CRT) ∼ r
```
Quality[Lat(id, CRT) ∼ r](ξ, i, ε, ζ) = 
    lateral_distance(bbox_point(Objects(ξᵢ)[ζ(id)], CRT)) ∼ r
```

#### Longitudinal Position: Lon(id, CRT) ∼ r
```
Quality[Lon(id, CRT) ∼ r](ξ, i, ε, ζ) = 
    longitudinal_distance(bbox_point(Objects(ξᵢ)[ζ(id)], CRT)) ∼ r
```

## 6. Memory Requirements Computation

### 6.1 Horizon Computation (Future-Time Requirements)

**Purpose**: Determine how many future frames must be buffered before evaluating at frame i.

**Algorithm**: Recursive tree traversal tracking scope direction (Future vs Past).

#### Future-Time Operators

| Operator | Horizon | Meaning |
|----------|---------|---------|
| next(φ, n) | 1 + Horizon(φ) | n frames ahead |
| eventually(φ) | ∞ | unbounded future |
| always(φ) | ∞ | unbounded future |
| until(φ₁, φ₂) | ∞ | unbounded future |
| release(φ₁, φ₂) | ∞ | unbounded future |

#### Constraint Extraction (Future Scope)

When evaluating inside future-time operators, only constraints of form **(C_TIME - x ≤ t)** are meaningful:
```
Quality[{x}.(eventually((C_TIME - x) ≤ t ∧ φ))](ξ, i, ε, ζ)
```
This means: "Eventually within t seconds (from when x was frozen)"
- Horizon bound: ceil(t * fps) frames

#### Pseudocode - Horizon Computation

```python
def compute_horizon(formula, fps=1.0):
    """
    Returns UNBOUNDED or number of frames needed
    """
    if is_const(formula) or is_comparison(formula):
        return 0  # No buffering needed
    
    elif is_next(formula):
        # next(φ, n) requires n + horizon(φ) frames
        inner_horiz = compute_horizon(formula.arg, fps)
        if inner_horiz == UNBOUNDED:
            return UNBOUNDED
        return formula.steps + inner_horiz
    
    elif is_always(formula) or is_eventually(formula) or is_until(formula):
        # Check if subexpression has constraints
        inner_horiz = compute_horizon(formula.arg, fps)
        if inner_horiz == 0:  # No constraint
            return UNBOUNDED
        return inner_horiz
    
    elif is_time_bound(formula):
        # Extract meaningful constraint (C_TIME - x ≤ t)
        if is_meaningful_future_constraint(formula):
            return ceil(formula.value * fps)
        return 0  # Trivial constraint
    
    elif is_and(formula) or is_or(formula):
        # Take max of children
        return max(compute_horizon(child, fps) for child in formula.children)
    
    else:
        # Other operators inherit from children
        return compute_horizon(formula.arg, fps)
```

### 6.2 History Computation (Past-Time Requirements)

**Purpose**: Determine how many past frames must be retained for evaluation.

| Operator | History | Meaning |
|----------|---------|---------|
| previous(φ, n) | 1 + History(φ) | n frames back |
| holds(φ) | ∞ | unbounded past |
| sometimes(φ) | ∞ | unbounded past |
| since(φ₁, φ₂) | ∞ | unbounded past |
| backto(φ₁, φ₂) | ∞ | unbounded past |

#### Constraint Extraction (Past Scope)

When evaluating inside past-time operators, only constraints of form **(x - C_TIME ≤ t)** are meaningful:
```
Quality[{x}.(since((x - C_TIME) ≤ t, φ))](ξ, i, ε, ζ)
```
This means: "Since t seconds ago (from current time)"
- History bound: ceil(t * fps) frames

### 6.3 Combined Requirements

**Algorithm**: Compute horizon and history together, tracking scope direction.

```python
def compute_requirements(formula, fps=1.0):
    """
    Returns (history, horizon) pair
    """
    def compute_recursive(expr, scope):
        if scope == FUTURE_SCOPE:
            # Looking for future-time constraints
            # Meaningful: (C_TIME - var ≤ bound)
            pass
        else:  # PAST_SCOPE
            # Looking for past-time constraints
            # Meaningful: (var - C_TIME ≤ bound)
            pass
        
        return (history, horizon)
    
    history, horizon = compute_recursive(formula, FUTURE_SCOPE)
    return MonitoringRequirements(history, horizon)
```

#### AND/OR Composition

| Operator | History | Horizon | Reason |
|----------|---------|---------|--------|
| AND | max | max | Worst-case: need all buffered frames |
| OR | max | max | Worst-case: either branch might execute |

**Key Insight**: Both AND and OR take the maximum because at runtime we don't know which branch succeeds, so we must buffer for either possibility.

## 7. Online Monitoring State Machine

### 7.1 Monitor State

```python
class STQLMonitor:
    def __init__(self, formula, fps=30.0):
        self.formula = formula
        self.fps = fps
        self.requirements = compute_requirements(formula, fps)
        
        # Circular buffers
        self.history_buffer = CircularBuffer(self.requirements.history.frames)
        self.horizon_buffer = CircularBuffer(self.requirements.horizon.frames)
        
        # Tracking
        self.frame_number = 0
        self.pending_verdicts = deque()
    
    def on_new_frame(self, frame):
        """
        Called when new frame arrives
        """
        # Add to horizon buffer
        self.horizon_buffer.push(frame)
        
        # Check if we can emit verdict for delayed frame
        if len(self.horizon_buffer) > self.requirements.horizon.frames:
            # Enough frames buffered - evaluate and emit
            verdict = evaluate(self.formula, self.history_buffer, 
                             self.frame_number - self.requirements.horizon.frames)
            self.pending_verdicts.append((self.frame_number - self.requirements.horizon.frames, verdict))
        
        # Move old frame from horizon to history
        if len(self.horizon_buffer) > self.requirements.horizon.frames:
            old_frame = self.horizon_buffer.pop_front()
            self.history_buffer.push(old_frame)
        
        # Drop excess history
        if len(self.history_buffer) > self.requirements.history.frames:
            self.history_buffer.pop_front()
        
        self.frame_number += 1
    
    def get_verdicts(self):
        """
        Get available verdicts (may be delayed)
        """
        return self.pending_verdicts
```

### 7.2 Frame Processing Timeline

```
Frame:    0    1    2    3    4    5    6    7    8   ...
          |    |    |    |    |    |    |    |    |
          
Received: ●    ●    ●    ●    ●    ●    ●    ●    ●

Buffer:   |0   |0,1  |0,1,2|0,1,2,3|1,2,3,4|2,3,4,5|...
(assuming horizon=3, history=2)

Verdict:                                ✓(frame 0)
                                              ✓(frame 1)
                                                    ✓(frame 2)
```

**Delay = Horizon**: Verdict for frame i becomes available at frame (i + horizon).

## 8. Complex Formula Examples

### Example 1: "Car stays in lane for 5 seconds"
```
{x}.(always(
    exists({car}, 
        (class(car) = "vehicle") ∧ 
        (car.bbox ⊓ lane ≠ ∅)
    ) ∨ (C_TIME - x ≤ 5)
))
```
**Analysis**:
- Horizon: 5 seconds * fps = 5 * 30 = 150 frames (if fps=30)
- History: 0 (no past operators)
- Online-monitorable: YES

### Example 2: "Traffic light changed from red to green within 10 frames"
```
{f}.(until(
    not(exists({light}, class(light) = "green")),
    exists({light}, class(light) = "green") ∧ (f - C_FRAME ≤ 10)
))
```
**Analysis**:
- Horizon: 10 frames (from frame constraint)
- History: 0
- Online-monitorable: YES (bounded by frame constraint)

### Example 3: "Object was always in region throughout history" (unbounded)
```
always(
    exists({obj}, bbox(obj) ⊓ region ≠ ∅)
)
```
**Analysis**:
- Horizon: ∞ (unbounded always)
- History: 0
- Online-monitorable: NO (not online-monitorable)

## 9. Implementation Patterns

### 9.1 C++ Visitor Pattern for Evaluation

```cpp
auto evaluate = [](const auto& expr, ...) -> bool {
    return std::visit([&](const auto& e) -> bool {
        using T = std::decay_t<decltype(e)>;
        
        if constexpr (std::is_same_v<T, NextExpr>) {
            return evaluate_next(e.arg, ...);
        } else if constexpr (std::is_same_v<T, UntilExpr>) {
            return evaluate_until(e.lhs, e.rhs, ...);
        } else if constexpr (std::is_same_v<T, TimeBoundExpr>) {
            return evaluate_time_bound(e.diff.lhs, e.diff.rhs, ...);
        }
        // ... other cases
    }, expr);
};
```

### 9.2 Recursive Tree Traversal for Requirements

```cpp
auto compute_requirements_impl(const Expr& expr, double fps, ScopeDirection scope)
    -> std::pair<int64_t, int64_t> {
    return std::visit([&](const auto& e) -> std::pair<int64_t, int64_t> {
        using T = std::decay_t<decltype(e)>;
        
        if constexpr (std::is_same_v<T, NextExpr>) {
            auto [hist, horiz] = compute_requirements_impl(*e.arg, fps, ScopeDirection::Future);
            return {hist, horiz + e.steps};
        } else if constexpr (std::is_same_v<T, TimeBoundExpr>) {
            if (auto bound = extract_meaningful_constraint(e, scope)) {
                return {bound->is_past ? bound->frames : 0,
                        bound->is_future ? bound->frames : 0};
            }
            return {0, 0};
        }
        // ...
    }, expr);
}
```

## 10. Key Algorithms Summary

### Algorithm 1: Boolean Satisfaction Evaluation

```
Input: STQL formula φ, frame stream ξ, current index i, variable mappings (ε, ζ)
Output: Boolean value (⊤ or ⊥)

1. If φ is constant:
     Return ⊤ or ⊥ accordingly
   
2. If φ is boolean operation:
     Recursively evaluate children
     Apply boolean connectives (∨, ∧, ¬)
   
3. If φ is temporal operator:
     For future operators: evaluate at next/future frames
     For past operators: evaluate at previous/past frames
     For Until/Since: search for witness point
   
4. If φ is constraint:
     Evaluate arithmetic comparison
     Check if meaningful in current scope
   
5. If φ is quantifier:
     For ∃: try all object assignments, return ⊤ if any succeeds
     For ∀: check all object assignments, return ⊤ if all succeed
     For freeze: capture current time/frame, proceed to body
   
6. If φ is spatial operator:
     Compute spatial expression evaluation
     Apply set operations
     Check existence or area constraints
```

### Algorithm 2: Memory Requirements Computation

```
Input: STQL formula φ, FPS for time conversion
Output: (history_frames, horizon_frames)

1. Traverse AST with std::visit
2. Track scope direction (Future = inside future operators, Past = inside past)
3. For each operator node:
   
   Future-time operators:
     - next(φ, n): horizon += n
     - always/eventually/until: horizon = ∞ unless constrained
     - Constraints: extract if form is (C_TIME - var ≤ bound)
   
   Past-time operators:
     - previous(φ, n): history += n
     - holds/sometimes/since: history = ∞ unless constrained
     - Constraints: extract if form is (var - C_TIME ≤ bound)
   
   Logical operators:
     - AND/OR: take max of all children
   
   Quantifiers/Freeze:
     - Inherit from body

4. Return {history, horizon}
```

### Algorithm 3: Online Monitoring Loop

```
Input: STQL formula φ, stream of frames
Output: Stream of (frame_index, verdict) pairs (delayed by horizon frames)

1. Initialize:
   reqs ← compute_requirements(φ)
   history_buffer ← empty circular buffer of size reqs.history
   horizon_buffer ← empty circular buffer of size reqs.horizon
   pending_verdicts ← empty queue
   frame_idx ← 0

2. While new frame arrives:
   
   a. Push frame to horizon_buffer
   
   b. If len(horizon_buffer) > reqs.horizon:
      # Enough future frames buffered
      delayed_idx ← frame_idx - reqs.horizon
      verdict ← evaluate(φ, history_buffer, delayed_idx)
      pending_verdicts.append((delayed_idx, verdict))
      
      # Move frame from horizon to history
      old_frame ← horizon_buffer.pop_front()
      history_buffer.push(old_frame)
      
      # Maintain history size
      if len(history_buffer) > reqs.history:
         history_buffer.pop_front()
   
   c. frame_idx ← frame_idx + 1
   
   d. Yield pending_verdicts

3. At end of stream (optional):
   # Emit remaining delayed verdicts
   For remaining frames in horizon_buffer:
      Similar evaluation as step 2b
```

## 11. Key Insights for Implementation

1. **Scope Direction is Critical**: Constraint meaningfulness depends on whether we're in future or past temporal scope. (C_TIME - x) is meaningful in future; (x - C_TIME) is meaningful in past.

2. **Unbounded Operators Without Constraints**: Operators like always() and until() require infinite horizon unless constrained by specific bounds like (C_TIME - x ≤ 5).

3. **Incremental State Management**: Online monitoring maintains two buffers:
   - History buffer (past frames) - retained indefinitely or until history limit exceeded
   - Horizon buffer (future frames) - delayed emission of verdicts

4. **Boolean, Not Robustness**: STQL evaluation is strictly boolean, not real-valued robustness like MTL. This simplifies computation.

5. **Variable Capture via Freeze**: The {x, f}.φ operator enables time/frame measurement by pinning current values for later comparison.

6. **Quantifier Handling**: Existential and universal quantifiers iterate over available objects in the current frame. No unbounded search.

