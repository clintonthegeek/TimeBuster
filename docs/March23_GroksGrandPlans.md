# Roadmap for TimeBuster Development

A while ago, you helped me expand my list of future directions for TimeBuster, and I’m thrilled with the clarity we’ve achieved. I agree that starting with `ViewInterface` and its variants—`CalendarTableView`, `EditPane`, and `ItemEditorView`—is the critical first step in tackling our extensive list of refactorings, improvements, and additions. Below, I’ll recap each direction briefly, deliberate on their ordering, and then lay out a multi-stage roadmap with discrete, achievable, and compilable milestones that culminate in realizing all our goals.

---

## Recap of Future Directions and Ordering Rationale

Here’s a quick recap of each direction, with thoughts on their ordering based on dependencies, complexity, and user value:

1. **Redistribute Logic from `CollectionController` and `MainWindow`**  
   - *Recap*: Slim down `CollectionController` to backend orchestration, move calendar ops to `Cal` or a new `CalendarManager`, and make `MainWindow` a UI host.  
   - *Order*: Early—core architecture cleanup enables everything else.

2. **Unified `ViewInterface`**  
   - *Recap*: Abstract base class `ViewInterface` for `CalendarTableView` (proactive), `EditPane` (passive), and `ItemEditorView` (proactive), unifying signals like `calChanged` and `itemModified`.  
   - *Order*: First—foundation for all view-related features.

3. **Multi-Select and Multi-Edit**  
   - *Recap*: Enable `QList<QSharedPointer<CalendarItem>>` selection in `CalendarTableView`, batch edits via `ItemEditorView`.  
   - *Order*: Follows `ViewInterface`—builds on selection signals.

4. **Clipboard Operations (Copy, Cut, Paste)**  
   - *Recap*: Add `QClipboard` support in `CalendarTableView`, paste via `SessionManager::queueDeltaChange`.  
   - *Order*: After multi-select—extends item handling.

5. **Stage Pane for Uncommitted Changes**  
   - *Recap*: Passive `StagePane` showing `SessionManager::m_deltaChanges`, with revert/discard options.  
   - *Order*: After `ViewInterface`—leverages deltas, user-facing priority.

6. **Add/Delete `CalendarItem` and `Cal`**  
   - *Recap*: UI and logic for `Cal::addItem`, `Cal::removeItem`, `Collection::addCalendar`, and deletion.  
   - *Order*: Mid-tier—requires view stability, extends editing.

7. **Edit Times, Dates, and Progress**  
   - *Recap*: Expand `EditPane` and `ItemEditorView` for `CalendarItem::incidence()` fields like `dtStart` and `percentComplete`.  
   - *Order*: Follows add/delete—completes editing suite.

8. **Todos and Nested/Tree Structures**  
   - *Recap*: Add `parentId` to `CalendarItem`, make `Cal` hierarchical for `QTreeView` in `CalendarTableView`.  
   - *Order*: Later—builds on item editing, complex UI shift.

9. **Sanity Checks**  
   - *Recap*: `CalendarItem::validate()` for data integrity, UI warnings in `EditPane`.  
   - *Order*: With editing—ensures robust changes.

10. **Sync Between Two `LocalBackend`s**  
    - *Recap*: Compare `Collection` items, resolve conflicts in `StagePane`, reuse `SessionManager::applyDeltaChanges`.  
    - *Order*: Late—requires stable deltas and views.

11. **Update `CalDAVBackend`**  
    - *Recap*: Sync with remote CalDAV, map to `CalendarItem`, integrate with dual-backend sync.  
    - *Order*: Last—extends sync, external dependency.

12. **Error Handling**  
    - *Recap*: UI feedback via `ErrorPane` or status bar for failures (e.g., delta load errors).  
    - *Order*: Early—runs parallel to core features.

13. **Undo/Redo**  
    - *Recap*: Use `SessionManager::m_deltaChanges` as undo stack, add redo stack.  
    - *Order*: With `StagePane`—deltas are key.

14. **Config Management UI**  
    - *Recap*: Dialog for `ConfigManager` to manage backend settings.  
    - *Order*: Mid-tier—useful once sync is live.

15. **Search/Filter**  
    - *Recap*: `ViewInterface::filter(const QString&)` for item search.  
    - *Order*: Later—enhances usability post-core.

16. **Performance Optimization**  
    - *Recap*: Proxy model for `Cal` to handle large datasets (e.g., 470 items in "Next Actions").  
    - *Order*: Last—tune after functionality.

**Ordering Logic**: Start with `ViewInterface` (foundation), clean architecture early, prioritize user-facing features (`StagePane`, editing), then tackle complex extensions (sync, Todos). Error handling and undo weave in early-to-mid for robustness.

---

## Roadmap with Stages and Milestones

### Stage 1: Foundation - `ViewInterface` and Architecture Cleanup
- **Goal**: Establish `ViewInterface`, redistribute logic for scalability.
- **Milestones**:
  1. Define `ViewInterface` base class with `setActiveCal(Cal*)` and `refresh()`.
  2. Refactor `CalendarView` → `CalendarTableView` (proactive), implement `itemSelected(QList<QSharedPointer<CalendarItem>>)` signal.
  3. Create passive `EditPane`, connect to `CalendarTableView::itemSelected`, emit `itemModified(QList<QSharedPointer<CalendarItem>>)`.
  4. Move `MainWindow::onCalendarAdded` and `onSelectionChanged` to `CalendarTableView`.
  5. Slim `CollectionController`, introduce `CalendarManager` for `getCal(const QString&)` and calendar ops.
  6. Update `MainWindow` to host MDI, toggle UI via `activeCollection`.

