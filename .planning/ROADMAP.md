# Roadmap: Auto-Rating Implementation

## Phase 1: Core AI Rating Module
**Goal**: Create the aesthetic scoring inference module

### Tasks
1. Create `src/common/ai/aesthetic_rating.h` — API header
2. Create `src/common/ai/aesthetic_rating.c` — Implementation
   - Load rating model from registry
   - Preprocess image (resize, normalize)
   - Run ONNX inference
   - Map score to 1-5 star rating
3. Update `src/common/ai/CMakeLists.txt`

### Deliverables
- `dt_ai_rating_init()` — Initialize rating environment
- `dt_ai_rating_available()` — Check if model is ready
- `dt_ai_rating_score_image()` — Score single image, return 1-5 rating
- `dt_ai_rating_cleanup()` — Free resources

## Phase 2: UI Button Module
**Goal**: Add toolbar button for auto-rating

### Tasks
1. Create `src/libs/tools/auto_rating.c` — Lib module
   - Register as lib module in `DT_UI_CONTAINER_PANEL_CENTER_TOP_LEFT`
   - Create star button with tooltip
   - Handle click: get selected images, run scoring, apply ratings
   - Show/hide based on AI enabled state
2. Update `src/libs/tools/CMakeLists.txt`

### Deliverables
- Auto-rate button in lighttable toolbar
- Batch processing via `dt_control_add_job`
- Progress feedback via `dt_control_job_add_progress`

## Phase 3: Model Registry
**Goal**: Register the rating model

### Tasks
1. Add rating model entry to `data/ai_models.json`

### Deliverables
- Model entry with task="rating", default=true

## Phase 4: Integration & Testing
**Goal**: Verify end-to-end flow

### Tasks
1. Build and test compilation
2. Verify button appears when AI enabled + model available
3. Verify button is insensitive when model unavailable
4. Verify ratings are applied correctly
5. Verify undo works

## Phase 5: Commit
**Goal**: Commit with minimal commits, maximum detail

### Commit Plan
Single commit containing all changes:
```
feat: add AI auto-rating for images

Add one-click AI-powered automatic star rating to lighttable.
When the user selects images and clicks the auto-rate button,
an ONNX aesthetic scoring model evaluates each image and
assigns a 1-5 star rating based on the predicted aesthetic score.

Changes:
- src/common/ai/aesthetic_rating.h (new): aesthetic scoring API
- src/common/ai/aesthetic_rating.c (new): ONNX inference for rating
- src/libs/tools/auto_rating.c (new): toolbar button lib module
- data/ai_models.json: add rating model registry entry
- src/common/ai/CMakeLists.txt: add aesthetic_rating.c
- src/libs/tools/CMakeLists.txt: add auto_rating.c

The feature respects the global AI enable toggle and only
activates when a rating model is downloaded and enabled.
Batch scoring runs in the background via the job queue.
```
