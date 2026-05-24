# Requirements: Auto-Rating Feature

## Functional Requirements

### FR-1: AI-Powered Auto-Rating
- **FR-1.1**: The system shall provide a one-click button to automatically rate selected images using an AI aesthetic model.
- **FR-1.2**: The rating shall be applied as standard darktable 1-5 star ratings.
- **FR-1.3**: The AI score (float [0,1]) shall be mapped to stars: [0,0.2)=1, [0.2,0.4)=2, [0.4,0.6)=3, [0.6,0.8)=4, [0.8,1]=5.
- **FR-1.4**: The operation shall support batch processing of multiple selected images.

### FR-2: AI Model Integration
- **FR-2.1**: The feature shall use darktable's existing AI model registry system.
- **FR-2.2**: The model task type shall be `"rating"`.
- **FR-2.3**: The model shall be downloadable/installable via the existing AI models UI.
- **FR-2.4**: If no rating model is available (not downloaded or not enabled), the button shall be insensitive.

### FR-3: UI Integration
- **FR-3.1**: The auto-rate button shall appear in the lighttable top toolbar.
- **FR-3.2**: The button shall use a star icon for visual consistency.
- **FR-3.3**: The button shall show a tooltip explaining the function.
- **FR-3.4**: The button shall be hidden or insensitive when AI features are disabled.
- **FR-3.5**: Processing shall show a progress indicator in the status bar.

### FR-4: Settings Integration
- **FR-4.1**: The feature shall respect the global "enable AI features" toggle.
- **FR-4.2**: The rating model shall appear in the AI preferences model list (automatic via registry).

## Non-Functional Requirements

### NFR-1: Performance
- **NFR-1.1**: Single image scoring shall complete within 2 seconds on typical hardware.
- **NFR-1.2**: Batch scoring shall run in background without blocking the UI.

### NFR-2: Maintainability
- **NFR-2.1**: New code shall be isolated in dedicated files to minimize merge conflicts.
- **NFR-2.2**: Code shall follow darktable's existing patterns for AI modules and lib modules.

### NFR-3: Robustness
- **NFR-3.1**: If inference fails for an image, skip it and continue with others.
- **NFR-3.2**: If the model is not available, gracefully degrade (button insensitive).
- **NFR-3.3**: Support undo for auto-rating operations.