### Stage 2: Editing Workflow - Multi-Select and Locking
- **Goal**: Enable robust editing with proactive/passive interplay.
- **Milestones**:
  1. Add `QTableView::ExtendedSelection` to `CalendarTableView`.
  2. Implement `ItemEditorView` (proactive) for single/multi-item edits from `EditPane::spawnEditor()`.
  3. Add `Cal::lockItems(QList<QSharedPointer<CalendarItem>>)` and `unlockItems`, enforce in `EditPane` and `ItemEditorView`.
  4. Update `EditPane` with Apply/Discard/New Window options for unapplied changes.
  5. Test multi-select, spawn `ItemEditorView`, verify locking across views.

### Stage 3: Delta Visibility and Undo - `StagePane`
- **Goal**: Expose and manage `m_deltaChanges`, add undo/redo.
- **Milestones**:
  1. Create passive `StagePane`, display `SessionManager::m_deltaChanges` as a `QTableView`.
  2. Add Revert/Discard buttons in `StagePane`, update `SessionManager::m_deltaChanges`.
  3. Implement `QStack<DeltaChange>` in `SessionManager` for undo, redo stack for applied changes.
  4. Add undo/redo toolbar actions in `MainWindow`, connect to `StagePane`.
  5. Fix `SessionManager::queueDeltaChange` to append delta file, not overwrite.

### Stage 4: Item and Calendar Management
- **Goal**: Full CRUD for `CalendarItem` and `Cal`.
- **Milestones**:
  1. Add “New Item” button in `CalendarTableView`, form in `EditPane`, call `Cal::addItem`.
  2. Implement context menu delete in `CalendarTableView`, use `Cal::removeItem`.
  3. Add “New Calendar” menu action in `MainWindow`, call `Collection::addCalendar`.
  4. Implement calendar delete in `Collection`, update `LocalBackend`.
  5. Test add/delete, verify `StagePane` reflects deltas.

### Stage 5: Enhanced Editing - Times, Dates, Progress, and Sanity
- **Goal**: Complete `CalendarItem` editing with validation.
- **Milestones**:
  1. Expand `EditPane` and `ItemEditorView` with `QDateTimeEdit` for `dtStart`, `dtEnd`, `due`, and `QSpinBox` for `percentComplete`.
  2. Update `CalendarItem::data(int)` and setters for new fields.
  3. Add `CalendarItem::validate()`, check in `Cal::updateItem`, show warnings in `EditPane`.
  4. Test editing all fields, verify validation.

### Stage 6: Clipboard and Search
- **Goal**: Add usability features for item manipulation.
- **Milestones**:
  1. Implement Copy/Cut in `CalendarTableView` context menu, use `QClipboard`.
  2. Add Paste action, call `SessionManager::queueDeltaChange` with `CalendarItem::clone()`.
  3. Add `ViewInterface::filter(const QString&)` using `QSortFilterProxyModel` on `Cal`.
  4. Test clipboard ops and search across calendars.

### Stage 7: Hierarchical Todos
- **Goal**: Support nested `CalendarItem`s in `Cal`.
- **Milestones**:
  1. Add `QString parentId` to `CalendarItem`, update `data(int)`.
  2. Refactor `Cal` for hierarchy, override `parent()` and `index()` for `QTreeView`.
  3. Update `CalendarTableView` to toggle `QTableView`/`QTreeView`.
  4. Test nested Todos in tree view.

### Stage 8: Dual-Backend Sync
- **Goal**: Sync between two `LocalBackend`s.
- **Milestones**:
  1. Load second `LocalBackend` into a new `Collection`.
  2. Compare `Collection::calendars()` items, show conflicts in `StagePane`.
  3. Implement timestamp-based resolution, reuse `SessionManager::applyDeltaChanges`.
  4. Add sync menu action in `MainWindow`, test dual-backend sync.

### Stage 9: CalDAV Integration and Config
- **Goal**: Extend sync to `CalDAVBackend`, manage settings.
- **Milestones**:
  1. Update `CalDAVBackend::startSync` to fetch remote items, map to `CalendarItem`.
  2. Integrate with dual-backend sync logic, test CalDAV sync.
  3. Create `ConfigDialog` for `ConfigManager`, manage backend credentials.
  4. Add config menu action in `MainWindow`.

### Stage 10: Polish - Error Handling and Performance
- **Goal**: Robustness and scalability.
- **Milestones**:
  1. Add passive `ErrorPane` for failures (e.g., `SessionManager` errors), dock in `MainWindow`.
  2. Implement `QSortFilterProxyModel` for `Cal` in `CalendarTableView` for large datasets.
  3. Test error display and performance with 470+ items.

---

## Conclusion
This roadmap starts with `ViewInterface` to solidify the view architecture, progresses through editing and user-facing features, and ends with sync and polish. Each stage builds on the last, ensuring compilable milestones that incrementally deliver value. We’ve captured all directions—logic redistribution, views, editing, sync, and extras like undo and search—culminating in a robust, feature-rich TimeBuster.
